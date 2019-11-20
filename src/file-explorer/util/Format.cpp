#include "Format.h"
#include <array>
#include <cmath>
#include <gsl/gsl>

std::string BytesToHumanReadable(std::uintmax_t numBytes)
{
	// special cases
	if (numBytes == 0)
	{
		// default case gets i == -1 if numBytes == 0, causing to go out-of-bounds when reading
		// Units array
		return "0 bytes";
	}
	else if (numBytes == 1)
	{
		// needs singular form
		return "1 byte";
	}
	else // default case
	{
		// https://ourcodeworld.com/articles/read/713/converting-bytes-to-human-readable-values-kb-mb-gb-tb-pb-eb-zb-yb-with-javascript
		static const std::array Units{ "bytes", "KB", "MB", "GB", "TB", "PB", "EB", "ZB", "YB" };

		const std::size_t i =
			static_cast<std::size_t>(std::floor(std::log(numBytes) / std::log(1024)));
		const std::uintmax_t bytesInUnits =
			static_cast<std::uintmax_t>(numBytes / std::pow(1024.0, i));

		std::array<char, 256> buffer{};
		std::snprintf(buffer.data(), buffer.size(), "%ju %s", bytesInUnits, Units[i]);

		return { buffer.data() };
	}
}