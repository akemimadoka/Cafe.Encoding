#pragma once

#include <Encoding/Encode.h>

namespace Cafe::Encoding
{
	namespace CodePage
	{
		constexpr CodePageType Utf8 = static_cast<CodePageType>(65001);

		template <>
		struct CodePageTrait<CodePage::Utf8>
		{
			static constexpr const char Name[] = "UTF-8";

			// 表示一个编码单元的类型，为了好看名称仍用 CharType，但 CharType 不表示字符
#if __cpp_char8_t == 201811L
			using CharType = char8_t;
#else
			using CharType = char;
#endif

			// 编码是否是变长的
			static constexpr bool IsVariableWidth = true;

			// 定长编码无此方法
			// 0 表示错误，负数的绝对值表示若需确定至少需要前移并重新测试的个数
			[[nodiscard]] static constexpr std::ptrdiff_t GetWidth(CharType value) noexcept
			{
				const auto codeValue = static_cast<std::make_unsigned_t<CharType>>(value);
				// 0xxxxxxx：单个编码单元
				if (!(codeValue & 0x80))
				{
					return 1;
				}

				// 10xxxxxx：中间的编码，需前移至少 1 个编码单元
				if ((codeValue & 0xC0) == 0x80)
				{
					return -1;
				}

				// 110xxxxx：2 个编码单元的首位
				if ((codeValue & 0xE0) == 0xC0)
				{
					return 2;
				}

				// 1110xxxx：3 个编码单元的首位
				// 注：0xD800-0xDFFF 之间不存在任何字符，但此时无法判断
				if ((codeValue & 0xF0) == 0xE0)
				{
					return 3;
				}

				// 11110xxx：4 个编码单元的首位
				// 0x10FFFF 码点之后无字符，因此只有首字节不大于 0xF4 才可行
				if ((codeValue & 0xF8) == 0xF0 && codeValue <= 0xF4)
				{
					return 4;
				}

				// RFC 3629 禁止更长的编码
				return 0;
			}

			// 若是变长编码则接受 span，否则是单个编码单元
			template <std::ptrdiff_t Extent, typename OutputReceiver>
			static constexpr void ToCodePoint(gsl::span<const CharType, Extent> const& span,
			                                  OutputReceiver&& receiver)
			{
				const auto size = span.size();
				if (size <= 0)
				{
					std::forward<OutputReceiver>(receiver)(
					    EncodingResult<Utf8, CodePoint, EncodingResultCode::Incomplete>{ 0, 1 });
					return;
				}

				const auto codeValue = static_cast<std::make_unsigned_t<CharType>>(span[0]);

				if (!(codeValue & 0x80))
				{
					const auto result = static_cast<CodePointType>(codeValue);

					std::forward<OutputReceiver>(receiver)(
					    EncodingResult<Utf8, CodePoint, EncodingResultCode::Accept>{ result, 1 });
				}
				else if ((codeValue & 0xC0) == 0x80)
				{
					std::forward<OutputReceiver>(receiver)(
					    EncodingResult<Utf8, CodePoint, EncodingResultCode::Incomplete>{ -1 });
				}
				else if ((codeValue & 0xE0) == 0xC0)
				{
					if (size < 2)
					{
						std::forward<OutputReceiver>(receiver)(
						    EncodingResult<Utf8, CodePoint, EncodingResultCode::Incomplete>{ 0, 2 - size });
						return;
					}

					const auto secondCodeUnit = static_cast<std::make_unsigned_t<CharType>>(span[1]);
					const auto result =
					    static_cast<CodePointType>(((codeValue & 0x1F) << 6) + (secondCodeUnit & 0x3F));

					std::forward<OutputReceiver>(receiver)(
					    EncodingResult<Utf8, CodePoint, EncodingResultCode::Accept>{ result, 2 });
				}
				else if ((codeValue & 0xF0) == 0xE0)
				{
					if (size < 3)
					{
						std::forward<OutputReceiver>(receiver)(
						    EncodingResult<Utf8, CodePoint, EncodingResultCode::Incomplete>{ 0, 3 - size });
						return;
					}

					const auto secondCodeUnit = static_cast<std::make_unsigned_t<CharType>>(span[1]);
					const auto thirdCodeUnit = static_cast<std::make_unsigned_t<CharType>>(span[2]);
					const auto result = static_cast<CodePointType>(
					    ((codeValue & 0x0F) << 12) + ((secondCodeUnit & 0x3F) << 6) + (thirdCodeUnit & 0x3F));

					if (result >= 0x00D800 && result <= 0x00DFFF)
					{
						// 这不是一个仅限 UTF-8 会有的情况，是否可以通用地处理？
						std::forward<OutputReceiver>(receiver)(
						    EncodingResult<Utf8, CodePoint, EncodingResultCode::Reject>{});
					}
					else
					{
						std::forward<OutputReceiver>(receiver)(
						    EncodingResult<Utf8, CodePoint, EncodingResultCode::Accept>{ result, 3 });
					}
				}
				else if ((codeValue & 0xF8) == 0xF0 && codeValue <= 0xF4)
				{
					if (size < 4)
					{
						std::forward<OutputReceiver>(receiver)(
						    EncodingResult<Utf8, CodePoint, EncodingResultCode::Incomplete>{ 0, 4 - size });
						return;
					}

					const auto secondCodeUnit = static_cast<std::make_unsigned_t<CharType>>(span[1]);
					const auto thirdCodeUnit = static_cast<std::make_unsigned_t<CharType>>(span[2]);
					const auto fourthCodeUnit = static_cast<std::make_unsigned_t<CharType>>(span[3]);
					const auto result = static_cast<CodePointType>(
					    ((codeValue & 0x07) << 18) + ((secondCodeUnit & 0x3F) << 12) +
					    ((thirdCodeUnit & 0x3F) << 6) + (fourthCodeUnit & 0x3F));

					std::forward<OutputReceiver>(receiver)(
					    EncodingResult<Utf8, CodePoint, EncodingResultCode::Accept>{ result, 4 });
				}
				else
				{
					std::forward<OutputReceiver>(receiver)(
					    EncodingResult<Utf8, CodePoint, EncodingResultCode::Reject>{});
				}
			}

			template <typename OutputReceiver>
			static constexpr void FromCodePoint(CodePointType codePoint, OutputReceiver&& receiver)
			{
				if (codePoint <= 0x00007F)
				{
					const CharType result[]{ static_cast<CharType>(codePoint) };
					std::forward<OutputReceiver>(receiver)(
					    EncodingResult<CodePoint, Utf8, EncodingResultCode::Accept>{
					        gsl::make_span(result) });
				}
				else if (codePoint <= 0x0007FF)
				{
					const CharType result[]{ static_cast<CharType>(0xC0 + ((codePoint & 0x0007C0) >> 6)),
						                       static_cast<CharType>(0x80 + (codePoint & 0x00003F)) };
					std::forward<OutputReceiver>(receiver)(
					    EncodingResult<CodePoint, Utf8, EncodingResultCode::Accept>{
					        gsl::make_span(result) });
				}
				else if (codePoint <= 0x00D7FF || (codePoint >= 0x00E000 && codePoint <= 0x00FFFF))
				{
					const CharType result[]{ static_cast<CharType>(0xE0 + ((codePoint & 0x00F000) >> 12)),
						                       static_cast<CharType>(0x80 + ((codePoint & 0x000FC0) >> 6)),
						                       static_cast<CharType>(0x80 + (codePoint & 0x00003F)) };
					std::forward<OutputReceiver>(receiver)(
					    EncodingResult<CodePoint, Utf8, EncodingResultCode::Accept>{
					        gsl::make_span(result) });
				}
				else if (codePoint >= 0x010000 && codePoint <= 0x10FFFF)
				{
					const CharType result[]{ static_cast<CharType>(0xF0 + ((codePoint & 0x1C0000) >> 18)),
						                       static_cast<CharType>(0x80 + ((codePoint & 0x03F000) >> 12)),
						                       static_cast<CharType>(0x80 + ((codePoint & 0x000FC0) >> 6)),
						                       static_cast<CharType>(0x80 + (codePoint & 0x00003F)) };
					std::forward<OutputReceiver>(receiver)(
					    EncodingResult<CodePoint, Utf8, EncodingResultCode::Accept>{
					        gsl::make_span(result) });
				}
				else
				{
					std::forward<OutputReceiver>(receiver)(
					    EncodingResult<CodePoint, Utf8, EncodingResultCode::Reject>{});
				}
			}
		};
	} // namespace CodePage
} // namespace Cafe::Encoding
