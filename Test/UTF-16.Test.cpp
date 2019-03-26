#include <Encoding/CodePage/UTF-16.h>
#include <catch2/catch.hpp>
#include <cstring>

using namespace Cafe;
using namespace Encoding;

TEST_CASE("Cafe.Encoding.UTF-16", "[Encoding][UTF-16]")
{
	SECTION("CodePoint to UTF-16 LE")
	{
		CodePage::CodePageTrait<CodePage::Utf16LittleEndian>::FromCodePoint(
		    0x24B62, [](auto const& result) {
			    if constexpr (GetEncodingResultCode<decltype(result)> == EncodingResultCode::Accept)
			    {
				    REQUIRE(result.Result.size() == 2);
				    const auto span = gsl::as_bytes(result.Result);
				    REQUIRE(std::memcmp(span.data(), "\x52\xD8\x62\xDF", 4) == 0);
			    }
			    else
			    {
				    REQUIRE(false);
			    }
		    });
	}

	SECTION("CodePoint to UTF-16 BE")
	{
		CodePage::CodePageTrait<CodePage::Utf16BigEndian>::FromCodePoint(
		    0x24B62, [](auto const& result) {
			    if constexpr (GetEncodingResultCode<decltype(result)> == EncodingResultCode::Accept)
			    {
				    REQUIRE(result.Result.size() == 2);
				    const auto span = gsl::as_bytes(result.Result);
				    REQUIRE(std::memcmp(span.data(), "\xD8\x52\xDF\x62", 4) == 0);
			    }
			    else
			    {
				    REQUIRE(false);
			    }
		    });
	}
}
