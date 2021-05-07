#include <Cafe/Encoding/Strings.h>
#include <catch2/catch_all.hpp>
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

		REQUIRE(str.Find(pattern) == 0);
		REQUIRE(str.Find(CAFE_UTF8_SV("c")) == 4);

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

		str2.Resize(4);
		REQUIRE(str2 == CAFE_UTF8_SV("a1a"));

		str2.Resize(6);
		REQUIRE(str2 == CAFE_UTF8_SV("a1a\0\0"));

		REQUIRE(str + str2 == CAFE_UTF8_SV("ababca1a\0\0"));
		REQUIRE(str.ToString() + str2 == CAFE_UTF8_SV("ababca1a\0\0"));
		REQUIRE(u8"123"_u8s + str == CAFE_UTF8_SV("123ababc"));
		REQUIRE(str2 + str == CAFE_UTF8_SV("a1a\0\0ababc"));

		String<CodePage::Utf8,
		       std::allocator<typename CodePage::CodePageTrait<CodePage::Utf8>::CharType>, 16>
		    str3 = CAFE_UTF8_SV("abc");
		REQUIRE(!str3.IsDynamicAllocated());

		str3.Append(CAFE_UTF8_SV("abcabcabcabcabc"));
		REQUIRE(str3.IsDynamicAllocated());

		REQUIRE(str3 == CAFE_UTF8_SV("abcabcabcabcabcabc"));

		String<CodePage::Utf8,
		       std::allocator<typename CodePage::CodePageTrait<CodePage::Utf8>::CharType>, 32>
		    str4 = std::move(str3);
		REQUIRE(str3.IsEmpty());
		REQUIRE(str3.IsDynamicAllocated());
		str3.ShrinkToFit();
		REQUIRE(!str3.IsDynamicAllocated());
		REQUIRE(str4 == CAFE_UTF8_SV("abcabcabcabcabcabc"));

		String<CodePage::Utf8> str5(u8"test");
		REQUIRE(str5 == u8"test"_u8sv);

		{
			std::unordered_map<StringView<CodePage::Utf8>, String<CodePage::Utf8>> map;
			map.emplace(CAFE_UTF8_SV("abc"), CAFE_UTF8_SV("def"));
			map.emplace(CAFE_UTF8_SV("def"), CAFE_UTF8_SV("abc"));
			const auto [_, succeed] = map.try_emplace(CAFE_UTF8_SV("abc"), CAFE_UTF8_SV("ghi"));
			REQUIRE(!succeed);
			REQUIRE(map.size() == 2);
			const auto iter = map.find(CAFE_UTF8_SV("abc"));
			const auto b = iter != map.end();
			REQUIRE(b);
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
			const auto b = iter != map.end();
			REQUIRE(b);
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

		{
			std::map<String<CodePage::Utf8>, StringView<CodePage::Utf8>> map;
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
			const auto nullTerminatedStr = u8"Null";
			const auto strV = StringView<CodePage::Utf8>::FromNullTerminatedStr(nullTerminatedStr);
			REQUIRE(strV == CAFE_UTF8_SV("Null"));
			const auto string = String<CodePage::Utf8>::FromNullTerminatedStr(nullTerminatedStr);
			REQUIRE(string == strV);
		}

		{
			constexpr auto constStr = u8"Const"_u8s;
			static_assert(constStr.GetSize() == 6);
			static_assert(constStr == u8"Const"_u8sv);
		}
	}
}
#endif
