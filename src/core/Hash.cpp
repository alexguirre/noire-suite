#include "Hash.h"
#include <algorithm>
#include <array>
#include <charconv>
#include <doctest/doctest.h>
#include <fstream>

namespace noire
{
	HashLookup::HashLookup(const std::filesystem::path& path, bool caseSensitive)
		: mHashToStr{}, mCaseSensitive{ caseSensitive }
	{
		Load(path);
	}

	inline static constexpr size HashStrLength{ HashLookup::HashPrefix.size() + 8 +
												HashLookup::HashSuffix.size() };

	std::string HashLookup::TryGetString(u32 hash) const
	{
		auto it = mHashToStr.find(hash);
		if (it == mHashToStr.end())
		{
			std::array<char, 8 + 1> hashStr{};
			const auto [p, ec] =
				std::to_chars(hashStr.data(), hashStr.data() + hashStr.size(), hash, 16);
			Expects(ec == std::errc{});

			std::string str;
			str.resize(HashStrLength, '0');
			std::copy(HashPrefix.begin(), HashPrefix.end(), str.data());
			const size hashStrSize = p - hashStr.data();
			for (size i = 0; i < hashStrSize; ++i)
			{
				char c = hashStr[i];
				if (c >= 'a' && c <= 'z') // to upper
				{
					c -= ('a' - 'A');
				}

				str[HashPrefix.size() + 8 - hashStrSize + i] = c;
			}
			std::copy(HashSuffix.begin(), HashSuffix.end(), str.data() + HashPrefix.size() + 8);

			return str;
		}
		else
		{
			return it->second;
		}
	}

	std::optional<std::string> HashLookup::GetString(u32 hash) const
	{
		auto it = mHashToStr.find(hash);
		return it == mHashToStr.end() ? std::nullopt : std::make_optional(it->second);
	}

	u32 HashLookup::GetHash(std::string_view str) const
	{
		if (str.size() == HashStrLength)
		{
			if (str.substr(0, HashPrefix.size()) == HashPrefix &&
				str.substr(HashStrLength - HashSuffix.size(), HashSuffix.size()) == HashSuffix)
			{
				const std::string_view hashStr = str.substr(HashPrefix.size(), 8);
				u32 v;
				const auto [p, ec] =
					std::from_chars(hashStr.data(), hashStr.data() + hashStr.size(), v, 16);
				Expects(ec == std::errc{});

				return v;
			}
		}

		return crc32(str);
	}

	void HashLookup::Load(const std::filesystem::path& dbPath)
	{
		auto fp = std::filesystem::absolute(dbPath);
		if (!std::filesystem::is_regular_file(fp))
		{
			return;
		}

		std::ifstream f{ dbPath, std::ios::in };

		if (mCaseSensitive)
		{
			for (std::string line; std::getline(f, line);)
			{
				u32 hash = crc32(line);
				mHashToStr.try_emplace(hash, line);

				for (auto& c : line)
				{
					c = static_cast<char>(std::tolower(c));
				}
				hash = crc32(line);
				mHashToStr.try_emplace(hash, std::move(line));
			}
		}
		else
		{
			for (std::string line; std::getline(f, line);)
			{
				u32 hash = crc32Lowercase(line);
				mHashToStr.try_emplace(hash, std::move(line));
			}
		}
	}

	const HashLookup& HashLookup::Instance(bool caseSensitive)
	{
		// TODO: don't hardcode HashLookup path
		if (caseSensitive)
		{
			static HashLookup inst{ "hashes.db", true };
			return inst;
		}
		else
		{
			static HashLookup inst{ "hashes.db", false };
			return inst;
		}
	}
}

TEST_SUITE("crc32")
{
	using namespace noire;

	TEST_CASE("case sensitive")
	{
		CHECK_EQ(0x00000000, crc32(""));
		CHECK_EQ(0x8B8838C2, crc32("abcdxyz"));
		CHECK_EQ(0xB4CDC6D8, crc32("ABCDXYZ"));
		CHECK_EQ(0xFC1BD0B1, crc32("AaBbCcDdXxYyZz"));
	}

	TEST_CASE("lowercase")
	{
		CHECK_EQ(0x00000000, crc32Lowercase(""));
		CHECK_EQ(0x8B8838C2, crc32Lowercase("abcdxyz"));
		CHECK_EQ(0x8B8838C2, crc32Lowercase("ABCDXYZ"));
		CHECK_EQ(0xAD7F9CBD, crc32Lowercase("AaBbCcDdXxYyZz"));
	}
}

TEST_SUITE("HashLookup")
{
	using namespace noire;

	TEST_CASE("get non-existing hash string")
	{
		HashLookup h{ "", true };

		CHECK_EQ("?#00000000#?", h.TryGetString(0x00000000));
		CHECK_EQ("?#00000001#?", h.TryGetString(0x00000001));
		CHECK_EQ("?#00001000#?", h.TryGetString(0x00001000));
		CHECK_EQ("?#8B8838C2#?", h.TryGetString(0x8B8838C2));
		CHECK_EQ("?#B4CDC6D8#?", h.TryGetString(0xB4CDC6D8));
		CHECK_EQ("?#FC1BD0B1#?", h.TryGetString(0xFC1BD0B1));

		CHECK_EQ(0x00000000, h.GetHash("?#00000000#?"));
		CHECK_EQ(0x00000001, h.GetHash("?#00000001#?"));
		CHECK_EQ(0x00001000, h.GetHash("?#00001000#?"));
		CHECK_EQ(0x8B8838C2, h.GetHash("?#8B8838C2#?"));
		CHECK_EQ(0xB4CDC6D8, h.GetHash("?#B4CDC6D8#?"));
		CHECK_EQ(0xFC1BD0B1, h.GetHash("?#FC1BD0B1#?"));
	}
}
