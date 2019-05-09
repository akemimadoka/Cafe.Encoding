#include <Cafe/Encoding/Strings.h>
#include <catch2/catch.hpp>
#include <map>
#include <unordered_map>

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
		REQUIRE(str == str);
		const auto pattern = CAFE_UTF8_SV("ab");
		REQUIRE(pattern.GetSize() == 3);

		const auto foundPos = str.Find(pattern);
		REQUIRE(foundPos == 0);

		auto str2 = str.ToString();
		REQUIRE(str2 == str);
		REQUIRE(str == str2);
		REQUIRE(str2 == str2);
		str2.Append(pattern);
		REQUIRE(str2 == CAFE_UTF8_SV("ababcab"));

		str2.Insert(str2.begin() + 1, CAFE_UTF8_SV("123"));
		REQUIRE(str2 == CAFE_UTF8_SV("a123babcab"));

		str2.Remove(str2.begin() + 2, 3);
		REQUIRE(str2 == CAFE_UTF8_SV("a1abcab"));

		{
			std::unordered_map<StringView<CodePage::Utf8>, String<CodePage::Utf8>> map;
			map.emplace(CAFE_UTF8_SV("abc"), CAFE_UTF8_SV("def"));
			map.emplace(CAFE_UTF8_SV("def"), CAFE_UTF8_SV("abc"));
			const auto [_, succeed] = map.try_emplace(CAFE_UTF8_SV("abc"), CAFE_UTF8_SV("ghi"));
			REQUIRE(!succeed);
			REQUIRE(map.size() == 2);
			const auto iter = map.find(CAFE_UTF8_SV("abc"));
			REQUIRE(iter != map.end());
			REQUIRE(iter->second == CAFE_UTF8_SV("def"));
		}

		{
			std::unordered_map<String<CodePage::Utf8>, StringView<CodePage::Utf8>> map;
			map.emplace(CAFE_UTF8_SV("abc"), CAFE_UTF8_SV("def"));
			map.emplace(CAFE_UTF8_SV("def"), CAFE_UTF8_SV("abc"));
			const auto [_, succeed] = map.try_emplace(CAFE_UTF8_SV("abc"), CAFE_UTF8_SV("ghi"));
			REQUIRE(!succeed);
			REQUIRE(map.size() == 2);
			const auto iter = map.find(CAFE_UTF8_SV("abc"));
			REQUIRE(iter != map.end());
			REQUIRE(iter->second == CAFE_UTF8_SV("def"));
		}

		{
			std::map<StringView<CodePage::Utf8>, String<CodePage::Utf8>> map;
			map.emplace(CAFE_UTF8_SV("abc"), CAFE_UTF8_SV("def"));
			map.emplace(CAFE_UTF8_SV("def"), CAFE_UTF8_SV("abc"));
			const auto [_, succeed] = map.try_emplace(CAFE_UTF8_SV("abc"), CAFE_UTF8_SV("ghi"));
			REQUIRE(!succeed);
			REQUIRE(map.size() == 2);
			const auto iter = map.find(CAFE_UTF8_SV("abc"));
			REQUIRE(iter != map.end());
			REQUIRE(iter->second == CAFE_UTF8_SV("def"));
		}
	}
}
#endif
