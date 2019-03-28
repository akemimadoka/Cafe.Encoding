#pragma once

#include <Encoding/Encode.h>
#include <IncludedEncoding.h> // 提前包含以阻止错误引入声明

namespace Cafe::Encoding::RuntimeEncoding
{
	using AllIncludedCodePages = std::integer_sequence<CodePageType
#define CAFE_CODEPAGE(codePageValue) , codePageValue
#include <IncludedEncoding.h>
	                                                   >;

	template <typename ResultUnitType>
	struct RuntimeEncodingResult
	{
		gsl::span<const ResultUnitType> Result;
		EncodingResultCode ResultCode;
		std::size_t AdvanceCount; ///< @brief 解码消费的编码单元数，若来源方运行时确定则是字节数
	};

	template <CodePageType CodePageValue>
	struct RuntimeEncoder
	{
		using UsingCodePageTrait = CodePageTrait<CodePageValue>;
		using CharType = typename UsingCodePageTrait::CharType;

	private:
		template <bool IsEncodeOne, CodePageType FromCodePage, typename OutputReceiver>
		static void EncodeFromImpl(gsl::span<const std::byte> const& src, OutputReceiver&& receiver)
		{
			using FromCodePageTrait = CodePageTrait<FromCodePage>;
			using FromCharType = typename FromCodePageTrait::CharType;
			std::unique_ptr<FromCharType[]> mayBeDynamicAllocatedBuffer{};
			const auto encodeUnit = [&]() constexpr
			{
				if constexpr (!IsEncodeOne || FromCodePageTrait::IsVariableWidth)
				{
					std::size_t dummy;
					if (std::align(alignof(FromCharType), src.size(), src.data(), dummy) == src.data())
					{
						return gsl::make_span(reinterpret_cast<const FromCharType*>(src.data()),
						                      src.size() / sizeof(FromCharType));
					}
					else
					{
						const auto size = src.size() / sizeof(FromCharType);
						if constexpr (IsEncodeOne && FromCodePageTrait::IsVariableWidth)
						{
							const auto testWidth = FromCodePageTrait::GetWidth(src.front());
							if (testWidth > 0)
							{
								size = testWidth;
							}
						}
						mayBeDynamicAllocatedBuffer = std::make_unique<FromCharType[]>(size);
						std::memcpy(mayBeDynamicAllocatedBuffer.get(), src.data(), size);
						return gsl::make_span(mayBeDynamicAllocatedBuffer, size);
					}
				}
				else
				{
					FromCharType fromChar{};
					std::memcpy(&fromChar, src.data(), sizeof(FromCharType));
					return fromChar;
				}
			}
			();

			const auto innerReceiver = [&](auto const& result) {
				if constexpr (GetEncodingResultCode<decltype(result)> == EncodingResultCode::Accept)
				{
					std::size_t advanceCount;
					gsl::span<const CharType> resultSpan;
					if constexpr (FromCodePageTrait::IsVariableWidth)
					{
						advanceCount = result.AdvanceCount;
						resultSpan = result.Result;
					}
					else
					{
						advanceCount = 1;
						resultSpan = gsl::make_span(&result.Result, 1);
					}
					std::forward<OutputReceiver>(receiver)(RuntimeEncodingResult<CharType>{
					    resultSpan, EncodingResultCode::Accept, advanceCount * sizeof(FromCharType) });
				}
				else
				{
					std::forward<OutputReceiver>(receiver)(
					    RuntimeEncodingResult<CharType>{ {}, GetEncodingResultCode<decltype(result)>, 0 });
				}
			};

			if constexpr (IsEncodeOne)
			{
				Encoder<FromCodePage, CodePageValue>::Encode(encodeUnit, innerReceiver);
			}
			else
			{
				Encoder<FromCodePage, CodePageValue>::EncodeAll(encodeUnit, innerReceiver);
			}
		}

	public:
		template <typename OutputReceiver>
		static void EncodeOneFrom(CodePageType fromCodePage, gsl::span<const std::byte> const& src,
		                          OutputReceiver&& receiver)
		{
			if (src.empty())
			{
				return;
			}

			switch (fromCodePage)
			{
#define CAFE_CODEPAGE(codePageValue)                                                               \
	case codePageValue:                                                                              \
		return EncodeFromImpl<true, codePageValue>(src, std::forward<OutputReceiver>(receiver));
#include <IncludedEncoding.h>
			default:
				assert(!"Invalid code page.");
				return;
			}
		}

		template <typename OutputReceiver>
		static void EncodeAllFrom(CodePageType fromCodePage, gsl::span<const std::byte> const& src,
		                          OutputReceiver&& receiver)
		{
			if (src.empty())
			{
				return;
			}

			switch (fromCodePage)
			{
#define CAFE_CODEPAGE(codePageValue)                                                               \
	case codePageValue:                                                                              \
		return EncodeFromImpl<false, codePageValue>(src, std::forward<OutputReceiver>(receiver));
#include <IncludedEncoding.h>
			default:
				assert(!"Invalid code page.");
				return;
			}
		}

	private:
		template <bool IsEncodeOne, CodePageType ToCodePage, typename OutputReceiver>
		static void EncodeOneToImpl(gsl::span<const CharType> const& src, OutputReceiver&& receiver)
		{
			using ToCodePageTrait = CodePageTrait<ToCodePage>;
			using ToCharType = typename ToCodePageTrait::CharType;

			const auto encodeUnit = [&]() constexpr
			{
				if constexpr (!IsEncodeOne || UsingCodePageTrait::IsVariableWidth)
				{
					return src;
				}
				else
				{
					return src.front();
				}
			}
			();

			const auto innerReceiver = [&](auto const& result) {
				if constexpr (GetEncodingResultCode<decltype(result)> == EncodingResultCode::Accept)
				{
					std::size_t advanceCount;
					gsl::span<const std::byte> resultSpan;
					if constexpr (UsingCodePageTrait::IsVariableWidth)
					{
						advanceCount = result.AdvanceCount;
					}
					else
					{
						advanceCount = 1;
					}

					if constexpr (ToCodePageTrait::IsVariableWidth)
					{
						resultSpan = gsl::as_bytes(result.Result);
					}
					else
					{
						resultSpan = gsl::make_span(&result.Result, 1);
					}

					std::forward<OutputReceiver>(receiver)(RuntimeEncodingResult<std::byte>{
					    resultSpan, EncodingResultCode::Accept, advanceCount });
				}
				else
				{
					std::forward<OutputReceiver>(receiver)(
					    RuntimeEncodingResult<std::byte>{ {}, GetEncodingResultCode<decltype(result)>, 0 });
				}
			};

			if constexpr (IsEncodeOne)
			{
				Encoder<CodePageValue, ToCodePage>::Encode(encodeUnit, innerReceiver);
			}
			else
			{
				Encoder<CodePageValue, ToCodePage>::EncodeAll(encodeUnit, innerReceiver);
			}
		}

	public:
		template <typename OutputReceiver>
		static void EncodeOneTo(gsl::span<const CharType> const& src, CodePageType toCodePage,
		                        OutputReceiver&& receiver)
		{
			if (src.empty())
			{
				return;
			}

			switch (toCodePage)
			{
#define CAFE_CODEPAGE(codePageValue)                                                               \
	case codePageValue:                                                                              \
		return EncodeToImpl<true, codePageValue>(src, std::forward<OutputReceiver>(receiver));
#include <IncludedEncoding.h>
			default:
				assert(!"Invalid code page.");
				return;
			}
		}

		template <typename OutputReceiver>
		static void EncodeAllTo(gsl::span<const CharType> const& src, CodePageType toCodePage,
		                        OutputReceiver&& receiver)
		{
			if (src.empty())
			{
				return;
			}

			switch (toCodePage)
			{
#define CAFE_CODEPAGE(codePageValue)                                                               \
	case codePageValue:                                                                              \
		return EncodeToImpl<false, codePageValue>(src, std::forward<OutputReceiver>(receiver));
#include <IncludedEncoding.h>
			default:
				assert(!"Invalid code page.");
				return;
			}
		}
	};

	template <typename OutputReceiver>
	void EncodeOne(CodePageType fromCodePage, gsl::span<const std::byte> const& src,
	               CodePageType toCodePage, OutputReceiver&& receiver)
	{
		switch (toCodePage)
		{
#define CAFE_CODEPAGE(codePageValue)                                                               \
	case codePageValue:                                                                              \
		RuntimeEncoder<codePageValue>::EncodeOneFrom(fromCodePage, src,                                \
		                                             std::forward<OutputReceiver>(receiver));          \
		break;
#include <IncludedEncoding.h>
		default:
			assert(!"Invalid code page.");
			return;
		}
	}

	template <typename OutputReceiver>
	void EncodeAll(CodePageType fromCodePage, gsl::span<const std::byte> const& src,
	               CodePageType toCodePage, OutputReceiver&& receiver)
	{
		switch (toCodePage)
		{
#define CAFE_CODEPAGE(codePageValue)                                                               \
	case codePageValue:                                                                              \
		RuntimeEncoder<codePageValue>::EncodeAllFrom(fromCodePage, src,                                \
		                                             std::forward<OutputReceiver>(receiver));          \
		break;
#include <IncludedEncoding.h>
		default:
			assert(!"Invalid code page.");
			return;
		}
	}
} // namespace Cafe::Encoding::RuntimeEncoding
