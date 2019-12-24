#ifndef CAFE_ENCODING_CODEPAGE_H
#define CAFE_ENCODING_CODEPAGE_H

#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace Cafe::Encoding
{
	namespace CodePage
	{
		/// @brief  代码页类型
		/// @remark 文档中的“编码”皆指代码页，但存在多个代码页对应一个编码的不同表示
		enum class CodePageType : std::uint16_t
		{
		};

		/// @brief  假想代码页，表示 Unicode 码点
		constexpr CodePageType CodePoint = static_cast<CodePageType>(0);

		/// @brief  代码页特征
		/// @remark 包含代码页的各种属性及转换方式
		/// @see    查看 CodePageTrait<Utf8> 特化以了解编写约定
		template <CodePageType CodePageValue>
		struct CodePageTrait;

		/// @brief  获取以指定代码页能表示单个任意码点的最大宽度
		/// @remark 由于定长代码页不要求提供 MaxWidth 成员因此提供此函数简单获取
		template <CodePageType CodePageValue>
		constexpr std::size_t GetMaxWidth() noexcept
		{
			if constexpr (CodePageTrait<CodePageValue>::IsVariableWidth)
			{
				return CodePageTrait<CodePageValue>::MaxWidth;
			}
			else
			{
				return 1;
			}
		}
	} // namespace CodePage

	/// @brief  码点类型
	/// @remark Unicode 中码点范围为 [0, 0x10FFFF]，因此一个 std::uint32_t 总能容纳任何单个码点
	using CodePointType = std::uint32_t;

	/// @brief  最大码点的值
	constexpr CodePointType MaxValidCodePoint = 0x10FFFF;
} // namespace Cafe::Encoding

#endif

#ifdef CAFE_CODEPAGE
CAFE_CODEPAGE(::Cafe::Encoding::CodePage::CodePoint)
#endif
