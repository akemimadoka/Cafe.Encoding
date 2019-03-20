#include <Encoding/RuntimeUnicodeData.h>
#include <catch2/catch.hpp>

using namespace Cafe;
using namespace Encoding;

TEST_CASE("Cafe.Encoding.UnicodeData", "[Encoding][UnicodeData]")
{
	SECTION("RuntimeUnicodeData")
	{
		REQUIRE(RuntimeUnicodeData::GetDigitValue('0') == 0);
	}
}
