#pragma once

#include "CodePage.h"
#include <Cafe/Misc/TypeTraits.h>
#include <span>

namespace Cafe::Encoding
{
	/// @brief  编码结果代码
	enum class EncodingResultCode
	{
		Accept,     ///< @brief 编码成功
		Incomplete, ///< @brief 不完整
		Reject      ///< @brief 编码失败，不可构成码点
	};

	/// @brief  编码结果，具体实现根据特化不同，部分约定可查看通用特化版本
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

		struct AlwaysOneCountInfo
		{
			std::integral_constant<std::size_t, 1> AdvanceCount;
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

	/// @brief  编码成功的通用版本
	/// @remark 对于变长代码页会自动增加 AdvanceCount
	///         字段，指定获取 Result 所消耗的源代码页编码单元数量
	///         对于所有特化，应允许不同来源代码页的 EncodingResult
	///         相互转换，以便扩展传递额外信息
	template <CodePage::CodePageType FromCodePageValue, CodePage::CodePageType ToCodePageValue>
	struct EncodingResult<FromCodePageValue, ToCodePageValue, EncodingResultCode::Accept>
	    : Core::Misc::DeriveIf<CodePage::CodePageTrait<FromCodePageValue>::IsVariableWidth,
	                           Detail::AdvanceCountInfo, Detail::AlwaysOneCountInfo>
	{
		using Base = Core::Misc::DeriveIf<CodePage::CodePageTrait<FromCodePageValue>::IsVariableWidth,
		                                  Detail::AdvanceCountInfo, Detail::AlwaysOneCountInfo>;

		using ResultType = std::conditional_t<
		    CodePage::CodePageTrait<ToCodePageValue>::IsVariableWidth,
		    std::span<const typename CodePage::CodePageTrait<ToCodePageValue>::CharType>,
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

		/// @brief  编码的结果
		/// @remark 若是变长编码其类型为 span<编码单元>，否则为单独的编码单元
		ResultType Result;
	};

	/// @brief  编码不完整的通用版本
	/// @remark 仅变长代码页会出现的情况
	///         对于所有特化，应允许默认构造，允许不同来源和结果代码页的 EncodingResult
	///         相互转换，以便扩展传递额外信息
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

		/// @brief  指示要获得完整的编码需至少对输入序列进行偏移的个数
		/// @remark 若无法提供此信息则为 0
		std::ptrdiff_t OffsetHint;
		/// @brief  指示至少可能缺失的源编码单元数量
		///         若无法提供此信息则为 0
		std::size_t LackHint;
	};

	/// @brief  编码失败的通用版本
	/// @remark 对于特殊的编码可能有描述原因的字段
	///         对于所有特化，应允许默认构造，允许不同来源和结果代码页的 EncodingResult
	///         相互转换，以便扩展传递额外信息
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

	/// @brief  用于快速获取编码结果的共有信息的工具模板
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

	/// @brief  用于快速获取编码结果的来源代码页的工具模板
	template <typename ResultType>
	constexpr CodePage::CodePageType GetFromCodePage = GetEncodingResultInfoTrait<
	    std::remove_cv_t<std::remove_reference_t<ResultType>>>::FromCodePage;

	/// @brief  用于快速获取编码结果的目标代码页的工具模板
	template <typename ResultType>
	constexpr CodePage::CodePageType GetToCodePage =
	    GetEncodingResultInfoTrait<std::remove_cv_t<std::remove_reference_t<ResultType>>>::ToCodePage;

	/// @brief  用于快速获取编码结果的结果代码的工具模板
	template <typename ResultType>
	constexpr EncodingResultCode GetEncodingResultCode =
	    GetEncodingResultInfoTrait<std::remove_cv_t<std::remove_reference_t<ResultType>>>::value;

	/// @brief  编码器基类
	/// @see    Cafe::Encoding::Encoder
	template <CodePage::CodePageType FromCodePageValue, CodePage::CodePageType ToCodePageValue>
	struct EncoderBase
	{
		using FromCodePageTrait = CodePage::CodePageTrait<FromCodePageValue>;
		using CharType = typename FromCodePageTrait::CharType;
		using EncodeUnitType =
		    std::conditional_t<FromCodePageTrait::IsVariableWidth, std::span<const CharType>, CharType>;

		template <typename OutputReceiver>
		static constexpr void Encode(EncodeUnitType const& encodeUnit, OutputReceiver&& receiver)
		{
			if constexpr (FromCodePageValue == ToCodePageValue)
			{
				if constexpr (FromCodePageTrait::IsVariableWidth)
				{
					std::forward<OutputReceiver>(receiver)(
					    EncodingResult<FromCodePageValue, ToCodePageValue, EncodingResultCode::Accept>{
					        encodeUnit, static_cast<std::size_t>(encodeUnit.size()) });
				}
				else
				{
					std::forward<OutputReceiver>(receiver)(
					    EncodingResult<FromCodePageValue, ToCodePageValue, EncodingResultCode::Accept>{
					        encodeUnit });
				}
			}
			else if constexpr (FromCodePageValue == CodePage::CodePoint)
			{
				CodePage::CodePageTrait<ToCodePageValue>::FromCodePoint(
				    encodeUnit, std::forward<OutputReceiver>(receiver));
			}
			else if constexpr (ToCodePageValue == CodePage::CodePoint)
			{
				FromCodePageTrait::ToCodePoint(encodeUnit, std::forward<OutputReceiver>(receiver));
			}
			else
			{
				FromCodePageTrait::ToCodePoint(encodeUnit, [&](auto const& result) {
					constexpr auto resultCode = GetEncodingResultCode<decltype(result)>;
					if constexpr (resultCode == EncodingResultCode::Accept)
					{
						CodePage::CodePageTrait<ToCodePageValue>::FromCodePoint(
						    result.Result, [&](auto const& finalResult) {
							    auto convertedResult =
							        static_cast<EncodingResult<FromCodePageValue, ToCodePageValue,
							                                   GetEncodingResultCode<decltype(finalResult)>>>(
							            finalResult);
							    if constexpr (FromCodePageTrait::IsVariableWidth &&
							                  GetEncodingResultCode<decltype(finalResult)> ==
							                      EncodingResultCode::Accept)
							    {
								    // 目前只有这个信息需要保留，或许会有其他自定义信息需要保留
								    convertedResult.AdvanceCount = result.AdvanceCount;
							    }
							    std::forward<OutputReceiver>(receiver)(convertedResult);
						    });
					}
					else
					{
						std::forward<OutputReceiver>(receiver)(
						    static_cast<EncodingResult<FromCodePageValue, ToCodePageValue, resultCode>>(
						        result));
					}
				});
			}
		}

		template <std::size_t Extent, typename OutputReceiver>
		static constexpr void EncodeAll(std::span<const CharType, Extent> const& span,
		                                OutputReceiver&& receiver)
		{
			std::span<const CharType> remainedSpan = span;
			while (!remainedSpan.empty())
			{
				const auto encodeUnit = [&]() constexpr
				{
					if constexpr (FromCodePageTrait::IsVariableWidth)
					{
						return remainedSpan;
					}
					else
					{
						return remainedSpan[0];
					}
				}
				();
				Encode(encodeUnit, [&](auto const& result) {
					if constexpr (GetEncodingResultCode<decltype(result)> == EncodingResultCode::Accept)
					{
						remainedSpan = remainedSpan.subspan(result.AdvanceCount);
					}
					else
					{
						// 出现错误时终止循环
						remainedSpan = std::span<const CharType>{};
					}

					std::forward<OutputReceiver>(receiver)(result);
				});
			}
		}
	};

	/// @brief  编码器
	/// @remark 使用类模板是为了允许可能的特化优化版本，继承是为了允许只重写 EncodeOne
	///         版本，也允许全部重写以获得最佳性能
	template <CodePage::CodePageType FromCodePageValue, CodePage::CodePageType ToCodePageValue>
	struct Encoder : EncoderBase<FromCodePageValue, ToCodePageValue>
	{
	};

	/// @brief  计算从 FromCodePageValue 的字符串编码到 ToCodePageValue 的长度
	/// @return 结束时编码的结果以及编码的长度（以 ToCodePageValue 的编码单元个数计算）
	/// @remark 若 span 为空，编码视为成功，返回大小为 0
	template <CodePage::CodePageType FromCodePageValue, CodePage::CodePageType ToCodePageValue>
	constexpr std::pair<EncodingResultCode, std::size_t> CountEncodeSize(
	    std::span<const typename CodePage::CodePageTrait<FromCodePageValue>::CharType> const&
	        span) noexcept
	{
		auto resultCode = EncodingResultCode::Accept;
		std::size_t count{};
		Encoder<FromCodePageValue, ToCodePageValue>::EncodeAll(span, [&](auto const& result) {
			if constexpr (GetEncodingResultCode<decltype(result)> == EncodingResultCode::Accept)
			{
				if constexpr (CodePage::CodePageTrait<ToCodePageValue>::IsVariableWidth)
				{
					count += result.Result.size();
				}
				else
				{
					++count;
				}
			}
			else
			{
				resultCode = GetEncodingResultCode<decltype(result)>;
			}
		});
		return { resultCode, count };
	}

	/// @brief  表示码点的信息
	template <>
	struct CodePage::CodePageTrait<CodePage::CodePoint>
	{
		/// @brief  代码页名称
		/// @note   使用 ASCII 表示
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
