#ifndef CAFE_ENCODING_CODEPAGE_UTF16_H
#define CAFE_ENCODING_CODEPAGE_UTF16_H

#include <Cafe/Encoding/Encode.h>
#include <Cafe/Encoding/Strings.h>
#include <Cafe/Misc/Utility.h>
#include <cassert>

namespace Cafe::Encoding
{
	namespace CodePage
	{
		constexpr CodePageType Utf16LittleEndian = static_cast<CodePageType>(1200);
		constexpr CodePageType Utf16BigEndian = static_cast<CodePageType>(1201);

		namespace Detail
		{
			static_assert(std::endian::native == std::endian::little ||
			              std::endian::native == std::endian::big);

			template <CodePageType Utf16>
			struct Utf16CommonPart
			{
			private:
				static constexpr bool NeedSwapEndian =
				    (Utf16 == Utf16BigEndian) == (std::endian::native == std::endian::little);

				[[nodiscard]] static constexpr char16_t MaySwapEndian(char16_t value) noexcept
				{
					if constexpr (NeedSwapEndian)
					{
#if defined(_MSC_VER)
						return _byteswap_ushort(value);
#elif defined(__GNUC__)
						return __builtin_bswap16(value);
#else
						return value << 8 | ((value & 0xFF00) >> 8);
#endif
					}
					else
					{
						return value;
					}
				}

			public:
				using CharType = char16_t;
				static constexpr bool IsVariableWidth = true;
				static constexpr std::size_t MaxWidth = 2;

				[[nodiscard]] static constexpr std::ptrdiff_t GetWidth(CharType value) noexcept
				{
					value = MaySwapEndian(value);
					if (value >= 0xD800 && value < 0xDC00)
					{
						// 是前导代理
						return 2;
					}
					else if (value >= 0xDC00 && value <= 0xDFFF)
					{
						// 是后尾代理
						return -1;
					}
					return 1;
				}

				template <std::size_t Extent, typename OutputReceiver>
				static constexpr void ToCodePoint(std::span<const CharType, Extent> const& span,
				                                  OutputReceiver&& receiver)
				{
					if (span.empty())
					{
						std::forward<OutputReceiver>(receiver)(
						    EncodingResult<Utf16, CodePoint, EncodingResultCode::Incomplete>{ 0,
						                                                                      1 });
						return;
					}

					// 此时不需要交换端序
					const auto mayBeLeadSurrogate = span[0];
					switch (GetWidth(mayBeLeadSurrogate))
					{
					default:
						assert(!"Should never happen.");
						std::forward<OutputReceiver>(receiver)(
						    EncodingResult<Utf16, CodePoint, EncodingResultCode::Reject>{});
						break;
					case -1:
						std::forward<OutputReceiver>(receiver)(
						    EncodingResult<Utf16, CodePoint, EncodingResultCode::Incomplete>{ -1 });
						break;
					case 1: {
						const auto result =
						    static_cast<CodePointType>(MaySwapEndian(mayBeLeadSurrogate));
						std::forward<OutputReceiver>(receiver)(
						    EncodingResult<Utf16, CodePoint, EncodingResultCode::Accept>{ result,
						                                                                  1u });
						break;
					}
					case 2: {
						if (span.size() < 2)
						{
							std::forward<OutputReceiver>(receiver)(
							    EncodingResult<Utf16, CodePoint, EncodingResultCode::Incomplete>{
							        0, 1 });
							return;
						}

						const auto leadSurrogate =
						    static_cast<CodePointType>(MaySwapEndian(mayBeLeadSurrogate));
						const auto trailSurrogate =
						    static_cast<CodePointType>(MaySwapEndian(span[1]));

						const auto result = static_cast<CodePointType>(
						    (((leadSurrogate - 0xD800) << 10) + (trailSurrogate - 0xDC00)) +
						    0x10000);

						std::forward<OutputReceiver>(receiver)(
						    EncodingResult<Utf16, CodePoint, EncodingResultCode::Accept>{ result,
						                                                                  2u });
						break;
					}
					}
				}

				template <typename OutputReceiver>
				static constexpr void FromCodePoint(CodePointType codePoint,
				                                    OutputReceiver&& receiver)
				{
					if (codePoint >= 0xD800 && codePoint <= 0xDFFF)
					{
						std::forward<OutputReceiver>(receiver)(
						    EncodingResult<CodePoint, Utf16, EncodingResultCode::Reject>{});
					}
					else if (codePoint <= 0xFFFF)
					{
						const CharType result[] = { MaySwapEndian(
							static_cast<CharType>(codePoint)) };
						std::forward<OutputReceiver>(receiver)(
						    EncodingResult<CodePoint, Utf16, EncodingResultCode::Accept>{
						        std::span(result) });
					}
					else
					{
						const auto processedCodePoint = codePoint - 0x10000;
						const CharType result[] = {
							MaySwapEndian(((processedCodePoint & 0b11111111110000000000) >> 10) +
							              0xD800),
							MaySwapEndian((processedCodePoint & 0b00000000001111111111) + 0xDC00)
						};
						std::forward<OutputReceiver>(receiver)(
						    EncodingResult<CodePoint, Utf16, EncodingResultCode::Accept>{
						        std::span(result) });
					}
				}
			};
		} // namespace Detail

		template <>
		struct CodePageTrait<CodePage::Utf16LittleEndian>
		    : Detail::Utf16CommonPart<CodePage::Utf16LittleEndian>
		{
			static constexpr const char Name[] = "UTF-16 LE";
		};

		template <>
		struct CodePageTrait<CodePage::Utf16BigEndian>
		    : Detail::Utf16CommonPart<CodePage::Utf16BigEndian>
		{
			static constexpr const char Name[] = "UTF-16 BE";
		};
	} // namespace CodePage

	inline namespace StringLiterals
	{
		constexpr StringView<CodePage::Utf16LittleEndian> operator""_lesv(
		    const typename CodePage::CodePageTrait<CodePage::Utf16LittleEndian>::CharType* str,
		    std::size_t size) noexcept
		{
			return std::span(str, size + 1);
		}

		inline String<CodePage::Utf16LittleEndian> operator""_les(
		    const typename CodePage::CodePageTrait<CodePage::Utf16LittleEndian>::CharType* str,
		    std::size_t size) noexcept
		{
			return String<CodePage::Utf16LittleEndian>{ std::span(str, size + 1) };
		}

		constexpr StringView<CodePage::Utf16BigEndian> operator""_besv(
		    const typename CodePage::CodePageTrait<CodePage::Utf16BigEndian>::CharType* str,
		    std::size_t size) noexcept
		{
			return std::span(str, size + 1);
		}

		inline String<CodePage::Utf16BigEndian> operator""_bes(
		    const typename CodePage::CodePageTrait<CodePage::Utf16BigEndian>::CharType* str,
		    std::size_t size) noexcept
		{
			return String<CodePage::Utf16BigEndian>{ std::span(str, size + 1) };
		}

#if __cpp_nontype_template_args >= 201911L
		template <Core::Misc::ArrayBinder<typename CodePage::CodePageTrait<
		    std::endian::native == std::endian::little
		        ? CodePage::Utf16LittleEndian
		        : CodePage::Utf16BigEndian>::CharType>::Result Array>
		constexpr StringView<std::endian::native == std::endian::little
		                         ? CodePage::Utf16LittleEndian
		                         : CodePage::Utf16BigEndian,
		                     Array.Size>
		operator""_sv() noexcept
		{
			return std::span(Array.Content);
		}
#else
		constexpr StringView<std::endian::native == std::endian::little
		                         ? CodePage::Utf16LittleEndian
		                         : CodePage::Utf16BigEndian>
		operator""_sv(
		    const typename CodePage::CodePageTrait<std::endian::native == std::endian::little
		                                               ? CodePage::Utf16LittleEndian
		                                               : CodePage::Utf16BigEndian>::CharType* str,
		    std::size_t size) noexcept
		{
			return std::span(str, size + 1);
		}
#endif

		inline String<std::endian::native == std::endian::little ? CodePage::Utf16LittleEndian
		                                                         : CodePage::Utf16BigEndian>
		operator""_s(
		    const typename CodePage::CodePageTrait<std::endian::native == std::endian::little
		                                               ? CodePage::Utf16LittleEndian
		                                               : CodePage::Utf16BigEndian>::CharType* str,
		    std::size_t size) noexcept
		{
			return String < std::endian::native == std::endian::little
			           ? CodePage::Utf16LittleEndian
			           : CodePage::Utf16BigEndian > { std::span(str, size + 1) };
		}
	} // namespace StringLiterals
} // namespace Cafe::Encoding

#endif

#ifdef CAFE_CODEPAGE
CAFE_CODEPAGE(::Cafe::Encoding::CodePage::Utf16LittleEndian)
CAFE_CODEPAGE(::Cafe::Encoding::CodePage::Utf16BigEndian)
#endif
