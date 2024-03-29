#pragma once

#include <Cafe/Encoding/Config/IncludedEncoding.h> // 提前包含以阻止错误引入声明
#include <Cafe/Encoding/Encode.h>
#include <Cafe/Misc/Export.h>
#include <cstring>
#include <optional>
#include <string_view>
#include <vector>

namespace Cafe::Encoding::RuntimeEncoding
{
	using AllIncludedCodePages = std::integer_sequence<CodePage::CodePageType
#define CAFE_CODEPAGE(codePageValue) , codePageValue
#include <Cafe/Encoding/Config/IncludedEncoding.h>
	                                                   >;

	CAFE_PUBLIC std::span<const CodePage::CodePageType> GetSupportCodePages() noexcept;

	CAFE_PUBLIC std::string_view GetCodePageName(CodePage::CodePageType codePage) noexcept;
	CAFE_PUBLIC std::optional<bool>
	IsCodePageVariableWidth(CodePage::CodePageType codePage) noexcept;
	CAFE_PUBLIC std::size_t GetCodePageMaxWidth(CodePage::CodePageType codePage) noexcept;

	template <typename ResultUnitType>
	struct RuntimeEncodingResult
	{
		std::span<const ResultUnitType> Result;
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
		static void EncodeFromImpl(std::span<const std::byte> const& src, OutputReceiver&& receiver)
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
					if (std::align(alignof(FromCharType), src.size(), dummyPtr, dummy) ==
					    src.data())
					{
						return std::span(reinterpret_cast<const FromCharType*>(src.data()),
						                 src.size() / sizeof(FromCharType));
					}
					else
					{
						auto size = src.size() / sizeof(FromCharType);
						if constexpr (IsEncodeOne && FromCodePageTrait::IsVariableWidth)
						{
							const auto testWidth = FromCodePageTrait::GetWidth(static_cast<FromCharType>(src[0]));
							if (testWidth > 0)
							{
								size = testWidth;
							}
						}
						mayBeDynamicAllocatedBuffer = std::make_unique<FromCharType[]>(size);
						std::memcpy(mayBeDynamicAllocatedBuffer.get(), src.data(), src.size());
						return std::span(&std::as_const(*mayBeDynamicAllocatedBuffer.get()), size);
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
					std::size_t advanceCount = result.AdvanceCount;
					std::span<const CharType> resultSpan;

					if constexpr (UsingCodePageTrait::IsVariableWidth)
					{
						resultSpan = result.Result;
					}
					else
					{
						resultSpan = std::span(&result.Result, 1);
					}
					std::forward<OutputReceiver>(receiver)(
					    RuntimeEncodingResult<CharType>{ resultSpan, EncodingResultCode::Accept,
					                                     advanceCount * sizeof(FromCharType) });
				}
				else
				{
					std::forward<OutputReceiver>(receiver)(RuntimeEncodingResult<CharType>{
					    {}, GetEncodingResultCode<decltype(result)>, 0 });
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
		                          std::span<const std::byte> const& src, OutputReceiver&& receiver)
		{
			if (src.empty())
			{
				return;
			}

			switch (fromCodePage)
			{
#define CAFE_CODEPAGE(codePageValue)                                                               \
	case codePageValue:                                                                            \
		return EncodeFromImpl<true, codePageValue>(src, std::forward<OutputReceiver>(receiver));
#include <Cafe/Encoding/Config/IncludedEncoding.h>
			default:
				assert(!"Invalid code page.");
				return;
			}
		}

		template <typename OutputReceiver>
		static void EncodeAllFrom(CodePage::CodePageType fromCodePage,
		                          std::span<const std::byte> const& src, OutputReceiver&& receiver)
		{
			if (src.empty())
			{
				return;
			}

			switch (fromCodePage)
			{
#define CAFE_CODEPAGE(codePageValue)                                                               \
	case codePageValue:                                                                            \
		return EncodeFromImpl<false, codePageValue>(src, std::forward<OutputReceiver>(receiver));
#include <Cafe/Encoding/Config/IncludedEncoding.h>
			default:
				assert(!"Invalid code page.");
				return;
			}
		}

	private:
		template <bool IsEncodeOne, CodePage::CodePageType ToCodePage, typename OutputReceiver>
		static void EncodeToImpl(std::span<const CharType> const& src, OutputReceiver&& receiver)
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
					std::size_t advanceCount = result.AdvanceCount;
					std::span<const std::byte> resultSpan;

					if constexpr (ToCodePageTrait::IsVariableWidth)
					{
						resultSpan = std::as_bytes(result.Result);
					}
					else
					{
						resultSpan = std::as_bytes(std::span(&result.Result, 1));
					}

					std::forward<OutputReceiver>(receiver)(RuntimeEncodingResult<std::byte>{
					    resultSpan, EncodingResultCode::Accept, advanceCount });
				}
				else
				{
					std::forward<OutputReceiver>(receiver)(RuntimeEncodingResult<std::byte>{
					    {}, GetEncodingResultCode<decltype(result)>, 0 });
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
		static void EncodeOneTo(std::span<const CharType> const& src,
		                        CodePage::CodePageType toCodePage, OutputReceiver&& receiver)
		{
			if (src.empty())
			{
				return;
			}

			switch (toCodePage)
			{
#define CAFE_CODEPAGE(codePageValue)                                                               \
	case codePageValue:                                                                            \
		return EncodeToImpl<true, codePageValue>(src, std::forward<OutputReceiver>(receiver));
#include <Cafe/Encoding/Config/IncludedEncoding.h>
			default:
				assert(!"Invalid code page.");
				return;
			}
		}

		template <typename OutputReceiver>
		static void EncodeAllTo(std::span<const CharType> const& src,
		                        CodePage::CodePageType toCodePage, OutputReceiver&& receiver)
		{
			if (src.empty())
			{
				return;
			}

			switch (toCodePage)
			{
#define CAFE_CODEPAGE(codePageValue)                                                               \
	case codePageValue:                                                                            \
		return EncodeToImpl<false, codePageValue>(src, std::forward<OutputReceiver>(receiver));
#include <Cafe/Encoding/Config/IncludedEncoding.h>
			default:
				assert(!"Invalid code page.");
				return;
			}
		}
	};

	template <typename OutputReceiver>
	void EncodeOne(CodePage::CodePageType fromCodePage, std::span<const std::byte> const& src,
	               CodePage::CodePageType toCodePage, OutputReceiver&& receiver)
	{
		switch (toCodePage)
		{
#define CAFE_CODEPAGE(codePageValue)                                                               \
	case codePageValue:                                                                            \
		RuntimeEncoder<codePageValue>::EncodeOneFrom(fromCodePage, src, [&](auto const& result) {  \
			std::forward<OutputReceiver>(receiver)(RuntimeEncodingResult<std::byte>{               \
			    std::as_bytes(result.Result), result.ResultCode, result.AdvanceCount });           \
		});                                                                                        \
		break;
#include <Cafe/Encoding/Config/IncludedEncoding.h>
		default:
			assert(!"Invalid code page.");
			return;
		}
	}

	template <typename OutputReceiver>
	void EncodeAll(CodePage::CodePageType fromCodePage, std::span<const std::byte> const& src,
	               CodePage::CodePageType toCodePage, OutputReceiver&& receiver)
	{
		switch (toCodePage)
		{
#define CAFE_CODEPAGE(codePageValue)                                                               \
	case codePageValue:                                                                            \
		RuntimeEncoder<codePageValue>::EncodeAllFrom(fromCodePage, src, [&](auto const& result) {  \
			std::forward<OutputReceiver>(receiver)(RuntimeEncodingResult<std::byte>{               \
			    std::as_bytes(result.Result), result.ResultCode, result.AdvanceCount });           \
		});                                                                                        \
		break;
#include <Cafe/Encoding/Config/IncludedEncoding.h>
		default:
			assert(!"Invalid code page.");
			return;
		}
	}

	enum class RuntimeEncodingResultCode
	{
		Accept = static_cast<int>(EncodingResultCode::Accept),
		Incomplete = static_cast<int>(EncodingResultCode::Incomplete),
		Reject = static_cast<int>(EncodingResultCode::Reject),

		BufferTooSmall,
	};

	struct RuntimeEncodingToSpanResult
	{
		RuntimeEncodingResultCode ResultCode;
		std::size_t ConsumeCount;
		std::size_t ProduceCount;
	};

	RuntimeEncodingToSpanResult EncodeOneToSpan(CodePage::CodePageType fromCodePage,
	                                            std::span<const std::byte> const& src,
	                                            CodePage::CodePageType toCodePage,
	                                            std::span<std::byte> const& dst);

	RuntimeEncodingToSpanResult EncodeAllToSpan(CodePage::CodePageType fromCodePage,
	                                            std::span<const std::byte> const& src,
	                                            CodePage::CodePageType toCodePage,
	                                            std::span<std::byte> const& dst);

	RuntimeEncodingToSpanResult EncodeAllToSpanWithReplacement(
	    CodePage::CodePageType fromCodePage, std::span<const std::byte> const& src,
	    CodePage::CodePageType toCodePage, std::span<std::byte> const& dst,
	    std::span<std::byte> const& replacement);
} // namespace Cafe::Encoding::RuntimeEncoding
