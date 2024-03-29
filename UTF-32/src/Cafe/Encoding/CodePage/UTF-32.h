#ifndef CAFE_ENCODING_CODEPAGE_UTF32_H
#define CAFE_ENCODING_CODEPAGE_UTF32_H

#include <Cafe/Encoding/Encode.h>
#include <Cafe/Encoding/Strings.h>
#include <Cafe/Misc/Utility.h>

namespace Cafe::Encoding
{
	namespace CodePage
	{
		constexpr CodePageType Utf32LittleEndian = static_cast<CodePageType>(12000);
		constexpr CodePageType Utf32BigEndian = static_cast<CodePageType>(12001);

		namespace Detail
		{
			static_assert(std::endian::native == std::endian::little ||
			              std::endian::native == std::endian::big);

			template <CodePageType Utf32>
			struct Utf32CommonPart
			{
			private:
				static constexpr bool NeedSwapEndian =
				    (Utf32 == Utf32BigEndian) == (std::endian::native == std::endian::little);

				/// @remark 假设了平台并非是混合端序的
				[[nodiscard]] static constexpr char32_t MaySwapEndian(char32_t value) noexcept
				{
					if constexpr (NeedSwapEndian)
					{
#if defined(_MSC_VER)
						return _byteswap_ulong(value);
#elif defined(__GNUC__)
						return __builtin_bswap32(value);
#else
						return value << 24 | ((value & 0x0000FF00) << 8) |
						       ((value & 0x00FF0000) >> 8 | ((value & 0xFF000000) >> 24));
#endif
					}
					else
					{
						return value;
					}
				}

			public:
				using CharType = char32_t;

				static constexpr bool IsVariableWidth = false;

				template <typename OutputReceiver>
				static constexpr void ToCodePoint(CharType codeUnit, OutputReceiver&& receiver)
				{
					std::forward<OutputReceiver>(receiver)(
					    EncodingResult<Utf32, CodePoint, EncodingResultCode::Accept>{
					        MaySwapEndian(static_cast<CharType>(codeUnit)) });
				}

				template <typename OutputReceiver>
				static constexpr void FromCodePoint(CodePointType codePoint,
				                                    OutputReceiver&& receiver)
				{
					std::forward<OutputReceiver>(receiver)(
					    EncodingResult<CodePoint, Utf32, EncodingResultCode::Accept>{
					        static_cast<CodePointType>(MaySwapEndian(codePoint)) });
				}
			};
		} // namespace Detail

		template <>
		struct CodePageTrait<CodePage::Utf32LittleEndian>
		    : Detail::Utf32CommonPart<CodePage::Utf32LittleEndian>
		{
			static constexpr const char Name[] = "UTF-32 LE";
		};

		template <>
		struct CodePageTrait<CodePage::Utf32BigEndian>
		    : Detail::Utf32CommonPart<CodePage::Utf32BigEndian>
		{
			static constexpr const char Name[] = "UTF-32 BE";
		};
	} // namespace CodePage

	inline namespace StringLiterals
	{
		constexpr StringView<CodePage::Utf32LittleEndian> operator""_lesv(
		    const typename CodePage::CodePageTrait<CodePage::Utf32LittleEndian>::CharType* str,
		    std::size_t size) noexcept
		{
			return std::span(str, size + 1);
		}

		inline String<CodePage::Utf32LittleEndian> operator""_les(
		    const typename CodePage::CodePageTrait<CodePage::Utf32LittleEndian>::CharType* str,
		    std::size_t size) noexcept
		{
			return String<CodePage::Utf32LittleEndian>{ std::span(str, size + 1) };
		}

		constexpr StringView<CodePage::Utf32BigEndian> operator""_besv(
		    const typename CodePage::CodePageTrait<CodePage::Utf32BigEndian>::CharType* str,
		    std::size_t size) noexcept
		{
			return std::span(str, size + 1);
		}

		inline String<CodePage::Utf32BigEndian> operator""_bes(
		    const typename CodePage::CodePageTrait<CodePage::Utf32BigEndian>::CharType* str,
		    std::size_t size) noexcept
		{
			return String<CodePage::Utf32BigEndian>{ std::span(str, size + 1) };
		}

#if __cpp_nontype_template_args >= 201911L
		template <Core::Misc::ArrayBinder<typename CodePage::CodePageTrait<
		    std::endian::native == std::endian::little
		        ? CodePage::Utf32LittleEndian
		        : CodePage::Utf32BigEndian>::CharType>::Result Array>
		constexpr StringView<std::endian::native == std::endian::little
		                         ? CodePage::Utf32LittleEndian
		                         : CodePage::Utf32BigEndian,
		                     Array.Size>
		operator""_sv() noexcept
		{
			return std::span(Array.Content);
		}
#else
		constexpr StringView<std::endian::native == std::endian::little
		                         ? CodePage::Utf32LittleEndian
		                         : CodePage::Utf32BigEndian>
		operator""_sv(
		    const typename CodePage::CodePageTrait<std::endian::native == std::endian::little
		                                               ? CodePage::Utf32LittleEndian
		                                               : CodePage::Utf32BigEndian>::CharType* str,
		    std::size_t size) noexcept
		{
			return std::span(str, size + 1);
		}
#endif

		inline String<std::endian::native == std::endian::little ? CodePage::Utf32LittleEndian
		                                                         : CodePage::Utf32BigEndian>
		operator""_s(
		    const typename CodePage::CodePageTrait<std::endian::native == std::endian::little
		                                               ? CodePage::Utf32LittleEndian
		                                               : CodePage::Utf32BigEndian>::CharType* str,
		    std::size_t size) noexcept
		{
			return String < std::endian::native == std::endian::little
			           ? CodePage::Utf32LittleEndian
			           : CodePage::Utf32BigEndian > { std::span(str, size + 1) };
		}
	} // namespace StringLiterals
} // namespace Cafe::Encoding

#endif

#ifdef CAFE_CODEPAGE
CAFE_CODEPAGE(::Cafe::Encoding::CodePage::Utf32LittleEndian)
CAFE_CODEPAGE(::Cafe::Encoding::CodePage::Utf32BigEndian)
#endif
