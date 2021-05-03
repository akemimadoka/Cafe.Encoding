#include <Cafe/Encoding/RuntimeEncoding.h>
#include <catch2/catch.hpp>
#include <cstring>
#include <vector>

using namespace Cafe;
using namespace Encoding;
using namespace StringLiterals;

#if __has_include(<Cafe/Encoding/CodePage/UTF-8.h>) && __has_include(<Cafe/Encoding/CodePage/UTF-16.h>)
TEST_CASE("Cafe.Encoding.RuntimeEncoding", "[Encoding][RuntimeEncoding]")
{
	SECTION("RuntimeEncoding test")
	{
		const auto u16Str = u"\xD852\xDF62\xFEFF"_sv;
		String<CodePage::Utf8> resultStr;
		RuntimeEncoding::EncodeAll(
		    CodePage::Utf16LittleEndian, std::as_bytes(u16Str.GetSpan()), CodePage::Utf8,
		    [&](auto const& result) {
			    REQUIRE(result.ResultCode == EncodingResultCode::Accept);
			    resultStr.Append(std::span(
			        reinterpret_cast<
			            const typename CodePage::CodePageTrait<CodePage::Utf8>::CharType*>(
			            result.Result.data()),
			        result.Result.size()));
		    });

		constexpr auto expectedOutput = CAFE_UTF8_SV("\xF0\xA4\xAD\xA2\xEF\xBB\xBF");
		REQUIRE(resultStr.GetSize() == expectedOutput.GetSize());
		REQUIRE(resultStr == expectedOutput);
	}
}
#endif
