#include <Cafe/Encoding/Encode.h>
#include <Cafe/Encoding/Strings.h>
#include <catch2/catch.hpp>

#if __has_include(<Cafe/Encoding/CodePage/UTF-8.h>) && __has_include(<Cafe/Encoding/CodePage/UTF-16.h>)

#	include <Cafe/Encoding/CodePage/UTF-16.h>
#	include <Cafe/Encoding/CodePage/UTF-8.h>

using namespace Cafe;
using namespace Encoding;

TEST_CASE("Cafe.Encoding.Base.Encoder", "[Encoding][String]")
{
	SECTION("Encoder test")
	{
		String<CodePage::Utf8> str;
		Encoder<CodePage::Utf16LittleEndian, CodePage::Utf8>::EncodeAll(
		    std::span(u"\xD852\xDF62\xFEFF"), [&](auto const& result) {
			    if constexpr (GetEncodingResultCode<decltype(result)> == EncodingResultCode::Accept)
			    {
				    str.Append(result.Result);
			    }
			    else
			    {
				    REQUIRE(false);
			    }
		    });
		REQUIRE(str.GetView() == StringView<CodePage::Utf8>(u8"\xF0\xA4\xAD\xA2\xEF\xBB\xBF"));
	}
}
#endif
