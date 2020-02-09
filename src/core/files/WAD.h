#pragma once
#include "Common.h"
#include "File.h"
#include "devices/Device.h"
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace noire
{
	struct WADEntry
	{
		std::string Path;
		u32 PathHash;
		size Offset;
		size Size;

		inline WADEntry(std::string path, u32 pathHash, size offset, size size)
			: Path{ std::move(path) }, PathHash{ pathHash }, Offset{ offset }, Size{ size }
		{
		}
	};

	class WAD : public File, Device
	{
	public:
		WAD(std::shared_ptr<Stream> input);

		bool Exists(PathView path) const override;
		std::shared_ptr<Stream> Open(PathView path) override;
		std::shared_ptr<Stream> Create(PathView path) override;
		bool Delete(PathView path) override;

		void Load() override;
		void Save(Stream& output) override;

	private:
		std::vector<WADEntry> mEntries;

	public:
		static constexpr u32 HeaderMagic{ 0x01444157 }; // WAD\01
	};
}
