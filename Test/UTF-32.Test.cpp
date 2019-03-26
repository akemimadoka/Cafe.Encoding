#include <Encoding/CodePage/UTF-32.h>
#include <catch2/catch.hpp>
#include <cstring>

using namespace Cafe;
using namespace Encoding;

TEST_CASE("Cafe.Encoding.UTF-32", "[Encoding][UTF-32]")
{
	SECTION("CodePoint to UTF-32 LE")
	{
		CodePage::CodePageTrait<CodePage::Utf32LittleEndian>::FromCodePoint(
		    0x24B62, [](auto const& result) {
			    if constexpr (GetEncodingResultCode<decltype(result)> == EncodingResultCode::Accept)
			    {
				    const auto span = gsl::as_bytes(gsl::make_span(&result.Result, 1));
				    REQUIRE(std::memcmp(span.data(), "\x62\x4B\x02\x00", 4) == 0);
			    }
			    else
			    {
				    REQUIRE(false);
			    }
		    });
	}

	SECTION("CodePoint to UTF-32 BE")
	{
		CodePage::CodePageTrait<CodePage::Utf32BigEndian>::FromCodePoint(
		    0x24B62, [](auto const& result) {
			    if constexpr (GetEncodingResultCode<decltype(result)> == EncodingResultCode::Accept)
			    {
				    const auto span = gsl::as_bytes(gsl::make_span(&result.Result, 1));
				    REQUIRE(std::memcmp(span.data(), "\x00\x02\x4B\x62", 4) == 0);
			    }
			    else
			    {
				    REQUIRE(false);
			    }
		    });
	}
}