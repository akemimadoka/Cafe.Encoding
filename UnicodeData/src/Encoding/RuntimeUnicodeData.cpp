#include <Cafe/Encoding/RuntimeUnicodeData.h>
#include <Cafe/Encoding/UnicodeData.h>

namespace Cafe::Encoding
{
	namespace Detail
	{
		template <CodePointType CodePointValue, typename = void>
		struct GetCharacterDecompositionMappingImpl
		{
			static constexpr std::optional<std::pair<DecompositionTag, gsl::span<const CodePointType>>>
			    DecompositionMapping{};
		};

		template <CodePointType CodePointValue>
		struct GetCharacterDecompositionMappingImpl<
		    CodePointValue, std::void_t<decltype(UnicodeData<CodePointValue>::DecompositionMapping)>>
		{
			static constexpr std::optional<std::pair<DecompositionTag, gsl::span<const CodePointType>>>
			    DecompositionMapping{ UnicodeData<CodePointValue>::DecompositionMapping.first,
				                        UnicodeData<CodePointValue>::DecompositionMapping.second };
		};

		template <CodePointType CodePointValue>
		struct GetCharacterDecompositionMapping
		    : Detail::GetCharacterDecompositionMappingImpl<CodePointValue>
		{
		};

		constexpr auto CodePointMap = []() constexpr
		{
			std::array<CodePointType, MaxValidCodePoint + 1> result{};
			CodePointType i{};
#define UNICODE_DEFINE(codeValue, characterName, generalCategory, canonicalCombiningClasses,       \
                       bidirectionalCategory, characterDecompositionMapping, decimalDigitValue,    \
                       digitValue, numeric, mirrored, unicodeName, commentField, uppercaseMapping, \
                       lowercaseMapping, titlecaseMapping)                                         \
	result[codeValue] = i++;
#include <Cafe/Encoding/Impl/UnicodeData.h>
			return result;
		}
		();

		template <typename IndexSeq>
		struct UnicodeDataArrayGenerator;

		template <CodePointType... I>
		struct UnicodeDataArrayGenerator<std::integer_sequence<CodePointType, I...>>
		{
			static constexpr StringView<CodePage::Utf8> CharacterNameArray[]{
				UnicodeData<I>::CharacterName...
			};
			static constexpr GeneralCategory GeneralCategoryValueArray[]{
				UnicodeData<I>::GeneralCategoryValue...
			};
			static constexpr CanonicalCombiningClasses CanonicalCombiningClassesValueArray[]{
				UnicodeData<I>::CanonicalCombiningClassesValue...
			};
			static constexpr BidirectionalCategory BidirectionalCategoryValueArray[]{
				UnicodeData<I>::BidirectionalCategoryValue...
			};
			static constexpr std::optional<std::pair<DecompositionTag, gsl::span<const CodePointType>>>
			    DecompositionMappingArray[]{
				    GetCharacterDecompositionMapping<I>::DecompositionMapping...
			    };
			static constexpr std::optional<std::uint8_t> DecimalDigitValueArray[]{
				UnicodeData<I>::DecimalDigitValue...
			};
			static constexpr std::optional<std::uint8_t> DigitValueArray[]{
				UnicodeData<I>::DigitValue...
			};
			static constexpr std::optional<Core::Misc::Ratio<unsigned>> NumericArray[]{
				UnicodeData<I>::Numeric...
			};
			static constexpr Mirrored MirroredArray[]{ UnicodeData<I>::MirroredValue... };
			static constexpr StringView<CodePage::Utf8> UnicodeNameArray[]{
				UnicodeData<I>::UnicodeName...
			};
			static constexpr StringView<CodePage::Utf8> CommentFieldArray[]{
				UnicodeData<I>::CommentField...
			};
			static constexpr std::optional<CodePointType> UppercaseMappingArray[]{
				UnicodeData<I>::UppercaseMapping...
			};
			static constexpr std::optional<CodePointType> LowercaseMappingArray[]{
				UnicodeData<I>::LowercaseMapping...
			};
			static constexpr std::optional<CodePointType> TitlecaseMappingArray[]{
				UnicodeData<I>::TitlecaseMapping...
			};
		};
	} // namespace Detail

	namespace RuntimeUnicodeData
	{
		using UnderlyingData = Detail::UnicodeDataArrayGenerator<std::integer_sequence<CodePointType
#define UNICODE_DEFINE(codeValue, characterName, generalCategory, canonicalCombiningClasses,       \
                       bidirectionalCategory, characterDecompositionMapping, decimalDigitValue,    \
                       digitValue, numeric, mirrored, unicodeName, commentField, uppercaseMapping, \
                       lowercaseMapping, titlecaseMapping)                                         \
	, codeValue
#include <Cafe/Encoding/Impl/UnicodeData.h>
		    >>;

		bool IsInUnicodeData(CodePointType codePoint) noexcept
		{
			assert(codePoint <= MaxValidCodePoint);
			return codePoint == 0 || Detail::CodePointMap[codePoint] != 0;
		}

		StringView<CodePage::Utf8> GetCharacterName(CodePointType codePoint) noexcept
		{
			assert(codePoint <= MaxValidCodePoint);
			return UnderlyingData::CharacterNameArray[Detail::CodePointMap[codePoint]];
		}

		GeneralCategory GetGeneralCategory(CodePointType codePoint) noexcept
		{
			assert(codePoint <= MaxValidCodePoint);
			return UnderlyingData::GeneralCategoryValueArray[Detail::CodePointMap[codePoint]];
		}

		CanonicalCombiningClasses GetCanonicalCombiningClasses(CodePointType codePoint) noexcept
		{
			assert(codePoint <= MaxValidCodePoint);
			return UnderlyingData::CanonicalCombiningClassesValueArray[Detail::CodePointMap[codePoint]];
		}

		BidirectionalCategory GetBidirectionalCategory(CodePointType codePoint) noexcept
		{
			assert(codePoint <= MaxValidCodePoint);
			return UnderlyingData::BidirectionalCategoryValueArray[Detail::CodePointMap[codePoint]];
		}

		std::optional<std::pair<DecompositionTag, gsl::span<const CodePointType>>> const&
		GetDecompositionMapping(CodePointType codePoint) noexcept
		{
			assert(codePoint <= MaxValidCodePoint);
			return UnderlyingData::DecompositionMappingArray[Detail::CodePointMap[codePoint]];
		}

		std::optional<std::uint8_t> const& GetDecimalDigitValue(CodePointType codePoint) noexcept
		{
			assert(codePoint <= MaxValidCodePoint);
			return UnderlyingData::DecimalDigitValueArray[Detail::CodePointMap[codePoint]];
		}

		std::optional<std::uint8_t> const& GetDigitValue(CodePointType codePoint) noexcept
		{
			assert(codePoint <= MaxValidCodePoint);
			return UnderlyingData::DigitValueArray[Detail::CodePointMap[codePoint]];
		}

		std::optional<Core::Misc::Ratio<unsigned>> const& GetNumeric(CodePointType codePoint) noexcept
		{
			assert(codePoint <= MaxValidCodePoint);
			return UnderlyingData::NumericArray[Detail::CodePointMap[codePoint]];
		}

		Mirrored IsMirrored(CodePointType codePoint) noexcept
		{
			assert(codePoint <= MaxValidCodePoint);
			return UnderlyingData::MirroredArray[Detail::CodePointMap[codePoint]];
		}

		StringView<CodePage::Utf8> GetUnicodeName(CodePointType codePoint) noexcept
		{
			assert(codePoint <= MaxValidCodePoint);
			return UnderlyingData::UnicodeNameArray[Detail::CodePointMap[codePoint]];
		}

		StringView<CodePage::Utf8> GetCommentField(CodePointType codePoint) noexcept
		{
			assert(codePoint <= MaxValidCodePoint);
			return UnderlyingData::CommentFieldArray[Detail::CodePointMap[codePoint]];
		}

		std::optional<CodePointType> const& GetUppercaseMapping(CodePointType codePoint) noexcept
		{
			assert(codePoint <= MaxValidCodePoint);
			return UnderlyingData::UppercaseMappingArray[Detail::CodePointMap[codePoint]];
		}

		std::optional<CodePointType> const& GetLowercaseMapping(CodePointType codePoint) noexcept
		{
			assert(codePoint <= MaxValidCodePoint);
			return UnderlyingData::LowercaseMappingArray[Detail::CodePointMap[codePoint]];
		}

		std::optional<CodePointType> const& GetTitlecaseMapping(CodePointType codePoint) noexcept
		{
			assert(codePoint <= MaxValidCodePoint);
			return UnderlyingData::TitlecaseMappingArray[Detail::CodePointMap[codePoint]];
		}
	} // namespace RuntimeUnicodeData
} // namespace Cafe::Encoding
