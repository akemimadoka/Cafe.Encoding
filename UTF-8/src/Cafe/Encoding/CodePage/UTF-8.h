#ifndef CAFE_ENCODING_CODEPAGE_UTF8_H
#define CAFE_ENCODING_CODEPAGE_UTF8_H

#include <Cafe/Encoding/Encode.h>
#include <Cafe/Encoding/Strings.h>

namespace Cafe::Encoding
{
	namespace CodePage
	{
		constexpr CodePageType Utf8 = static_cast<CodePageType>(65001);

		template <>
		struct CodePageTrait<CodePage::Utf8>
		{
			static constexpr const char Name[] = "UTF-8";

			/// @brief  表示一个编码单元的类型，为了好看名称仍用 CharType，但 CharType 不表示字符
#if __cpp_char8_t >= 201811L
			using CharType = char8_t;
#else
			using CharType = char;
#endif

			/// @brief  编码是否是变长的
			static constexpr bool IsVariableWidth = true;

			/// @brief  能完整组成一个码点表示的最大编码单元个数，定长编码无此字段
			static constexpr std::size_t MaxWidth = 4;

			/// @brief  获得指定编码单元组成的码点的对应宽度
			/// @remark 定长编码无此方法
			///         0 表示错误，负数的绝对值表示若需确定至少需要前移并重新测试的个数
			///         在特殊情况下 GetWidth 可能无法获得长度，这不意味着 ToCodePoint 必然也失败
			///         请查阅对应的特化版本的文档以决定是否考虑此类情况，UTF-8 不会存在此情况
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

			/// @brief  由编码单元转换为码点
			/// @param  span        编码单元
			/// @param  receiver    结果的接收器
			/// @remark 若是变长编码则接受 span，否则是单个编码单元
			///         receiver 应可接受 EncodingResult 中 FromCodePage 是当前代码页，
			///         ToCodePage 是 CodePoint，所有结果代码的实例
			template <std::size_t Extent, typename OutputReceiver>
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
					    EncodingResult<Utf8, CodePoint, EncodingResultCode::Accept>{ result, 1u });
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
						    EncodingResult<Utf8, CodePoint, EncodingResultCode::Incomplete>{
						        0, static_cast<std::size_t>(2 - size) });
						return;
					}

					const auto secondCodeUnit = static_cast<std::make_unsigned_t<CharType>>(span[1]);
					const auto result =
					    static_cast<CodePointType>(((codeValue & 0x1F) << 6) + (secondCodeUnit & 0x3F));

					std::forward<OutputReceiver>(receiver)(
					    EncodingResult<Utf8, CodePoint, EncodingResultCode::Accept>{ result, 2u });
				}
				else if ((codeValue & 0xF0) == 0xE0)
				{
					if (size < 3)
					{
						std::forward<OutputReceiver>(receiver)(
						    EncodingResult<Utf8, CodePoint, EncodingResultCode::Incomplete>{
						        0, static_cast<std::size_t>(3 - size) });
						return;
					}

					const auto secondCodeUnit = static_cast<std::make_unsigned_t<CharType>>(span[1]);
					const auto thirdCodeUnit = static_cast<std::make_unsigned_t<CharType>>(span[2]);
					const auto result = static_cast<CodePointType>(
					    ((codeValue & 0x0F) << 12) + ((secondCodeUnit & 0x3F) << 6) + (thirdCodeUnit & 0x3F));

					if (result >= 0xD800 && result <= 0xDFFF)
					{
						// 这不是一个仅限 UTF-8 会有的情况，是否可以通用地处理？
						std::forward<OutputReceiver>(receiver)(
						    EncodingResult<Utf8, CodePoint, EncodingResultCode::Reject>{});
					}
					else
					{
						std::forward<OutputReceiver>(receiver)(
						    EncodingResult<Utf8, CodePoint, EncodingResultCode::Accept>{ result, 3u });
					}
				}
				else if ((codeValue & 0xF8) == 0xF0 && codeValue <= 0xF4)
				{
					if (size < 4)
					{
						std::forward<OutputReceiver>(receiver)(
						    EncodingResult<Utf8, CodePoint, EncodingResultCode::Incomplete>{
						        0, static_cast<std::size_t>(4 - size) });
						return;
					}

					const auto secondCodeUnit = static_cast<std::make_unsigned_t<CharType>>(span[1]);
					const auto thirdCodeUnit = static_cast<std::make_unsigned_t<CharType>>(span[2]);
					const auto fourthCodeUnit = static_cast<std::make_unsigned_t<CharType>>(span[3]);
					const auto result = static_cast<CodePointType>(
					    ((codeValue & 0x07) << 18) + ((secondCodeUnit & 0x3F) << 12) +
					    ((thirdCodeUnit & 0x3F) << 6) + (fourthCodeUnit & 0x3F));

					std::forward<OutputReceiver>(receiver)(
					    EncodingResult<Utf8, CodePoint, EncodingResultCode::Accept>{ result, 4u });
				}
				else
				{
					std::forward<OutputReceiver>(receiver)(
					    EncodingResult<Utf8, CodePoint, EncodingResultCode::Reject>{});
				}
			}

			/// @brief  从码点转换为编码单元
			/// @remark receiver 应可接受 EncodingResult 中 FromCodePage 是 CodePoint，
			///         ToCodePage 是当前代码页，所有结果代码的实例
			template <typename OutputReceiver>
			static constexpr void FromCodePoint(CodePointType codePoint, OutputReceiver&& receiver)
			{
				if (codePoint <= 0x00007F)
				{
					const CharType result[]{ static_cast<CharType>(codePoint) };
					std::forward<OutputReceiver>(receiver)(
					    EncodingResult<CodePoint, Utf8, EncodingResultCode::Accept>{
					        gsl::span(result) });
				}
				else if (codePoint <= 0x0007FF)
				{
					const CharType result[]{ static_cast<CharType>(0xC0 + ((codePoint & 0x0007C0) >> 6)),
						                       static_cast<CharType>(0x80 + (codePoint & 0x00003F)) };
					std::forward<OutputReceiver>(receiver)(
					    EncodingResult<CodePoint, Utf8, EncodingResultCode::Accept>{
					        gsl::span(result) });
				}
				else if (codePoint <= 0x00D7FF || (codePoint >= 0x00E000 && codePoint <= 0x00FFFF))
				{
					const CharType result[]{ static_cast<CharType>(0xE0 + ((codePoint & 0x00F000) >> 12)),
						                       static_cast<CharType>(0x80 + ((codePoint & 0x000FC0) >> 6)),
						                       static_cast<CharType>(0x80 + (codePoint & 0x00003F)) };
					std::forward<OutputReceiver>(receiver)(
					    EncodingResult<CodePoint, Utf8, EncodingResultCode::Accept>{
					        gsl::span(result) });
				}
				else if (codePoint >= 0x010000 && codePoint <= 0x10FFFF)
				{
					const CharType result[]{ static_cast<CharType>(0xF0 + ((codePoint & 0x1C0000) >> 18)),
						                       static_cast<CharType>(0x80 + ((codePoint & 0x03F000) >> 12)),
						                       static_cast<CharType>(0x80 + ((codePoint & 0x000FC0) >> 6)),
						                       static_cast<CharType>(0x80 + (codePoint & 0x00003F)) };
					std::forward<OutputReceiver>(receiver)(
					    EncodingResult<CodePoint, Utf8, EncodingResultCode::Accept>{
					        gsl::span(result) });
				}
				else
				{
					std::forward<OutputReceiver>(receiver)(
					    EncodingResult<CodePoint, Utf8, EncodingResultCode::Reject>{});
				}
			}
		};
	} // namespace CodePage

#if __cpp_lib_is_constant_evaluated >= 201811L
	template <CodePage::CodePageType ToCodePageValue>
	struct Encoder<CodePage::Utf8, ToCodePageValue>
	{
		using Trait = CodePage::CodePageTrait<CodePage::Utf8>;
		using CharType = typename Trait::CharType;

		template <std::size_t Extent, typename OutputReceiver>
		static constexpr void EncodeAll(gsl::span<const CharType, Extent> const& span,
		                                OutputReceiver&& receiver)
		{
			if (std::is_constant_evaluated())
			{
				return EncoderBase<CodePage::Utf8, ToCodePageValue>::EncodeAll(
				    span, std::forward<OutputReceiver>(receiver));
			}
			else
			{
				// TODO: 针对运行期优化，使用 SIMD
				return EncoderBase<CodePage::Utf8, ToCodePageValue>::EncodeAll(
				    span, std::forward<OutputReceiver>(receiver));
			}
		}
	};
#endif

	inline namespace StringLiterals
	{
		// 字符串自定义字面量的 size 参数不包含结尾的空字符
		// TODO: 考虑使用模板以推断长度信息

#if __cpp_char8_t >= 201811L
		static_assert(std::is_same_v<CodePage::CodePageTrait<CodePage::Utf8>::CharType, char8_t>);

		constexpr StringView<CodePage::Utf8> operator""_sv(const char8_t* str,
		                                                   std::size_t size) noexcept
		{
			return gsl::span(str, size + 1);
		}

		inline String<CodePage::Utf8> operator""_s(const char8_t* str, std::size_t size)
		{
			return String<CodePage::Utf8>{ gsl::span(str, size + 1) };
		}
#endif

		constexpr StringView<CodePage::Utf8>
		operator""_u8sv(const CodePage::CodePageTrait<CodePage::Utf8>::CharType* str,
		                std::size_t size) noexcept
		{
			return gsl::span(str, size + 1);
		}

		inline String<CodePage::Utf8>
		operator""_u8s(const CodePage::CodePageTrait<CodePage::Utf8>::CharType* str, std::size_t size)
		{
			return String<CodePage::Utf8>{ gsl::span(str, size + 1) };
		}
	} // namespace StringLiterals
} // namespace Cafe::Encoding

#define CAFE_UTF8_IMPL(str) u8##str
#define CAFE_UTF8(str) CAFE_UTF8_IMPL(str)
#define CAFE_UTF8_SV(str)                                                                          \
	::Cafe::Encoding::StringView<::Cafe::Encoding::CodePage::Utf8, std::size(CAFE_UTF8(str))>        \
	{                                                                                                \
		CAFE_UTF8(str)                                                                                 \
	}

#endif

#ifdef CAFE_CODEPAGE
CAFE_CODEPAGE(::Cafe::Encoding::CodePage::Utf8)
#endif
