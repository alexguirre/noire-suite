#include "ShaderProgramsDevice.h"
#include "FileSystem.h"
#include "Hash.h"
#include <array>
#include <charconv>
#include <gsl/gsl>
#include <string_view>
#include <unordered_set>

namespace noire::fs
{
	// Wrapper stream to treat shader program files as a contiguous chunk of memory, although the
	// vertex shader data and pixel shader data are not contiguous in most cases (if not all).
	// TODO: this may not be the best approach, especially when we start writing to the files.
	class CShaderProgramStream : public IFileStream
	{
	public:
		CShaderProgramStream(IFileStream* baseStream, const SShaderProgramEntry& entry)
			: mVertexShaderStream{ baseStream, entry.VertexShaderOffset, entry.VertexShaderSize },
			  mPixelShaderStream{ baseStream, entry.PixelShaderOffset, entry.PixelShaderSize },
			  mSize{ entry.VertexShaderSize + entry.PixelShaderSize },
			  mReadingOffset{ 0 }
		{
		}

		void Read(void* destBuffer, FileStreamSize count) override
		{
			Expects(destBuffer);
			Expects(count <= (mSize - mReadingOffset));

			const FileStreamSize vertexShaderStreamSize = mVertexShaderStream.Size();
			if (mReadingOffset < vertexShaderStreamSize &&
				mReadingOffset + count > vertexShaderStreamSize)
			{
				// in case of reading past the end of the vertex shader stream, split the Read
				const FileStreamSize toReadFromVertexStream =
					vertexShaderStreamSize - mReadingOffset;
				const FileStreamSize toReadFromPixelStream =
					(mReadingOffset + count) - vertexShaderStreamSize;

				Read(destBuffer, toReadFromVertexStream);
				Read(reinterpret_cast<char*>(destBuffer) + toReadFromVertexStream,
					 toReadFromPixelStream);
			}
			else
			{
				// get which stream we should read
				IFileStream& stream = mReadingOffset < vertexShaderStreamSize ?
										  mVertexShaderStream :
										  mPixelShaderStream;
				const FileStreamSize readingOffset = mReadingOffset < vertexShaderStreamSize ?
														 mReadingOffset :
														 mReadingOffset - vertexShaderStreamSize;

				const FileStreamSize oldPos = stream.Tell();

				stream.Seek(readingOffset);
				stream.Read(destBuffer, count);
				mReadingOffset += count;

				stream.Seek(oldPos);
			}
		}

		void Seek(FileStreamSize offset) override
		{
			Expects(offset < mSize);
			mReadingOffset = offset;
		}

		FileStreamSize Tell() override { return mReadingOffset; }

		FileStreamSize Size() override { return mSize; }

	private:
		CSubFileStream mVertexShaderStream;
		CSubFileStream mPixelShaderStream;
		FileStreamSize mSize;
		FileStreamSize mReadingOffset;
	};

	CShaderProgramsDevice::CShaderProgramsDevice(IDevice& parentDevice, SPathView programsFilePath)
		: mParent{ parentDevice },
		  mProgramsFilePath{ (Expects(mParent.FileExists(programsFilePath)), programsFilePath) },
		  mProgramsFileStream{ mParent.OpenFile(mProgramsFilePath) },
		  mProgramsFile{ *mProgramsFileStream },
		  mEntries{}
	{
		CreateEntries();
	}

	bool CShaderProgramsDevice::PathExists(SPathView path) const
	{
		if (path.IsFile())
		{
			std::uint32_t nameHash;
			const auto [_, ec] = std::from_chars(path.String().data(),
												 path.String().data() + path.String().size(),
												 nameHash,
												 16);
			Expects(ec == std::errc{} || ec == std::errc::invalid_argument);
			const std::uint32_t nameHash2 = crc32(path.String());

			// check if any entry path is equal to the path or contains it
			return std::any_of(mProgramsFile.Entries().begin(),
							   mProgramsFile.Entries().end(),
							   [nameHash, nameHash2](const SShaderProgramEntry& e) {
								   return e.NameHash == nameHash || e.NameHash == nameHash2;
							   });
		}
		else
		{
			for (auto& e : mEntries)
			{
				if (e.Type != EDirectoryEntryType::File && e.Path == path)
				{
					return true;
				}
			}

			return false;
		}
	}

	bool CShaderProgramsDevice::FileExists(SPathView filePath) const
	{
		Expects(filePath.IsFile());

		return PathExists(filePath);
	}

	bool CShaderProgramsDevice::DirectoryExists(SPathView dirPath) const
	{
		if (dirPath.IsEmpty()) // empty path represent root of this device
		{
			return true;
		}

		Expects(dirPath.IsDirectory());

		return PathExists(dirPath);
	}

	FileStreamSize CShaderProgramsDevice::FileSize(SPathView filePath)
	{
		Expects(FileExists(filePath));

		std::uint32_t nameHash;
		const auto [_, ec] = std::from_chars(filePath.String().data(),
											 filePath.String().data() + filePath.String().size(),
											 nameHash,
											 16);
		Expects(ec == std::errc{} || ec == std::errc::invalid_argument);
		const std::uint32_t nameHash2 = crc32(filePath.String());

		auto it = std::find_if(mProgramsFile.Entries().begin(),
							   mProgramsFile.Entries().end(),
							   [nameHash, nameHash2](const SShaderProgramEntry& e) {
								   return e.NameHash == nameHash || e.NameHash == nameHash2;
							   });
		return static_cast<FileStreamSize>(it->VertexShaderSize) + it->PixelShaderSize;
	}

	std::unique_ptr<IFileStream> CShaderProgramsDevice::OpenFile(SPathView path)
	{
		std::uint32_t nameHash;
		const auto [_, ec] = std::from_chars(path.String().data(),
											 path.String().data() + path.String().size(),
											 nameHash,
											 16);
		Expects(ec == std::errc{} || ec == std::errc::invalid_argument);
		const std::uint32_t nameHash2 = crc32(path.String());

		auto it = std::find_if(mProgramsFile.Entries().begin(),
							   mProgramsFile.Entries().end(),
							   [nameHash, nameHash2](const SShaderProgramEntry& e) {
								   return e.NameHash == nameHash || e.NameHash == nameHash2;
							   });

		if (it != mProgramsFile.Entries().end())
		{
			const SShaderProgramEntry& entry = *it;
			return std::make_unique<CShaderProgramStream>(mProgramsFileStream.get(), entry);
		}
		else
		{
			return nullptr;
		}
	}

	std::vector<SDirectoryEntry> CShaderProgramsDevice::GetAllEntries() { return mEntries; }

	std::vector<SDirectoryEntry> CShaderProgramsDevice::GetEntries(SPathView dirPath)
	{
		Expects(DirectoryExists(dirPath));

		std::vector<SDirectoryEntry> entries{};
		for (auto& e : mEntries)
		{
			if (!e.Path.IsEmpty() && e.Path.Parent() == dirPath)
			{
				entries.emplace_back(this, e.Path, e.Type);
			}
		}

		return entries;
	}

	void CShaderProgramsDevice::CreateEntries()
	{
		mEntries.emplace_back(this, SPathView{}, EDirectoryEntryType::Collection); // root

		std::unordered_set<std::string_view> dirs{};
		const auto getDirectoriesFromPath = [](SPathView path,
											   std::unordered_set<std::string_view>& directories) {
			SPathView p = path.IsFile() ? path.Parent() : path;
			while (!p.IsEmpty() && !p.IsRoot())
			{
				directories.emplace(p.String());
				p = p.Parent();
			}
		};

		for (auto& e : mProgramsFile.Entries())
		{
			std::string str = CHashDatabase::Instance().GetString(e.NameHash);
			mEntries.emplace_back(this, std::move(str), EDirectoryEntryType::File);
			getDirectoriesFromPath(mEntries.back().Path, dirs);
		}

		for (std::string_view d : dirs)
		{
			mEntries.emplace_back(this, d, EDirectoryEntryType::Directory);
		}
	}
}