#include <Cafe/Encoding/RuntimeEncoding.h>

#ifdef _WIN32
#	include <Winnls.h>
#endif

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

#ifdef _WIN32
Cafe::Encoding::CodePage::CodePageType Cafe::Encoding::RuntimeEncoding::GetAnsiEncoding() noexcept
{
	return static_cast<CodePage::CodePageType>(GetACP());
}
#endif
