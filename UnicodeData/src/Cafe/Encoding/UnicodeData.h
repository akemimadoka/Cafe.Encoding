#pragma once
#include <Cafe/Encoding/CodePage.h>
#include <Cafe/Encoding/CodePage/UTF-8.h>
#include <Cafe/Encoding/Strings.h>
#include <Cafe/Misc/Ratio.h>
#include <Cafe/Misc/Utility.h>

#include <array>
#include <cassert>
#include <optional>

#include "UnicodeDataTypes.h"

namespace Cafe::Encoding
{
	namespace Detail
	{
		using DecompositionTagUnderlyingType = std::underlying_type_t<DecompositionTag>;
		enum class DecompositionTagMap : DecompositionTagUnderlyingType
		{
			canonical = static_cast<DecompositionTagUnderlyingType>(DecompositionTag::Canonical),
			font = static_cast<DecompositionTagUnderlyingType>(DecompositionTag::Font),
			noBreak = static_cast<DecompositionTagUnderlyingType>(DecompositionTag::NoBreak),
			initial = static_cast<DecompositionTagUnderlyingType>(DecompositionTag::Initial),
			medial = static_cast<DecompositionTagUnderlyingType>(DecompositionTag::Medial),
			final = static_cast<DecompositionTagUnderlyingType>(DecompositionTag::Final),
			isolated = static_cast<DecompositionTagUnderlyingType>(DecompositionTag::Isolated),
			circle = static_cast<DecompositionTagUnderlyingType>(DecompositionTag::Circle),
			super = static_cast<DecompositionTagUnderlyingType>(DecompositionTag::Super),
			sub = static_cast<DecompositionTagUnderlyingType>(DecompositionTag::Sub),
			vertical = static_cast<DecompositionTagUnderlyingType>(DecompositionTag::Vertical),
			wide = static_cast<DecompositionTagUnderlyingType>(DecompositionTag::Wide),
			narrow = static_cast<DecompositionTagUnderlyingType>(DecompositionTag::Narrow),
			small = static_cast<DecompositionTagUnderlyingType>(DecompositionTag::Small),
			square = static_cast<DecompositionTagUnderlyingType>(DecompositionTag::Square),
			fraction = static_cast<DecompositionTagUnderlyingType>(DecompositionTag::Fraction),
			compat = static_cast<DecompositionTagUnderlyingType>(DecompositionTag::Compat),
		};

		template <typename T, typename... Args>
		constexpr auto ConvertToArray(Args&&... args) noexcept(
		    noexcept(Cafe::Core::Misc::ConvertConstruct<std::array<T, sizeof...(Args)>, T>(
		        std::forward<Args>(args)...)))
		{
			return Cafe::Core::Misc::ConvertConstruct<std::array<T, sizeof...(Args)>, T>(
			    std::forward<Args>(args)...);
		}
	} // namespace Detail

	inline constexpr UnicodeData UnicodeDataArray[] = {

#define DECOMPOSITION(tag, ...)                                                                    \
	static_cast<DecompositionTag>(Detail::DecompositionTagMap::tag),                               \
	{                                                                                              \
		__VA_ARGS__                                                                                \
	}

#define NUMERIC(...) Core::Misc::Ratio<std::uintmax_t>(__VA_ARGS__)

#define UNICODE_DEFINE(codeValue, characterName, generalCategory, canonicalCombiningClasses,       \
                       bidirectionalCategory, characterDecompositionMapping, decimalDigitValue,    \
                       digitValue, numeric, mirrored, unicodeName, commentField, uppercaseMapping, \
                       lowercaseMapping, titlecaseMapping)                                         \
                                                                                                   \
	{                                                                                              \
		.CodeValue = static_cast<CodePointType>(codeValue),                                        \
		.CharacterName{ u8##characterName },                                                       \
		.GeneralCategoryValue = GeneralCategory::generalCategory,                                  \
		.CanonicalCombiningClassesValue =                                                          \
		    static_cast<CanonicalCombiningClasses>(canonicalCombiningClasses),                     \
		.BidirectionalCategoryValue = BidirectionalCategory::bidirectionalCategory,                \
		.DecompositionMapping = { characterDecompositionMapping },                                 \
		.DecimalDigitValue{ decimalDigitValue },                                                   \
		.DigitValue{ digitValue },                                                                 \
		.Numeric{ numeric },                                                                       \
		.MirroredValue = Mirrored::mirrored,                                                       \
		.UnicodeName{ u8##unicodeName },                                                           \
		.CommentField{ u8##commentField },                                                         \
		.UppercaseMapping{ uppercaseMapping },                                                     \
		.LowercaseMapping{ lowercaseMapping },                                                     \
		.TitlecaseMapping{ titlecaseMapping },                                                     \
	},

#include "Impl/UnicodeData.h"

	};

	inline constexpr auto CodePointMap = []() constexpr
	{
		std::array<CodePointType, MaxValidCodePoint + 1> result{};
		CodePointType i{};
#define UNICODE_DEFINE(codeValue, characterName, generalCategory, canonicalCombiningClasses,       \
                       bidirectionalCategory, characterDecompositionMapping, decimalDigitValue,    \
                       digitValue, numeric, mirrored, unicodeName, commentField, uppercaseMapping, \
                       lowercaseMapping, titlecaseMapping)                                         \
	result[codeValue] = i++;
#include "Impl/UnicodeData.h"
		return result;
	}
	();

	constexpr UnicodeData GetUnicodeData(CodePointType codePoint) noexcept
	{
		assert(codePoint <= MaxValidCodePoint);
		return UnicodeDataArray[CodePointMap[codePoint]];
	}

	constexpr bool IsValidUnicodeData(CodePointType codePoint) noexcept
	{
		return !codePoint || (codePoint <= MaxValidCodePoint && CodePointMap[codePoint] != 0);
	}
} // namespace Cafe::Encoding
