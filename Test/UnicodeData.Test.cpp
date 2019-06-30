#include <Cafe/Encoding/RuntimeUnicodeData.h>
#include <catch2/catch.hpp>
#include <iostream>

using namespace Cafe;
using namespace Encoding;

TEST_CASE("Cafe.Encoding.UnicodeData", "[Encoding][UnicodeData]")
{
	SECTION("RuntimeUnicodeData")
	{
		const auto digitValue = RuntimeUnicodeData::GetDigitValue('0');
		REQUIRE(digitValue.value() == 0);
		for (CodePointType codePoint = 0; codePoint < 50; ++codePoint)
		{
			const auto name = RuntimeUnicodeData::GetCharacterName(codePoint);
			std::cout << reinterpret_cast<const char*>(name.GetData()) << std::endl;
		}
	}
}
