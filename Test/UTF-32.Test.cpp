#include <Cafe/Encoding/CodePage/UTF-32.h>
#include <catch2/catch_all.hpp>
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
				    const auto span = std::as_bytes(std::span(&result.Result, 1));
				    REQUIRE(std::memcmp(span.data(), "\x62\x4B\x02\x00", 4) == 0);
			    }
			    else
			    {
				    REQUIRE(false);
			    }
		    });

		CodePage::CodePageTrait<CodePage::Utf32LittleEndian>::ToCodePoint(
		    U'\x24B62', [](auto const& result) {
			    if constexpr (GetEncodingResultCode<decltype(result)> == EncodingResultCode::Accept)
			    {
				    REQUIRE(result.Result == 0x24B62);
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
				    const auto span = std::as_bytes(std::span(&result.Result, 1));
				    REQUIRE(std::memcmp(span.data(), "\x00\x02\x4B\x62", 4) == 0);
			    }
			    else
			    {
				    REQUIRE(false);
			    }
		    });

		CodePage::CodePageTrait<CodePage::Utf32BigEndian>::ToCodePoint(
		    U'\x624B0200', [](auto const& result) {
			    if constexpr (GetEncodingResultCode<decltype(result)> == EncodingResultCode::Accept)
			    {
				    REQUIRE(result.Result == 0x24B62);
			    }
			    else
			    {
				    REQUIRE(false);
			    }
		    });
	}
}
