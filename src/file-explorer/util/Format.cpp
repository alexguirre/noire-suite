#include "Format.h"
#include <array>
#include <cmath>

std::string BytesToHumanReadable(std::size_t numBytes)
{
	// https://ourcodeworld.com/articles/read/713/converting-bytes-to-human-readable-values-kb-mb-gb-tb-pb-eb-zb-yb-with-javascript
	static const std::array Units{ "bytes", "KB", "MB", "GB", "TB", "PB", "EB", "ZB", "YB" };

	const std::size_t i = static_cast<std::size_t>(std::floor(std::log(numBytes) / std::log(1024)));
	const double convertedBytes = (numBytes / std::pow(1024.0, i));

	std::array<char, 256> buffer{};
	std::snprintf(buffer.data(), buffer.size(), "%.0f %s", convertedBytes, Units[i]);

	return { buffer.data() };
}