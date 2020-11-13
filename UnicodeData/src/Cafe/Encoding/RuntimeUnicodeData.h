#pragma once
#include <Cafe/Encoding/CodePage.h>
#include <Cafe/Encoding/CodePage/UTF-8.h>
#include <Cafe/Encoding/Strings.h>
#include <Cafe/Misc/Export.h>
#include <Cafe/Misc/Ratio.h>

#include <optional>

#include "UnicodeDataTypes.h"

namespace Cafe::Encoding::RuntimeUnicodeData
{
	CAFE_PUBLIC bool IsInUnicodeData(CodePointType codePoint) noexcept;

	CAFE_PUBLIC std::optional<UnicodeData> GetUnicodeData(CodePointType codePoint) noexcept;
} // namespace Cafe::Encoding::RuntimeUnicodeData
