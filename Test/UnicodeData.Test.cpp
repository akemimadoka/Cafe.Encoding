#include <Cafe/Encoding/RuntimeUnicodeData.h>
#include <catch2/catch_all.hpp>
#include <iostream>

using namespace Cafe;
using namespace Encoding;

TEST_CASE("Cafe.Encoding.UnicodeData", "[Encoding][UnicodeData]")
{
	SECTION("RuntimeUnicodeData")
	{
		const auto zeroUnicodeData = RuntimeUnicodeData::GetUnicodeData('0');
		const auto digitValue = zeroUnicodeData.value().DigitValue;
		REQUIRE(digitValue.value() == 0);
		for (CodePointType codePoint = 0; codePoint < 50; ++codePoint)
		{
			const auto unicodeData = RuntimeUnicodeData::GetUnicodeData(codePoint);
			const auto name = unicodeData.value().CharacterName;
			std::cout << reinterpret_cast<const char*>(name.GetData()) << std::endl;
		}
	}
}
