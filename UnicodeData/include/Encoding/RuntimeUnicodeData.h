#pragma once
#include <Encoding/CodePage.h>
#include <Encoding/CodePage/UTF-8.h>
#include <Encoding/Strings.h>
#include <Misc/Export.h>
#include <Misc/Ratio.h>

#include <optional>

#include "UnicodeDataTypes.h"

namespace Cafe::Encoding::RuntimeUnicodeData
{
	CAFE_PUBLIC bool IsInUnicodeData(CodePointType codePoint) noexcept;

	CAFE_PUBLIC StringView<CodePage::Utf8> GetCharacterName(CodePointType codePoint) noexcept;

	CAFE_PUBLIC GeneralCategory GetGeneralCategory(CodePointType codePoint) noexcept;

	CAFE_PUBLIC CanonicalCombiningClasses
	GetCanonicalCombiningClasses(CodePointType codePoint) noexcept;

	CAFE_PUBLIC BidirectionalCategory GetBidirectionalCategory(CodePointType codePoint) noexcept;

	CAFE_PUBLIC std::optional<std::pair<DecompositionTag, gsl::span<const CodePointType>>> const&
	GetDecompositionMapping(CodePointType codePoint) noexcept;

	CAFE_PUBLIC std::optional<std::uint8_t> const& GetDecimalDigitValue(CodePointType codePoint) noexcept;

	CAFE_PUBLIC std::optional<std::uint8_t> const& GetDigitValue(CodePointType codePoint) noexcept;

	CAFE_PUBLIC std::optional<Core::Misc::Ratio<unsigned>> const&
	GetNumeric(CodePointType codePoint) noexcept;

	CAFE_PUBLIC Mirrored IsMirrored(CodePointType codePoint) noexcept;

	CAFE_PUBLIC StringView<CodePage::Utf8> GetUnicodeName(CodePointType codePoint) noexcept;

	CAFE_PUBLIC StringView<CodePage::Utf8> GetCommentField(CodePointType codePoint) noexcept;

	CAFE_PUBLIC std::optional<CodePointType> const&
	GetUppercaseMapping(CodePointType codePoint) noexcept;

	CAFE_PUBLIC std::optional<CodePointType> const&
	GetLowercaseMapping(CodePointType codePoint) noexcept;

	CAFE_PUBLIC std::optional<CodePointType> const&
	GetTitlecaseMapping(CodePointType codePoint) noexcept;
} // namespace Cafe::Encoding::RuntimeUnicodeData
