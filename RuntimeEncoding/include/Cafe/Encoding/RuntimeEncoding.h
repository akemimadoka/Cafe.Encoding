#pragma once

#include <Cafe/Encoding/Config/IncludedEncoding.h> // 提前包含以阻止错误引入声明
#include <Cafe/Encoding/Encode.h>
#include <Cafe/Misc/Export.h>
#include <cstring>
#include <optional>
#include <string_view>

namespace Cafe::Encoding::RuntimeEncoding
{
	using AllIncludedCodePages = std::integer_sequence<CodePage::CodePageType
#define CAFE_CODEPAGE(codePageValue) , codePageValue
#include <Cafe/Encoding/Config/IncludedEncoding.h>
	                                                   >;

	CAFE_PUBLIC gsl::span<const CodePage::CodePageType> GetSupportCodePages() noexcept;

	CAFE_PUBLIC std::string_view GetCodePageName(CodePage::CodePageType codePage) noexcept;
	CAFE_PUBLIC std::optional<bool> IsCodePageVariableWidth(CodePage::CodePageType codePage) noexcept;
	CAFE_PUBLIC std::size_t GetCodePageMaxWidth(CodePage::CodePageType codePage) noexcept;

#ifdef _WIN32
	CAFE_PUBLIC CodePage::CodePageType GetAnsiEncoding() noexcept;
	constexpr CodePage::CodePageType GetWideEncoding() noexcept
	{
		return CodePage::WideCharCodePage;
	}
#else
	constexpr CodePage::CodePageType GetNarrowCharEncoding() noexcept
	{
		return CodePage::NarrowCharCodePage;
	}
#endif

	template <typename ResultUnitType>
	struct RuntimeEncodingResult
	{
		gsl::span<const ResultUnitType> Result;
		EncodingResultCode ResultCode;
		std::size_t AdvanceCount; ///< @brief 编码消费的编码单元数，若来源方运行时确定则是字节数
	};

	template <CodePage::CodePageType CodePageValue>
	struct RuntimeEncoder
	{
		using UsingCodePageTrait = CodePage::CodePageTrait<CodePageValue>;
		using CharType = typename UsingCodePageTrait::CharType;

	private:
		template <bool IsEncodeOne, CodePage::CodePageType FromCodePage, typename OutputReceiver>
		static void EncodeFromImpl(gsl::span<const std::byte> const& src, OutputReceiver&& receiver)
		{
			using FromCodePageTrait = CodePage::CodePageTrait<FromCodePage>;
			using FromCharType = typename FromCodePageTrait::CharType;
			std::unique_ptr<FromCharType[]> mayBeDynamicAllocatedBuffer{};
			const auto encodeUnit = [&]() constexpr
			{
				if constexpr (!IsEncodeOne || FromCodePageTrait::IsVariableWidth)
				{
					std::size_t dummy;
					void* dummyPtr = const_cast<std::byte*>(src.data());
					if (std::align(alignof(FromCharType), src.size(), dummyPtr, dummy) == src.data())
					{
						return gsl::make_span(reinterpret_cast<const FromCharType*>(src.data()),
						                      src.size() / sizeof(FromCharType));
					}
					else
					{
						auto size = src.size() / sizeof(FromCharType);
						if constexpr (IsEncodeOne && FromCodePageTrait::IsVariableWidth)
						{
							const auto testWidth = FromCodePageTrait::GetWidth(src[0]);
							if (testWidth > 0)
							{
								size = testWidth;
							}
						}
						mayBeDynamicAllocatedBuffer = std::make_unique<FromCharType[]>(size);
						std::memcpy(mayBeDynamicAllocatedBuffer.get(), src.data(), src.size());
						return gsl::make_span(&std::as_const(*mayBeDynamicAllocatedBuffer.get()), size);
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
					}
					else
					{
						advanceCount = 1;
					}

					if constexpr (UsingCodePageTrait::IsVariableWidth)
					{
						resultSpan = result.Result;
					}
					else
					{
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
		static void EncodeOneFrom(CodePage::CodePageType fromCodePage,
		                          gsl::span<const std::byte> const& src, OutputReceiver&& receiver)
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
#include <Cafe/Encoding/Config/IncludedEncoding.h>
			default:
				assert(!"Invalid code page.");
				return;
			}
		}

		template <typename OutputReceiver>
		static void EncodeAllFrom(CodePage::CodePageType fromCodePage,
		                          gsl::span<const std::byte> const& src, OutputReceiver&& receiver)
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
#include <Cafe/Encoding/Config/IncludedEncoding.h>
			default:
				assert(!"Invalid code page.");
				return;
			}
		}

	private:
		template <bool IsEncodeOne, CodePage::CodePageType ToCodePage, typename OutputReceiver>
		static void EncodeToImpl(gsl::span<const CharType> const& src, OutputReceiver&& receiver)
		{
			using ToCodePageTrait = CodePage::CodePageTrait<ToCodePage>;
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
						resultSpan = gsl::as_bytes(gsl::make_span(&result.Result, 1));
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
		static void EncodeOneTo(gsl::span<const CharType> const& src, CodePage::CodePageType toCodePage,
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
#include <Cafe/Encoding/Config/IncludedEncoding.h>
			default:
				assert(!"Invalid code page.");
				return;
			}
		}

		template <typename OutputReceiver>
		static void EncodeAllTo(gsl::span<const CharType> const& src, CodePage::CodePageType toCodePage,
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
#include <Cafe/Encoding/Config/IncludedEncoding.h>
			default:
				assert(!"Invalid code page.");
				return;
			}
		}
	};

	template <typename OutputReceiver>
	void EncodeOne(CodePage::CodePageType fromCodePage, gsl::span<const std::byte> const& src,
	               CodePage::CodePageType toCodePage, OutputReceiver&& receiver)
	{
		switch (toCodePage)
		{
#define CAFE_CODEPAGE(codePageValue)                                                               \
	case codePageValue:                                                                              \
		RuntimeEncoder<codePageValue>::EncodeOneFrom(fromCodePage, src, [&](auto const& result) {      \
			std::forward<OutputReceiver>(receiver)(RuntimeEncodingResult<std::byte>{                     \
			    gsl::as_bytes(result.Result), result.ResultCode, result.AdvanceCount });                 \
		});                                                                                            \
		break;
#include <Cafe/Encoding/Config/IncludedEncoding.h>
		default:
			assert(!"Invalid code page.");
			return;
		}
	}

	template <typename OutputReceiver>
	void EncodeAll(CodePage::CodePageType fromCodePage, gsl::span<const std::byte> const& src,
	               CodePage::CodePageType toCodePage, OutputReceiver&& receiver)
	{
		switch (toCodePage)
		{
#define CAFE_CODEPAGE(codePageValue)                                                               \
	case codePageValue:                                                                              \
		RuntimeEncoder<codePageValue>::EncodeAllFrom(fromCodePage, src, [&](auto const& result) {      \
			std::forward<OutputReceiver>(receiver)(RuntimeEncodingResult<std::byte>{                     \
			    gsl::as_bytes(result.Result), result.ResultCode, result.AdvanceCount });                 \
		});                                                                                            \
		break;
#include <Cafe/Encoding/Config/IncludedEncoding.h>
		default:
			assert(!"Invalid code page.");
			return;
		}
	}
} // namespace Cafe::Encoding::RuntimeEncoding
