#include <Cafe/Encoding/CodePage/GB2312.h>
#include <catch2/catch.hpp>
#include <cstring>

using namespace Cafe;
using namespace Encoding;

TEST_CASE("Cafe.Encoding.GB2312", "[Encoding][GB2312]")
{
	SECTION("CodePoint to GB2312")
	{
		CodePage::CodePageTrait<CodePage::GB2312>::FromCodePoint(0x6D4B, [](auto const& result) {
			if constexpr (GetEncodingResultCode<decltype(result)> == EncodingResultCode::Accept)
			{
				REQUIRE(result.Result.size() == 2);
				REQUIRE(std::memcmp(result.Result.data(), "\xB2\xE2", 2) == 0);
			}
			else
			{
				REQUIRE(false);
			}
		});

		CodePage::CodePageTrait<CodePage::GB2312>::ToCodePoint(gsl::make_span("\xB2\xE2"), [](auto const& result) {
			if constexpr (GetEncodingResultCode<decltype(result)> == EncodingResultCode::Accept)
			{
				REQUIRE(result.AdvanceCount == 2);
				REQUIRE(result.Result == 0x6D4B);
			}
			else
			{
				REQUIRE(false);
			}
		});
	}
}
