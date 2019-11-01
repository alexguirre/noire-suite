#pragma once
#include "Device.h"
#include "FileStream.h"
#include "WADFile.h"
#include <memory>
#include <string>
#include <string_view>

namespace noire::fs
{
	class CWADDevice : public IDevice
	{
	public:
		CWADDevice(IDevice& parentDevice, std::string_view wadFilePath);

		CWADDevice(const CWADDevice&) = delete;
		CWADDevice& operator=(const CWADDevice&) = delete;
		CWADDevice(CWADDevice&&) = default;
		CWADDevice& operator=(CWADDevice&&) = default;

		bool PathExists(std::string_view path) const override;
		std::unique_ptr<IFileStream> OpenFile(std::string_view path) override;

	private:
		IDevice& mParent;
		std::string mWADFilePath;
		std::unique_ptr<IFileStream> mWADFileStream;
		WADFile mWADFile;
	};
}
