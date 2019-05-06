#include <Cafe/Encoding/Strings.h>
#include <catch2/catch.hpp>

#if __has_include(<Cafe/Encoding/CodePage/UTF-8.h>)
#	include <Cafe/Encoding/CodePage/UTF-8.h>
#endif

using namespace Cafe;
using namespace Encoding;

#if __has_include(<Cafe/Encoding/CodePage/UTF-8.h>)
TEST_CASE("Cafe.Encoding.Base.String", "[Encoding][String]")
{
	SECTION("String test")
	{
		const auto str = CAFE_UTF8_SV("ababc");
		REQUIRE(str.GetSize() == 6);
		const auto pattern = CAFE_UTF8_SV("ab");
		REQUIRE(pattern.GetSize() == 3);

		const auto foundPos = str.Find(pattern);
		REQUIRE(foundPos == 0);

		auto str2 = str.ToString();
		REQUIRE(str2.GetView() == str);
		str2.Append(pattern);
		REQUIRE(str2.GetView() == CAFE_UTF8_SV("ababcab"));
	}
}
#endif
