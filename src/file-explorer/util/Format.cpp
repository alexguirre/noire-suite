#include "Format.h"
#include <array>
#include <cmath>
#include <gsl/gsl>

std::string BytesToHumanReadable(std::uintmax_t numBytes)
{
	// same as how Windows' File Explorer calculates the file size in KB
	const std::uintmax_t kiloBytes = gsl::narrow_cast<std::uintmax_t>(std::ceil(numBytes / 1024.0));

	std::array<char, 256> buffer{};
	const int len = std::snprintf(buffer.data(), buffer.size(), "%ju KB", kiloBytes);
	Expects(len >= 0);

	std::string str{ buffer.data(), gsl::narrow_cast<std::size_t>(len) };
	if (str.size() > 6)
	{
		// insert thousands separator
		for (std::ptrdiff_t i = str.size() - 6; i > 0; i -= 3)
		{
			str.insert(i, 1, ',');
		}
	}

	return str;
}