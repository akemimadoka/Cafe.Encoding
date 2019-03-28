#include <Encoding/Encode.h>
#include <catch2/catch.hpp>

using namespace Cafe;
using namespace Encoding;

#if __has_include(<Encoding/CodePage/UTF-8.h>) && __has_include(<Encoding/CodePage/UTF-16.h>)
TEST_CASE("Cafe.Encoding.Base", "[Encoding][String]")
{
	SECTION("Encoder test")
	{
		String<Utf8> str;
		Encoder::EncodeAll<Utf16LittleEndian, Utf8>(u"\xD852\xDF62\xFEFF", [&](auto const& result) {
			if constexpr (GetEncodingResultCode<decltype(result)> == EncodingResultCode::Accept)
			{
				str.Append(result.Result);
			}
			else
			{
				REQUIRE(false);
			}
		});
		REQUIRE(str == u8"\xF0\xA4\xAD\xA2\xEF\xBB\xBF");
	}
}
#endif
