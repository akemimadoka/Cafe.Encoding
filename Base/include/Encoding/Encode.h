#pragma once

#include "CodePage.h"
#include <Misc/TypeTraits.h>
#include <gsl/span>

namespace Cafe::Encoding
{
	enum class EncodingResultCode
	{
		Accept,
		Incomplete,
		Reject
	};

	template <CodePage::CodePageType FromCodePageValue, CodePage::CodePageType ToCodePageValue,
	          EncodingResultCode Result>
	struct EncodingResult;

	namespace Detail
	{
		struct AdvanceCountInfo
		{
			explicit constexpr AdvanceCountInfo(std::size_t advanceCount = 1) noexcept
			    : AdvanceCount{ advanceCount }
			{
			}

			std::size_t AdvanceCount;
		};

		template <typename ToType, typename FromType>
		constexpr ToType CastOrDefaultConstruct(FromType&& value)
		{
			if constexpr (std::is_convertible_v<FromType&&, ToType>)
			{
				return static_cast<ToType>(std::forward<FromType>(value));
			}
			else
			{
				return ToType{};
			}
		}
	} // namespace Detail

	// Result 字段是解码的结果，若是变长编码其类型为
	// span<编码单元>，否则为单独的编码单元 对于变长代码页会自动增加 AdvanceCount
	// 字段，指定获取 Result 所消耗的源代码页编码单元数量
	// 对于所有特化，应允许不同来源代码页的 EncodingResult
	// 相互转换，以便扩展传递额外信息
	template <CodePage::CodePageType FromCodePageValue, CodePage::CodePageType ToCodePageValue>
	struct EncodingResult<FromCodePageValue, ToCodePageValue, EncodingResultCode::Accept>
	    : Core::Misc::DeriveIf<CodePage::CodePageTrait<FromCodePageValue>::IsVariableWidth,
	                           Detail::AdvanceCountInfo>
	{
		using Base = Core::Misc::DeriveIf<CodePage::CodePageTrait<FromCodePageValue>::IsVariableWidth,
		                                  Detail::AdvanceCountInfo>;

		using ResultType = std::conditional_t<
		    CodePage::CodePageTrait<ToCodePageValue>::IsVariableWidth,
		    gsl::span<const typename CodePage::CodePageTrait<ToCodePageValue>::CharType>,
		    typename CodePage::CodePageTrait<ToCodePageValue>::CharType>;

		template <typename... Args>
		explicit constexpr EncodingResult(ResultType const& result, Args&&... args)
		    : Base{ std::forward<Args>(args)... }, Result{ result }
		{
		}

		template <CodePage::CodePageType OtherFromCodePageValue>
		explicit constexpr EncodingResult(
		    EncodingResult<OtherFromCodePageValue, ToCodePageValue, EncodingResultCode::Accept> const&
		        other) noexcept
		    : Base{ Detail::CastOrDefaultConstruct<Base>(other) }, Result{ other.Result }
		{
		}

		ResultType Result;
	};

	// 仅变长代码页会出现的情况
	// 对于所有特化，应允许默认构造，允许不同来源和结果代码页的 EncodingResult
	// 相互转换，以便扩展传递额外信息
	template <CodePage::CodePageType FromCodePageValue, CodePage::CodePageType ToCodePageValue>
	struct EncodingResult<FromCodePageValue, ToCodePageValue, EncodingResultCode::Incomplete>
	{
		explicit constexpr EncodingResult(std::ptrdiff_t offsetHint = 0,
		                                  std::size_t lackHint = 0) noexcept
		    : OffsetHint{ offsetHint }, LackHint{ lackHint }
		{
		}

		template <CodePage::CodePageType OtherFromCodePageValue,
		          CodePage::CodePageType OtherToCodePageValue>
		explicit constexpr EncodingResult(
		    EncodingResult<OtherFromCodePageValue, OtherToCodePageValue,
		                   EncodingResultCode::Incomplete> const& other) noexcept
		    : OffsetHint{ other.OffsetHint }, LackHint{ other.LackHint }
		{
		}

		// 指示要获得完整的编码需至少对输入序列进行偏移的个数，若无法提供此信息则为
		// 0
		std::ptrdiff_t OffsetHint;
		// 指示至少可能缺失的源编码单元数量，若无法提供此信息则为 0
		std::size_t LackHint;
	};

	// 对于特殊的编码可能有描述原因的字段
	// 对于所有特化，应允许默认构造，允许不同来源和结果代码页的 EncodingResult
	// 相互转换，以便扩展传递额外信息
	template <CodePage::CodePageType FromCodePageValue, CodePage::CodePageType ToCodePageValue>
	struct EncodingResult<FromCodePageValue, ToCodePageValue, EncodingResultCode::Reject>
	{
		constexpr EncodingResult() = default;

		template <CodePage::CodePageType OtherFromCodePageValue,
		          CodePage::CodePageType OtherToCodePageValue>
		explicit constexpr EncodingResult(EncodingResult<OtherFromCodePageValue, OtherToCodePageValue,
		                                                 EncodingResultCode::Reject> const&) noexcept
		{
		}
	};

	template <typename ResultType>
	struct GetEncodingResultInfoTrait;

	template <CodePage::CodePageType FromCodePageValue, CodePage::CodePageType ToCodePageValue,
	          EncodingResultCode Result>
	struct GetEncodingResultInfoTrait<EncodingResult<FromCodePageValue, ToCodePageValue, Result>>
	    : std::integral_constant<EncodingResultCode, Result>
	{
		static constexpr CodePage::CodePageType FromCodePage = FromCodePageValue;
		static constexpr CodePage::CodePageType ToCodePage = ToCodePageValue;
	};

	template <typename ResultType>
	constexpr CodePage::CodePageType GetFromCodePage = GetEncodingResultInfoTrait<
	    std::remove_cv_t<std::remove_reference_t<ResultType>>>::FromCodePage;

	template <typename ResultType>
	constexpr CodePage::CodePageType GetToCodePage =
	    GetEncodingResultInfoTrait<std::remove_cv_t<std::remove_reference_t<ResultType>>>::ToCodePage;

	template <typename ResultType>
	constexpr EncodingResultCode GetEncodingResultCode =
	    GetEncodingResultInfoTrait<std::remove_cv_t<std::remove_reference_t<ResultType>>>::value;

	template <CodePage::CodePageType FromCodePageValue, CodePage::CodePageType ToCodePageValue>
	struct EncoderBase
	{
		using EncodeUnitType = std::conditional_t<
		    CodePageTrait<FromCodePage>::IsVariableWidth,
		    gsl::span<const typename CodePage::CodePageTrait<FromCodePageValue>::CharType>,
		    typename CodePage::CodePageTrait<FromCodePageValue>::CharType>;

		template <typename OutputReceiver>
		static constexpr void Encode(EncodeUnitType const& encodeUnit, OutputReceiver&& receiver)
		{
			CodePage::CodePageTrait<FromCodePageValue>::ToCodePoint(encodeUnit, [&](auto const& result) {
				constexpr auto resultCode = GetEncodingResultCode<decltype(result)>;
				if constexpr (resultCode == EncodingResultCode::Accept)
				{
					CodePage::CodePageTrait<ToCodePageValue>::FromCodePoint(
					    result.Result, [&](auto const& finalResult) {
						    std::forward<OutputReceiver>(receiver)(
						        static_cast<EncodingResult<FromCodePageValue, ToCodePageValue,
						                                   GetEncodingResultCode<decltype(finalResult)>>>(
						            finalResult));
					    });
				}
				else
				{
					std::forward<OutputReceiver>(receiver)(
					    static_cast<EncodingResult<FromCodePageValue, ToCodePageValue, resultCode>>(result));
				}
			});
		}

		template <std::ptrdiff_t Extent, typename OutputReceiver>
		static constexpr void
		EncodeAll(gsl::span<const typename CodePage::CodePageTrait<FromCodePageValue>::CharType,
		                    Extent> const& span,
		          OutputReceiver&& receiver)
		{
			gsl::span<const typename CodePage::CodePageTrait<FromCodePageValue>::CharType> remainedSpan =
			    span;
			while (!remainedSpan.empty())
			{
				const auto encodeUnit = [&]() constexpr
				{
					if constexpr (CodePageTrait<FromCodePage>::IsVariableWidth)
					{
						return remainedSpan;
					}
					else
					{
						return remainedSpan.front();
					}
				}
				();
				Encode(encodeUnit, [&](auto const& result) {
					if constexpr (GetEncodingResultCode<decltype(result)> == EncodingResultCode::Accept)
					{
						if constexpr (CodePage::CodePageTrait<FromCodePageValue>::IsVariableWidth)
						{
							remainedSpan = remainedSpan.subspan(result.AdvanceCount);
						}
						else
						{
							remainedSpan = remainedSpan.subspan(1);
						}
					}
					else
					{
						// 出现错误时终止循环
						remainedSpan =
						    gsl::span<const typename CodePage::CodePageTrait<FromCodePageValue>::CharType>{};
					}

					std::forward<OutputReceiver>(receiver)(result);
				});
			}
		}
	};

	/// @brief  编码器
	/// @remark 使用类模板是为了允许可能的特化优化版本，继承是为了允许只重写 EncodeOne 版本，也允许全部重写以获得最佳性能
	template <CodePage::CodePageType FromCodePageValue, CodePage::CodePageType ToCodePageValue>
	struct Encoder : EncoderBase<FromCodePageValue, ToCodePageValue>
	{
	};

	template <>
	struct CodePage::CodePageTrait<CodePage::CodePoint>
	{
		static constexpr const char Name[] = "Code Point";

		using CharType = CodePointType;

		static constexpr bool IsVariableWidth = false;

		template <typename OutputReceiver>
		static constexpr void ToCodePoint(CharType value, OutputReceiver&& receiver)
		{
			std::forward<OutputReceiver>(receiver)(
			    EncodingResult<CodePoint, CodePoint, EncodingResultCode::Accept>{ value });
		}

		template <typename OutputReceiver>
		static constexpr void FromCodePoint(CodePointType value, OutputReceiver&& receiver)
		{
			std::forward<OutputReceiver>(receiver)(
			    EncodingResult<CodePoint, CodePoint, EncodingResultCode::Accept>{ value });
		}
	};
} // namespace Cafe::Encoding
