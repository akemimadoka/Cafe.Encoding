#pragma once

#include <Encoding/Encode.h>

namespace Cafe::Encoding
{
	namespace CodePage
	{
		constexpr CodePageType Utf32LittleEndian = static_cast<CodePageType>(12000);
		constexpr CodePageType Utf32BigEndian = static_cast<CodePageType>(12001);

		namespace Detail
		{
			template <CodePageType Utf32>
			struct Utf32CommonPart
			{
			private:
				static constexpr bool IsLittleEndian = []() constexpr
				{
#ifdef _MSC_VER
					return true;
#elif __cplusplus > 201709L
					static_assert(std::endian::native == std::endian::little ||
					              std::endian::native == std::endian::big);
					return std::endian::native == std::endian::little;
#else
					return __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__;
#endif
				}
				();

				static constexpr bool NeedSwapEndian = (Utf32 == Utf32BigEndian) == IsLittleEndian;

				[[nodiscard]] static constexpr char32_t MaySwapEndian(char32_t value) noexcept
				{
					if constexpr (NeedSwapEndian)
					{
						return value << 24 | ((value & 0x0000FF00) << 8) |
						       ((value & 0x00FF0000) >> 8 | ((value & 0xFF000000) >> 24));
					}
					else
					{
						return value;
					}
				}

			public:
				using CharType = char32_t;

				static constexpr bool IsVariableWidth = false;

				template <std::ptrdiff_t Extent, typename OutputReceiver>
				static constexpr void ToCodePoint(CharType codeUnit, OutputReceiver&& receiver)
				{
					std::forward<OutputReceiver>(receiver)(
					    EncodingResult<Utf32, CodePoint, EncodingResultCode::Accept>{
					        MaySwapEndian(static_cast<CharType>(codeUnit)) });
				}

				template <typename OutputReceiver>
				static constexpr void FromCodePoint(CodePointType codePoint, OutputReceiver&& receiver)
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
} // namespace Cafe::Encoding
