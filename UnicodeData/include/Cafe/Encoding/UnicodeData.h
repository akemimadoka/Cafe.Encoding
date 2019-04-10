#pragma once
#include <Cafe/Encoding/CodePage.h>
#include <Cafe/Encoding/CodePage/UTF-8.h>
#include <Cafe/Encoding/Strings.h>
#include <Cafe/Misc/Ratio.h>
#include <Cafe/Misc/Utility.h>

#include <array>
#include <optional>

#include "UnicodeDataTypes.h"

namespace Cafe::Encoding
{
	namespace Detail
	{
		using DecompositionTagUnderlyingType = std::underlying_type_t<DecompositionTag>;
		enum class DecompositionTagMap : DecompositionTagUnderlyingType
		{
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

	template <CodePointType CodePointValue>
	struct UnicodeData;

#define DECOMPOSITION(tag, ...)                                                                    \
	static constexpr auto DecompositionMapping = std::pair                                           \
	{                                                                                                \
		static_cast<DecompositionTag>(Detail::DecompositionTagMap::tag),                               \
		    Detail::ConvertToArray<CodePointType>(__VA_ARGS__)                                         \
	}

#define UNICODE_DEFINE(codeValue, characterName, generalCategory, canonicalCombiningClasses,       \
                       bidirectionalCategory, characterDecompositionMapping, decimalDigitValue,    \
                       digitValue, numeric, mirrored, unicodeName, commentField, uppercaseMapping, \
                       lowercaseMapping, titlecaseMapping)                                         \
	template <>                                                                                      \
	struct UnicodeData<static_cast<CodePointType>(codeValue)>                                        \
	{                                                                                                \
		static constexpr CodePointType CodeValue = static_cast<CodePointType>(codeValue);              \
		static constexpr StringView<CodePage::Utf8> CharacterName{ u8##characterName };                \
		static constexpr GeneralCategory GeneralCategoryValue = GeneralCategory::generalCategory;      \
		static constexpr CanonicalCombiningClasses CanonicalCombiningClassesValue =                    \
		    static_cast<CanonicalCombiningClasses>(canonicalCombiningClasses);                         \
		static constexpr BidirectionalCategory BidirectionalCategoryValue =                            \
		    BidirectionalCategory::bidirectionalCategory;                                              \
		characterDecompositionMapping;                                                                 \
		static constexpr std::optional<std::uint8_t> DecimalDigitValue{ decimalDigitValue };           \
		static constexpr std::optional<std::uint8_t> DigitValue{ digitValue };                         \
		static constexpr std::optional<Core::Misc::Ratio<unsigned>> Numeric{ numeric };                \
		static constexpr Mirrored MirroredValue = Mirrored::mirrored;                                  \
		static constexpr StringView<CodePage::Utf8> UnicodeName{ u8##unicodeName };                    \
		static constexpr StringView<CodePage::Utf8> CommentField{ u8##commentField };                  \
		static constexpr std::optional<CodePointType> UppercaseMapping{ uppercaseMapping };            \
		static constexpr std::optional<CodePointType> LowercaseMapping{ lowercaseMapping };            \
		static constexpr std::optional<CodePointType> TitlecaseMapping{ titlecaseMapping };            \
	};

#include "Impl/UnicodeData.h"

	template <CodePointType CodePointValue>
	constexpr bool IsValidUnicodeData = Core::Misc::IsComplete<UnicodeData<CodePointValue>>;
} // namespace Cafe::Encoding
