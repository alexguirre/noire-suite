#include "App.h"
#include "windows/AttributeWindow.h"
#include "windows/ImageWindow.h"
#include "windows/MainWindow.h"
#include "windows/ShaderProgramWindow.h"
#include <IL/il.h>
#include <formats/AttributeFile.h>
#include <formats/ContainerFile.h>
#include <formats/File.h>
#include <formats/Hash.h>
#include <formats/ShaderProgramFile.h>
#include <formats/WADFile.h>
#include <formats/fs/ContainerDevice.h>
#include <formats/fs/FileStream.h>
#include <formats/fs/NativeDevice.h>
#include <formats/fs/ShaderProgramsDevice.h>
#include <formats/fs/TrunkDevice.h>
#include <formats/fs/WADDevice.h>
#include <processthreadsapi.h>
#include <thread>

using namespace noire;
using namespace noire::fs;

wxIMPLEMENT_APP(CApp);

wxDEFINE_EVENT(EVT_FILE_SYSTEM_SCAN_STARTED, wxThreadEvent);
wxDEFINE_EVENT(EVT_FILE_SYSTEM_SCAN_COMPLETED, wxThreadEvent);

CApp::CApp() : mFileSystem{ nullptr }, mMainWindow{ nullptr } {}

bool CApp::OnInit()
{
	ilInit();

	mMainWindow = new CMainWindow;
	mMainWindow->Show();

	return true;
}

int CApp::OnExit()
{
	ilShutDown();

	return 0;
}

void CApp::ChangeRootPath(const std::filesystem::path& path)
{
	// TODO: remember last opened folder after closing the application

	std::thread t{ [this, path]() {
		wxQueueEvent(this, new wxThreadEvent(EVT_FILE_SYSTEM_SCAN_STARTED));

		std::unique_ptr fs = std::make_unique<CFileSystem>();

		// CContainerDevice
		fs->RegisterDeviceType(&TFileTraits<CContainerFile>::IsValid,
							   [](CFileSystem&, IDevice& d, SPathView relPath) {
								   return std::make_unique<CContainerDevice>(d, relPath);
							   });

		// CWADDevice
		fs->RegisterDeviceType(&TFileTraits<WADFile>::IsValid,
							   [](CFileSystem&, IDevice& d, SPathView relPath) {
								   return std::make_unique<CWADDevice>(d, relPath);
							   });

		// CTrunkDevice
		fs->RegisterDeviceType(&TFileTraits<CTrunkFile>::IsValid,
							   [](CFileSystem&, IDevice& d, SPathView relPath) {
								   return std::make_unique<CTrunkDevice>(d, relPath);
							   });

		// CShaderProgramsDevice
		fs->RegisterDeviceType(&TFileTraits<CShaderProgramsFile>::IsValid,
							   [](CFileSystem&, IDevice& d, SPathView relPath) {
								   return std::make_unique<CShaderProgramsDevice>(d, relPath);
							   });

		fs->EnableDeviceScanning(true);
		fs->Mount("/", std::make_unique<CNativeDevice>(path));

		mFileSystem = std::move(fs);
		wxQueueEvent(this, new wxThreadEvent(EVT_FILE_SYSTEM_SCAN_COMPLETED));
	} };

	SetThreadPriority(t.native_handle(), THREAD_PRIORITY_TIME_CRITICAL);
	t.detach();
}

static wxImage CreateImageFromDDS(gsl::span<std::byte> ddsData)
{
	ILuint imgId = ilGenImage();
	ilBindImage(imgId);

	ilLoadL(IL_DDS, ddsData.data(), ddsData.size_bytes());

	ilConvertImage(IL_RGBA, IL_UNSIGNED_BYTE);

	ILint width = ilGetInteger(IL_IMAGE_WIDTH);
	ILint height = ilGetInteger(IL_IMAGE_HEIGHT);
	ILubyte* data = ilGetData();
	// ILint dataSize = ilGetInteger(IL_IMAGE_SIZE_OF_DATA);
	// copies allocated with malloc since wxImage will take ownership of them
	ILubyte* rgbCopy = reinterpret_cast<ILubyte*>(std::malloc(width * height * 3));
	ILubyte* alphaCopy = reinterpret_cast<ILubyte*>(std::malloc(width * height * 1));

	// TODO: figure out why with `/final/pc/out.wad.pc/out/graphicsdata/dirt.dds` this precondition
	// isn't met
	// Expects(dataSize == (width * height * 4));

	for (ILint y = 0; y < height; y++)
	{
		for (ILint x = 0; x < width; x++)
		{
			const std::size_t dataOffset = (x + y * width) * 4;

			ILubyte r = data[dataOffset + 0];
			ILubyte g = data[dataOffset + 1];
			ILubyte b = data[dataOffset + 2];
			ILubyte a = data[dataOffset + 3];

			const std::size_t rgbOffset = (x + y * width) * 3;
			rgbCopy[rgbOffset + 0] = r;
			rgbCopy[rgbOffset + 1] = g;
			rgbCopy[rgbOffset + 2] = b;

			const std::size_t alphaOffset = (x + y * width) * 1;
			alphaCopy[alphaOffset + 0] = a;
		}
	}

	// wxImage takes ownership of rgbCopy and alphaCopy
	wxImage img{ width, height, rgbCopy, alphaCopy };

	ilDeleteImage(imgId);
	return img;
}

bool CApp::OpenDDSFile(SPathView filePath)
{
	std::unique_ptr<IFileStream> file = mFileSystem->OpenFile(filePath);

	constexpr std::uint32_t DDSHeaderMagic{ 0x20534444 }; // 'DDS '
	const std::uint32_t headerMagic = file->Read<std::uint32_t>();

	if (headerMagic == DDSHeaderMagic)
	{
		const std::size_t size = file->Size();

		std::unique_ptr<std::byte[]> buffer = std::make_unique<std::byte[]>(size);
		file->Seek(0);
		file->Read(buffer.get(), size);

		const wxImage img = CreateImageFromDDS({ buffer.get(), gsl::narrow<std::ptrdiff_t>(size) });
		CImageWindow* imgWin =
			new CImageWindow(mMainWindow,
							 wxID_ANY,
							 { filePath.String().data(), filePath.String().size() },
							 img);
		imgWin->Show();
		return true;
	}

	return false;
}

bool CApp::OpenShaderProgramFile(SPathView filePath)
{
	std::unique_ptr<IFileStream> file = mFileSystem->OpenFile(filePath);

	if (TFileTraits<CShaderProgramFile>::IsValid(*file))
	{
		wxString title =
			"Shader Program - " + wxString{ filePath.String().data(), filePath.String().size() };
		CShaderProgramWindow* shaderWin =
			new CShaderProgramWindow(mMainWindow,
									 wxID_ANY,
									 title,
									 std::make_unique<CShaderProgramFile>(*file));
		shaderWin->Show();
		return true;
	}

	return false;
}

bool CApp::OpenAttributeFile(SPathView filePath)
{
	std::unique_ptr<IFileStream> file = mFileSystem->OpenFile(filePath);

	if (TFileTraits<CAttributeFile>::IsValid(*file))
	{
		wxString title =
			"Attribute - " + wxString{ filePath.String().data(), filePath.String().size() };
		CAttributeWindow* atbWin = new CAttributeWindow(mMainWindow,
														wxID_ANY,
														title,
														std::make_unique<CAttributeFile>(*file));
		atbWin->Show();
		return true;
	}

	return false;
}

bool CApp::OpenUniqueTextureVRamFile(SPathView filePath)
{
	auto* fs = wxGetApp().FileSystem();
	if (filePath.Name() == "uniquetexturevram")
	{
		// TODO: move this loading somewhere else, possibly to noire-formats
		// TODO: show all textures in the same window
		SPath mainPath = filePath.Parent() / "uniquetexturemain";
		Expects(fs->FileExists(mainPath)); // TODO: 'uniquetexturemain' may not exist along
										   // with a 'uniquetexturevram' file
		std::unique_ptr<IFileStream> mainStream = fs->OpenFile(mainPath);
		std::unique_ptr<IFileStream> vramStream = fs->OpenFile(filePath);
		const std::size_t vramStreamSize = vramStream->Size();

		mainStream->Read<std::uint32_t>(); // these 4 bytes are used by the game at runtime to
										   // indicate if it already loaded the texture, the file
										   // should always have 0 here

		// read entries
		const std::uint32_t textureCount = mainStream->Read<std::uint32_t>();
		struct TextureEntry
		{
			std::uint32_t Offset;
			std::uint32_t UnkZero;
			std::uint32_t NameHash;
		};
		std::vector<TextureEntry> entries{};
		entries.reserve(textureCount);
		for (std::size_t i = 0; i < textureCount; i++)
		{
			entries.emplace_back(mainStream->Read<TextureEntry>());
		}

		// open a window for each texture
		for (std::size_t i = 0; i < entries.size(); i++)
		{
			const TextureEntry& e = entries[i];
			// this expects the entries to be sorted by offset, not sure if that is always the case
			const std::size_t size = (i < (entries.size() - 1)) ?
										 (entries[i + 1].Offset - e.Offset) :
										 (vramStreamSize - e.Offset);
			std::unique_ptr<std::byte[]> buffer = std::make_unique<std::byte[]>(size);
			vramStream->Seek(e.Offset);
			vramStream->Read(buffer.get(), size);

			const wxImage img =
				CreateImageFromDDS({ buffer.get(), gsl::narrow<std::ptrdiff_t>(size) });

			std::string title = noire::CHashDatabase::Instance().TryGetString(e.NameHash);
			title += " | " + std::string{ filePath.String() };
			CImageWindow* imgWin = new CImageWindow(mMainWindow, wxID_ANY, title, img);
			imgWin->Show();
		}
		return true;
	}

	return false;
}

bool CApp::OpenFile(SPathView filePath)
{
	return OpenUniqueTextureVRamFile(filePath) || OpenShaderProgramFile(filePath) ||
		   OpenAttributeFile(filePath) || OpenDDSFile(filePath);
}
