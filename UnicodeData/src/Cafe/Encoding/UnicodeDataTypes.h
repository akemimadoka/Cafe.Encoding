#pragma once
#include <cstdint>

namespace Cafe::Encoding
{
	enum class GeneralCategory : std::uint8_t
	{
		Invalid,

		// Normative Categories
		Lu, // Letter, Uppercase
		Ll, // Letter, Lowercase
		Lt, // Letter, Titlecase
		Mn, // Mark, Non-Spacing
		Mc, // Mark, Spacing Combining
		Me, // Mark, Enclosing
		Nd, // Number, Decimal Digit
		Nl, // Number, Letter
		No, // Number, Other
		Zs, // Separator, Space
		Zl, // Separator, Line
		Zp, // Separator, Paragraph
		Cc, // Other, Control
		Cf, // Other, Format
		Cs, // Other, Surrogate
		Co, // Other, Private Use
		Cn, // Other, Not Assigned (not used)

		// Informative Categories
		Lm, // Letter, Modifier
		Lo, // Letter, Other
		Pc, // Punctuation, Connector
		Pd, // Punctuation, Dash
		Ps, // Punctuation, Open
		Pe, // Punctuation, Close
		Pi, // Punctuation, Initial quote (may behave like Ps or Pe depending on usage)
		Pf, // Punctuation, Final quote (may behave like Ps or Pe depending on usage)
		Po, // Punctuation, Other
		Sm, // Symbol, Math
		Sc, // Symbol, Currency
		Sk, // Symbol, Modifier
		So  // Symbol, Other
	};

	constexpr bool IsInformativeCategory(GeneralCategory value) noexcept
	{
		return value >= GeneralCategory::Lm;
	}

	enum class CanonicalCombiningClasses : std::uint8_t
	{
		NotReordered = 0,         // Spacing, split, enclosing, reordrant, and Tibetan subjoined
		Overlay = 1,              // Overlays and interior
		Nuktas = 7,               // Nuktas
		KanaVoicing = 8,          // Hiragana/Katakana voicing marks
		Viramas = 9,              // Viramas
		BelowLeftAttached = 200,  // Below left attached
		BelowAttached = 202,      // Below attached
		BelowRightAttached = 204, // Below right attached
		LeftAttached = 208,       // Left attached (reordrant around single base character)
		RightAttached = 210,      // Right attached
		AboveLeftAttached = 212,  // Above left attached
		AboveAttached = 214,      // Above attached
		AboveRightAttached = 216, // Above right attached
		BelowLeft = 218,          // Below left
		Below = 220,              // Below
		BelowRight = 222,         // Below right
		Left = 224,               // Left (reordrant around single base character)
		Right = 226,              // Right
		AboveLeft = 228,          // Above left
		Above = 230,              // Above
		AboveRight = 232,         // Above right
		DoubleBelow = 233,        // Double below
		DoubleAbove = 234,        // Double above
		IotaSubscript = 240,      // Below (iota subscript)

		Invalid,
	};

	enum class BidirectionalCategory : std::uint8_t
	{
		Invalid,

		L,   //	Left-to-Right
		LRE, //	Left-to-Right Embedding
		LRO, //	Left-to-Right Override
		R,   //	Right-to-Left
		AL,  //	Right-to-Left Arabic
		RLE, //	Right-to-Left Embedding
		RLO, //	Right-to-Left Override
		PDF, //	Pop Directional Format
		EN,  //	European Number
		ES,  //	European Number Separator
		ET,  //	European Number Terminator
		AN,  //	Arabic Number
		CS,  //	Common Number Separator
		NSM, //	Non-Spacing Mark
		BN,  //	Boundary Neutral
		B,   //	Paragraph Separator
		S,   //	Segment Separator
		WS,  //	Whitespace
		ON,  //	Other Neutrals
		FSI, // First Strong Isolate
		LRI, // Left To Right Isolate
		PDI, // Pop Directional Isolate
		RLI, // Right To Left Isolate
	};

	enum class DecompositionTag : std::uint8_t
	{
		Invalid,

		Canonical, // Canonical equivalence
		Font,      // A font variant (e.g. a blackletter form)
		NoBreak,   // A no-break version of a space or hyphen
		Initial,   // An initial presentation form (Arabic)
		Medial,    // A medial presentation form (Arabic)
		Final,     // A final presentation form (Arabic)
		Isolated,  // An isolated presentation form (Arabic)
		Circle,    // An encircled form
		Super,     // A superscript form
		Sub,       // A subscript form
		Vertical,  // A vertical layout presentation form
		Wide,      // A wide (or zenkaku) compatibility character
		Narrow,    // A narrow (or hankaku) compatibility character
		Small,     // A small variant form (CNS compatibility)
		Square,    // A CJK squared font variant
		Fraction,  // A vulgar fraction form
		Compat,    // Otherwise unspecified compatibility character
	};

	enum class Mirrored : std::uint8_t
	{
		N,
		Y,
		Invalid
	};

	struct UnicodeData
	{
		CodePointType CodeValue;
		StringView<CodePage::Utf8> CharacterName;
		GeneralCategory GeneralCategoryValue;
		CanonicalCombiningClasses CanonicalCombiningClassesValue;
		BidirectionalCategory BidirectionalCategoryValue;
		std::optional<std::pair<DecompositionTag, std::array<CodePointType, 20>>>
		    DecompositionMapping;
		std::optional<std::uint8_t> DecimalDigitValue;
		std::optional<std::uint8_t> DigitValue;
		std::optional<Core::Misc::Ratio<std::uintmax_t>> Numeric;
		Mirrored MirroredValue;
		StringView<CodePage::Utf8> UnicodeName;
		StringView<CodePage::Utf8> CommentField;
		std::optional<CodePointType> UppercaseMapping;
		std::optional<CodePointType> LowercaseMapping;
		std::optional<CodePointType> TitlecaseMapping;
	};
} // namespace Cafe::Encoding
