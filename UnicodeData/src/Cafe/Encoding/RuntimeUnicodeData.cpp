#include <Cafe/Encoding/RuntimeUnicodeData.h>
#include <Cafe/Encoding/UnicodeData.h>

namespace Cafe::Encoding
{
	namespace RuntimeUnicodeData
	{
		bool IsInUnicodeData(CodePointType codePoint) noexcept
		{
			return IsValidUnicodeData(codePoint);
		}

		std::optional<UnicodeData> GetUnicodeData(CodePointType codePoint) noexcept
		{
			if (!IsValidUnicodeData(codePoint))
			{
				return {};
			}

			return Cafe::Encoding::GetUnicodeData(codePoint);
		}
	} // namespace RuntimeUnicodeData
} // namespace Cafe::Encoding
