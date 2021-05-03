#ifndef CAFE_ENCODING_CODEPAGE_GB2312
#define CAFE_ENCODING_CODEPAGE_GB2312

#include <Cafe/Encoding/Encode.h>
#include <Cafe/Encoding/Strings.h>
#include <iterator>

namespace Cafe::Encoding
{
	namespace CodePage
	{
		/// @brief  表示以 EUC-CN 方式表示的 GB2312 编码
		/// @see    https://en.wikipedia.org/wiki/Extended_Unix_Code
		constexpr CodePageType GB2312 = static_cast<CodePageType>(936);

		template <>
		struct CodePageTrait<GB2312>
		{
		private:
			// index 是编码单元组合减去 0x8140
			static constexpr auto GB2312ToUnicodeMapping = []() constexpr
			{
				std::array<std::uint16_t, 0x7D10> result{};
#define GB2312_PAIR(gb2312, unicode) result[gb2312 - 0x8140] = unicode;
#include "Impl/GB2312Impl.h"
				return result;
			}
			();

			static constexpr auto UnicodeToGB2312Mapping = []() constexpr
			{
				std::array<std::uint16_t, 0xFFFF> result{};
#define GB2312_PAIR(gb2312, unicode) result[unicode] = gb2312;
#include "Impl/GB2312Impl.h"
				return result;
			}
			();

		public:
			static constexpr const char Name[] = "GB2312";

			using CharType = char;

			static constexpr bool IsVariableWidth = true;

			static constexpr std::size_t MaxWidth = 2;

			[[nodiscard]] static constexpr std::ptrdiff_t GetWidth(CharType value) noexcept
			{
				const auto charValue = static_cast<unsigned>(value);
				if (charValue <= 0x80)
				{
					return 1;
				}

				return 2;
			}

			template <std::size_t Extent, typename OutputReceiver>
			static constexpr void ToCodePoint(std::span<const CharType, Extent> const& span,
			                                  OutputReceiver&& receiver)
			{
				if (span.empty())
				{
					std::forward<OutputReceiver>(receiver)(
					    EncodingResult<GB2312, CodePoint, EncodingResultCode::Incomplete>{ 0, 1 });
					return;
				}

				const auto firstUnit = static_cast<unsigned char>(span[0]);
				if (firstUnit < 0x80)
				{
					std::forward<OutputReceiver>(receiver)(
					    EncodingResult<GB2312, CodePoint, EncodingResultCode::Accept>{
					        static_cast<CodePointType>(firstUnit), 1u });
				}
				else if (firstUnit == 0x80)
				{
					// 特殊硬编码对应
					std::forward<OutputReceiver>(receiver)(
					    EncodingResult<GB2312, CodePoint, EncodingResultCode::Accept>{
					        static_cast<CodePointType>(0x20AC), 1u });
				}
				else
				{
					if (span.size() < 2)
					{
						std::forward<OutputReceiver>(receiver)(
						    EncodingResult<GB2312, CodePoint, EncodingResultCode::Incomplete>{ 0,
						                                                                       1 });
						return;
					}

					const auto secondUnit = static_cast<unsigned char>(span[1]);
					const auto unit = static_cast<std::uint16_t>(firstUnit) << 8 | secondUnit;
					const auto index = unit - 0x8140;

					if (index >= std::size(GB2312ToUnicodeMapping) ||
					    !GB2312ToUnicodeMapping[index])
					{
						std::forward<OutputReceiver>(receiver)(
						    EncodingResult<GB2312, CodePoint, EncodingResultCode::Reject>{});
					}
					else
					{
						const auto result =
						    static_cast<CodePointType>(GB2312ToUnicodeMapping[index]);
						std::forward<OutputReceiver>(receiver)(
						    EncodingResult<GB2312, CodePoint, EncodingResultCode::Accept>{ result,
						                                                                   2u });
					}
				}
			}

			template <typename OutputReceiver>
			static constexpr void FromCodePoint(CodePointType codePoint, OutputReceiver&& receiver)
			{
				if (codePoint > 0xFFFF)
				{
					std::forward<OutputReceiver>(receiver)(
					    EncodingResult<CodePoint, GB2312, EncodingResultCode::Reject>{});
					return;
				}

				if (codePoint < 0x80)
				{
					const auto result = static_cast<CharType>(codePoint);
					std::forward<OutputReceiver>(receiver)(
					    EncodingResult<CodePoint, GB2312, EncodingResultCode::Accept>{
					        std::span(&result, 1) });
				}
				else if (codePoint == 0x20AC) // 特殊硬编码对应
				{
					constexpr CharType result = 0x80;
					std::forward<OutputReceiver>(receiver)(
					    EncodingResult<CodePoint, GB2312, EncodingResultCode::Accept>{
					        std::span(&result, 1) });
				}
				else
				{
					const auto mappedUnit = UnicodeToGB2312Mapping[codePoint];
					if (!mappedUnit)
					{
						std::forward<OutputReceiver>(receiver)(
						    EncodingResult<CodePoint, GB2312, EncodingResultCode::Reject>{});
						return;
					}

					const CharType result[]{ static_cast<CharType>((mappedUnit & 0xFF00) >> 8),
						                     static_cast<CharType>(mappedUnit & 0xFF) };
					std::forward<OutputReceiver>(receiver)(
					    EncodingResult<CodePoint, GB2312, EncodingResultCode::Accept>{
					        std::span(result) });
				}
			}
		};
	} // namespace CodePage
} // namespace Cafe::Encoding

#endif

#ifdef CAFE_CODEPAGE
CAFE_CODEPAGE(::Cafe::Encoding::CodePage::GB2312)
#endif
