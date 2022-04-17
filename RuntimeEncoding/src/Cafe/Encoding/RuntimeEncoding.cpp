#include <Cafe/Encoding/RuntimeEncoding.h>
#include <Cafe/Misc/Utility.h>

namespace Cafe::Encoding::RuntimeEncoding
{
	std::span<const Cafe::Encoding::CodePage::CodePageType> GetSupportCodePages() noexcept
	{
		return std::span(Cafe::Core::Misc::SequenceToArray<AllIncludedCodePages>::Array);
	}

	std::string_view GetCodePageName(CodePage::CodePageType codePage) noexcept
	{
		switch (codePage)
		{
#define CAFE_CODEPAGE(codePageValue)                                                               \
	case codePageValue:                                                                            \
		return CodePage::CodePageTrait<codePageValue>::Name;
#include <Cafe/Encoding/Config/IncludedEncoding.h>
		default:
			return {};
		}
	}

	std::optional<bool> IsCodePageVariableWidth(CodePage::CodePageType codePage) noexcept
	{
		switch (codePage)
		{
#define CAFE_CODEPAGE(codePageValue)                                                               \
	case codePageValue:                                                                            \
		return CodePage::CodePageTrait<codePageValue>::IsVariableWidth;
#include <Cafe/Encoding/Config/IncludedEncoding.h>
		default:
			return {};
		}
	}

	std::size_t GetCodePageMaxWidth(CodePage::CodePageType codePage) noexcept
	{
		switch (codePage)
		{
#define CAFE_CODEPAGE(codePageValue)                                                               \
	case codePageValue:                                                                            \
		return CodePage::GetMaxWidth<codePageValue>();
#include <Cafe/Encoding/Config/IncludedEncoding.h>
		default:
			return {};
		}
	}

	RuntimeEncodingToSpanResult EncodeOneToSpan(CodePage::CodePageType fromCodePage,
	                                            std::span<const std::byte> const& src,
	                                            CodePage::CodePageType toCodePage,
	                                            std::span<std::byte> const& dst)
	{
		RuntimeEncodingResultCode resultCode;
		std::size_t consumeCount;
		std::size_t produceCount;
		EncodeOne(fromCodePage, src, toCodePage, [&](auto const& result) {
			consumeCount = result.AdvanceCount;
			if (dst.size() < result.Result.size())
			{
				resultCode = RuntimeEncodingResultCode::BufferTooSmall;
				produceCount = dst.size();
			}
			else
			{
				resultCode = static_cast<RuntimeEncodingResultCode>(result.ResultCode);
				produceCount = result.Result.size();
			}
			std::memcpy(dst.data(), result.Result.data(), produceCount);
		});
		return {
			.ResultCode = resultCode,
			.ConsumeCount = consumeCount,
			.ProduceCount = produceCount,
		};
	}

	RuntimeEncodingToSpanResult EncodeAllToSpan(CodePage::CodePageType fromCodePage,
	                                            std::span<const std::byte> const& src,
	                                            CodePage::CodePageType toCodePage,
	                                            std::span<std::byte> const& dst)
	{
		RuntimeEncodingResultCode resultCode;
		std::size_t consumeCount = 0;
		std::size_t produceCount = 0;
		auto dstPtr = dst.data();
		const auto dstEnd = dstPtr + dst.size();
		EncodeAll(fromCodePage, src, toCodePage,
		          [&](auto const& result) -> Core::Misc::ControlFlowVariant<> {
			          consumeCount += result.AdvanceCount;
			          const auto restBufferSize = dstEnd - dstPtr;
			          std::size_t currentProduceCount;
			          if (restBufferSize < result.Result.size())
			          {
				          resultCode = RuntimeEncodingResultCode::BufferTooSmall;
				          currentProduceCount = restBufferSize;
			          }
			          else
			          {
				          resultCode = static_cast<RuntimeEncodingResultCode>(result.ResultCode);
				          currentProduceCount = result.Result.size();
			          }
			          std::memcpy(dstPtr, result.Result.data(), currentProduceCount);
			          dstPtr += currentProduceCount;
			          produceCount += currentProduceCount;
			          if (resultCode != RuntimeEncodingResultCode::Accept)
			          {
				          return Core::Misc::BreakType{};
			          }
			          else
			          {
				          return Core::Misc::ContinueType{};
			          }
		          });
		return {
			.ResultCode = resultCode,
			.ConsumeCount = consumeCount,
			.ProduceCount = produceCount,
		};
	}

	RuntimeEncodingToSpanResult EncodeAllToSpanWithReplacement(
	    CodePage::CodePageType fromCodePage, std::span<const std::byte> const& src,
	    CodePage::CodePageType toCodePage, std::span<std::byte> const& dst,
	    std::span<std::byte> const& replacement)
	{
		RuntimeEncodingResultCode resultCode;
		std::size_t consumeCount = 0;
		std::size_t produceCount = 0;
		auto dstPtr = dst.data();
		const auto dstEnd = dstPtr + dst.size();
		EncodeAll(fromCodePage, src, toCodePage,
		          [&](auto const& result) -> Core::Misc::ControlFlowVariant<> {
			          consumeCount += result.AdvanceCount;
			          const auto restBufferSize = dstEnd - dstPtr;
			          std::size_t currentProduceCount;
			          if (restBufferSize < result.Result.size())
			          {
				          resultCode = RuntimeEncodingResultCode::BufferTooSmall;
			          }
			          else
			          {
				          resultCode = static_cast<RuntimeEncodingResultCode>(result.ResultCode);
				          if (resultCode == RuntimeEncodingResultCode::Accept)
				          {
					          currentProduceCount = result.Result.size();
					          std::memcpy(dstPtr, result.Result.data(), currentProduceCount);
					          dstPtr += currentProduceCount;
					          produceCount += currentProduceCount;
				          }
				          else if (restBufferSize < replacement.size()) [[unlikely]]
				          {
					          resultCode = RuntimeEncodingResultCode::BufferTooSmall;
				          }
				          else
				          {
					          currentProduceCount = replacement.size();
					          std::memcpy(dstPtr, replacement.data(), currentProduceCount);
					          dstPtr += currentProduceCount;
					          produceCount += currentProduceCount;
				          }
			          }
			          if (resultCode == RuntimeEncodingResultCode::BufferTooSmall)
			          {
				          return Core::Misc::BreakType{};
			          }
			          else
			          {
				          return Core::Misc::ContinueType{};
			          }
		          });
		return {
			.ResultCode = resultCode,
			.ConsumeCount = consumeCount,
			.ProduceCount = produceCount,
		};
	}
} // namespace Cafe::Encoding::RuntimeEncoding
