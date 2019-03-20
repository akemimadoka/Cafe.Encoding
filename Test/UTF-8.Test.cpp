#include <catch2/catch.hpp>
#include <Encoding/CodePage/UTF-8.h>
#include <cstring>

using namespace Cafe;
using namespace Encoding;

TEST_CASE("Cafe.Encoding.UTF-8", "[Encoding][UTF-8]")
{
	SECTION("CodePoint to UTF-8")
	{
		CodePage::CodePageTrait<CodePage::Utf8>::FromCodePoint(0xFEFF, [](auto const& result) {
			if constexpr (GetEncodingResultCode<decltype(result)> == EncodingResultCode::Accept)
			{
				REQUIRE(result.Result.size() == 3);
				REQUIRE(std::memcmp(result.Result.data(), "\xEF\xBB\xBF", 3) == 0);
			}
			else
			{
				REQUIRE(false);
			}
		});
	}
}
