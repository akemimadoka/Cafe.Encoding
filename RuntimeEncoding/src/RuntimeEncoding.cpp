#include <Cafe/Encoding/RuntimeEncoding.h>
#include <Cafe/Misc/Utility.h>

#ifdef _WIN32
#	include <Winnls.h>
#endif

gsl::span<const Cafe::Encoding::CodePage::CodePageType>
Cafe::Encoding::RuntimeEncoding::GetSupportCodePages() noexcept
{
	return gsl::make_span(Cafe::Core::Misc::SequenceToArray<AllIncludedCodePages>::Array);
}

std::string_view
Cafe::Encoding::RuntimeEncoding::GetCodePageName(CodePage::CodePageType codePage) noexcept
{
	switch (codePage)
	{
#define CAFE_CODEPAGE(codePageValue)                                                               \
	case codePageValue:                                                                              \
		return CodePage::CodePageTrait<codePageValue>::Name;
#include <Cafe/Encoding/Config/IncludedEncoding.h>
	default:
		return {};
	}
}

std::optional<bool>
Cafe::Encoding::RuntimeEncoding::IsCodePageVariableWidth(CodePage::CodePageType codePage) noexcept
{
	switch (codePage)
	{
#define CAFE_CODEPAGE(codePageValue)                                                               \
	case codePageValue:                                                                              \
		return CodePage::CodePageTrait<codePageValue>::IsVariableWidth;
#include <Cafe/Encoding/Config/IncludedEncoding.h>
	default:
		return {};
	}
}

std::size_t
Cafe::Encoding::RuntimeEncoding::GetCodePageMaxWidth(CodePage::CodePageType codePage) noexcept
{
	switch (codePage)
	{
#define CAFE_CODEPAGE(codePageValue)                                                               \
	case codePageValue:                                                                              \
		return CodePage::GetMaxWidth<codePageValue>();
#include <Cafe/Encoding/Config/IncludedEncoding.h>
	default:
		return {};
	}
}

#ifdef _WIN32
Cafe::Encoding::CodePage::CodePageType Cafe::Encoding::RuntimeEncoding::GetAnsiEncoding() noexcept
{
	return static_cast<CodePage::CodePageType>(GetACP());
}
#endif
