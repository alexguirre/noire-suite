#pragma once
#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>
#include <unordered_map>

namespace noire
{
	std::uint32_t crc32Partial(std::string_view str, std::uint32_t inHash = -1);
	std::uint32_t crc32(std::string_view str, std::uint32_t inHash = -1);
	std::uint32_t crc32LowercasePartial(std::string_view str, std::uint32_t inHash = -1);
	std::uint32_t crc32Lowercase(std::string_view str, std::uint32_t inHash = -1);

	class CHashDatabase
	{
	public:
		CHashDatabase(const std::filesystem::path& dbPath, bool caseSensitive);

		/// Tries to translate the hash to a string. If no translation is found, the hash converted
		/// to a hexadecimal string (without '0x' prefix) is returned.
		std::string TryGetString(std::uint32_t hash) const;

		static const CHashDatabase& Instance(bool caseSensitive = true);

	private:
		void Load(const std::filesystem::path& dbPath);

		std::unordered_map<std::uint32_t, std::string> mHashToStr;
		bool mCaseSensitive;
	};
}
