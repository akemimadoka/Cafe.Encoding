#pragma once

#include <cstddef>
#include <cstdint>

namespace Cafe::Encoding
{
	namespace CodePage
	{
		enum class CodePageType : std::uint16_t
		{
		};

		// 假想代码页，表示 Unicode 码点
		// 虽然 UTF-32 可表示相同的含义，但他们语义并不相同，因此分开
		constexpr CodePageType CodePoint = static_cast<CodePageType>(0);

		template <CodePageType CodePageValue>
		struct CodePageTrait;
	} // namespace CodePage

	// Unicode 中码点范围为 [0, 0x10FFFF]，因此一个 std::uint32_t 总能容纳任何单个码点
	using CodePointType = std::uint32_t;

	constexpr CodePointType MaxValidCodePoint = 0x10FFFF;
} // namespace Cafe::Encoding
