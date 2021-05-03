#include <cassert>
#include <fstream>
#include <iostream>
#include <regex>
#include <string>

constexpr const char Prologue[] =
    R"(// This file is generated from ftp://ftp.unicode.org/Public/UNIDATA/UnicodeData.txt
// DO NOT EDIT!

#ifndef UNICODE_DEFINE
#define UNICODE_DEFINE(codeValue, characterName, generalCategory, canonicalCombiningClasses, bidirectionalCategory, \
	characterDecompositionMapping, decimalDigitValue, digitValue, numeric, mirrored, unicodeName, commentField, \
	uppercaseMapping, lowercaseMapping, titlecaseMapping)
#endif

#ifndef DECOMPOSITON
#define DECOMPOSITON(tag, ...)
#endif

)";

constexpr const char Epilogue[] =
    R"(
#undef UNICODE_DEFINE
#undef DECOMPOSITON
)";

int main(int argc, char** argv)
{
	if (argc != 3)
	{
		std::cerr << "Invalid arguments";
		return -1;
	}

	const std::string input{ argv[1] }, output{ argv[2] };
	std::ifstream inputFile{ input };
	if (!inputFile)
	{
		std::cerr << "Cannot open input file \"" << input << "\"";
		return -2;
	}

	std::ofstream outputFile{ output };
	if (!outputFile)
	{
		std::cerr << "Cannot open output file \"" << output << "\"";
		return -3;
	}

	outputFile << Prologue;
	std::string line;
	const std::regex pattern{
		R"(([^;\r\n]*);([^;]*);([^;]*);([^;]*);([^;]*);([^;]*);([^;]*);([^;]*);([^;]*);([^;]*);([^;]*);([^;]*);([^;]*);([^;]*);([^;\r\n]*))"
	};
	const std::regex decompositionMappingPattern{ R"(<(\w+)>((?: [\dA-F]{4,6})+))" };
	while (std::getline(inputFile, line))
	{
		std::smatch match;
		const auto succeed = std::regex_search(line, match, pattern);
		assert(succeed);

		const auto codeValue = match[1].str();
		const auto characterName = match[2].str();
		const auto generalCategory = match[3].str();
		const auto canonicalCombiningClasses = match[4].str();
		const auto bidirectionalCategory = match[5].str();
		const auto characterDecompositionMapping = match[6].str();
		const auto decimalDigitValue = match[7].str();
		const auto digitValue = match[8].str();
		const auto numeric = match[9].str();
		const auto mirrored = match[10].str();
		const auto unicodeName = match[11].str();
		const auto commentField = match[12].str();
		const auto uppercaseMapping = match[13].str();
		const auto lowercaseMapping = match[14].str();
		const auto titlecaseMapping = match[15].str();

		std::string result = "UNICODE_DEFINE(";
		result.append("0x").append(codeValue).append(", ");
		result.append(1, '\"').append(characterName).append("\", ");
		result.append(generalCategory).append(", ");
		result.append(canonicalCombiningClasses).append(", ");
		result.append(bidirectionalCategory).append(", ");

		std::smatch decompositionMappingMatch;
		const auto succeed2 = std::regex_match(
		    characterDecompositionMapping, decompositionMappingMatch, decompositionMappingPattern);

		if (succeed2)
		{
			result.append("DECOMPOSITON(");
			result.append(decompositionMappingMatch[1].str());
			result.append(std::regex_replace(decompositionMappingMatch[2].str(),
			                                 std::regex{ R"( ([\dA-Z]{4,6}))" }, ", 0x$1"));
			result.append(")");
		}
		else
		{
			assert(characterDecompositionMapping.empty());
		}

		result.append(", ");

		result.append(decimalDigitValue).append(", ");
		result.append(digitValue).append(", ");
		result.append(numeric).append(", ");
		result.append(mirrored).append(", ");
		result.append(1, '\"').append(unicodeName).append("\", ");
		result.append(1, '\"').append(commentField).append("\", ");

		if (!uppercaseMapping.empty())
		{
			result.append("0x").append(uppercaseMapping);
		}

		result.append(", ");

		if (!lowercaseMapping.empty())
		{
			result.append("0x").append(lowercaseMapping);
		}

		result.append(", ");

		if (!titlecaseMapping.empty())
		{
			result.append("0x").append(titlecaseMapping);
		}

		result.append(")");

		outputFile << result << std::endl;
	}

	outputFile << Epilogue;
}
