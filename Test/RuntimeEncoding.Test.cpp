#include <Encoding/RuntimeEncoding.h>
#include <catch2/catch.hpp>
#include <cstring>
#include <vector>

using namespace Cafe;
using namespace Encoding;

#if __has_include(<Encoding/CodePage/UTF-8.h>) && __has_include(<Encoding/CodePage/UTF-16.h>)
TEST_CASE("Cafe.Encoding.RuntimeEncoding", "[Encoding][RuntimeEncoding]")
{
	SECTION("RuntimeEncoding test")
	{
		const auto u16StrSpan = gsl::make_span(u"\xD852\xDF62\xFEFF");
		std::vector<std::byte> resultVec;
		RuntimeEncoding::EncodeAll(CodePage::Utf16LittleEndian, gsl::as_bytes(u16StrSpan), CodePage::Utf8,
		                           [&](auto const& result) {
			                           REQUIRE(result.ResultCode == EncodingResultCode::Accept);
			                           resultVec.insert(resultVec.end(), result.Result.begin(),
			                                            result.Result.end());
		                           });
		REQUIRE(std::memcmp(resultVec.data(), u8"\xF0\xA4\xAD\xA2\xEF\xBB\xBF", resultVec.size()) == 0);
	}
}
#endif
