#include "Format.h"
#include <array>
#include <cmath>
#include <gsl/gsl>

namespace noire::explorer
{
	std::string BytesToHumanReadable(u64 numBytes)
	{
		// same as how Windows' File Explorer calculates the file size in KB
		const u64 kiloBytes = gsl::narrow_cast<u64>(std::ceil(numBytes / 1024.0));

		std::array<char, 256> buffer{};
		const int len = std::snprintf(buffer.data(), buffer.size(), "%ju KB", kiloBytes);
		Expects(len >= 0);

		std::string str{ buffer.data(), gsl::narrow_cast<size>(len) };
		if (str.size() > 6)
		{
			// insert thousands separator
			for (ptrdiff i = str.size() - 6; i > 0; i -= 3)
			{
				str.insert(i, 1, ',');
			}
		}

		return str;
	}
}
