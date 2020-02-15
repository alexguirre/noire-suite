#include "App.h"
#include "windows/AttributeWindow.h"
#include "windows/ImageWindow.h"
#include "windows/MainWindow.h"
#include "windows/ShaderProgramWindow.h"
#include <IL/il.h>
#include <core/Common.h>
#include <core/files/RawFile.h>
#include <core/files/WAD.h>
#include <core/streams/FileStream.h>
#include <processthreadsapi.h>
#include <thread>

wxIMPLEMENT_APP(noire::explorer::App);

wxDEFINE_EVENT(EVT_FILE_SYSTEM_SCAN_STARTED, wxThreadEvent);
wxDEFINE_EVENT(EVT_FILE_SYSTEM_SCAN_COMPLETED, wxThreadEvent);

namespace noire::explorer
{
	App::App() : mRootDevice{ nullptr }, mMainWindow{ nullptr } {}

	bool App::OnInit()
	{
		ilInit();

		mMainWindow = new MainWindow;
		mMainWindow->Show();

		return true;
	}

	int App::OnExit()
	{
		ilShutDown();

		return 0;
	}

	void App::ChangeRootPath(const std::filesystem::path& path)
	{
		// TODO: remember last opened folder after closing the application

		std::thread t{ [this, path]() {
			wxQueueEvent(this, new wxThreadEvent(EVT_FILE_SYSTEM_SCAN_STARTED));

			std::unique_ptr d = std::make_unique<MultiDevice>();

			const std::filesystem::path wadPath =
				"E:\\Rockstar Games\\L.A. Noire Complete Edition\\test\\out.wad.pc";
			d->Mount(PathView::Root, std::make_unique<WAD>(std::make_shared<FileStream>(wadPath)));

			mRootDevice = std::move(d);
			wxQueueEvent(this, new wxThreadEvent(EVT_FILE_SYSTEM_SCAN_COMPLETED));
		} };

		SetThreadPriority(t.native_handle(), THREAD_PRIORITY_TIME_CRITICAL);
		t.detach();
	}

	static wxImage CreateImageFromDDS(gsl::span<byte> ddsData)
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

		// TODO: figure out why with `/final/pc/out.wad.pc/out/graphicsdata/dirt.dds` this
		// precondition isn't met Expects(dataSize == (width * height * 4));

		for (ILint y = 0; y < height; y++)
		{
			for (ILint x = 0; x < width; x++)
			{
				const size dataOffset = (x + y * width) * 4;

				ILubyte r = data[dataOffset + 0];
				ILubyte g = data[dataOffset + 1];
				ILubyte b = data[dataOffset + 2];
				ILubyte a = data[dataOffset + 3];

				const size rgbOffset = (x + y * width) * 3;
				rgbCopy[rgbOffset + 0] = r;
				rgbCopy[rgbOffset + 1] = g;
				rgbCopy[rgbOffset + 2] = b;

				const size alphaOffset = (x + y * width) * 1;
				alphaCopy[alphaOffset + 0] = a;
			}
		}

		// wxImage takes ownership of rgbCopy and alphaCopy
		wxImage img{ width, height, rgbCopy, alphaCopy };

		ilDeleteImage(imgId);
		return img;
	}

	bool App::OpenDDSFile(PathView filePath)
	{
		std::shared_ptr file = std::dynamic_pointer_cast<RawFile>(mRootDevice->Open(filePath));
		if (!file)
		{
			return false;
		}

		Stream& s = file->Stream();
		constexpr u32 DDSHeaderMagic{ 0x20534444 }; // 'DDS '
		const u32 headerMagic = s.Read<u32>();

		if (headerMagic == DDSHeaderMagic)
		{
			const size ddsSize = gsl::narrow<size>(s.Size());

			std::unique_ptr buffer = std::make_unique<byte[]>(ddsSize);
			s.Seek(0, StreamSeekOrigin::Begin);
			s.Read(buffer.get(), ddsSize);

			const wxImage img = CreateImageFromDDS({ buffer.get(), gsl::narrow<ptrdiff>(ddsSize) });
			ImageWindow* imgWin =
				new ImageWindow(mMainWindow,
								wxID_ANY,
								{ filePath.String().data(), filePath.String().size() },
								img);
			imgWin->Show();
			return true;
		}

		return false;
	}

	bool App::OpenShaderProgramFile(PathView)
	{
		// std::shared_ptr file = mRootDevice->Open(filePath);

		// if (TFileTraits<CShaderProgramFile>::IsValid(*file))
		//{
		//	wxString title =
		//		"Shader Program - " + wxString{ filePath.String().data(), filePath.String().size()
		//}; 	CShaderProgramWindow* shaderWin = 		new CShaderProgramWindow(mMainWindow,
		// wxID_ANY, 								 title,
		// std::make_unique<CShaderProgramFile>(*file)); shaderWin->Show(); 	return true;
		//}

		return false;
	}

	bool App::OpenAttributeFile(PathView)
	{
		// std::shared_ptr file = mRootDevice->Open(filePath);

		// if (TFileTraits<CAttributeFile>::IsValid(*file))
		//{
		//	wxString title =
		//		"Attribute - " + wxString{ filePath.String().data(), filePath.String().size() };
		//	CAttributeWindow* atbWin = new CAttributeWindow(mMainWindow,
		//													wxID_ANY,
		//													title,
		//													std::make_unique<CAttributeFile>(*file));
		//	atbWin->Show();
		//	return true;
		//}

		return false;
	}

	bool App::OpenUniqueTextureVRamFile(PathView)
	{
		// auto* fs = wxGetApp().FileSystem();
		// if (filePath.Name() == "uniquetexturevram")
		//{
		//	// TODO: move this loading somewhere else, possibly to noire-formats
		//	// TODO: show all textures in the same window
		//	SPath mainPath = filePath.Parent() / "uniquetexturemain";
		//	Expects(fs->FileExists(mainPath)); // TODO: 'uniquetexturemain' may not exist along
		//									   // with a 'uniquetexturevram' file
		//	std::unique_ptr<IFileStream> mainStream = fs->OpenFile(mainPath);
		//	std::unique_ptr<IFileStream> vramStream = fs->OpenFile(filePath);
		//	const std::size_t vramStreamSize = vramStream->Size();

		//	mainStream->Read<std::uint32_t>(); // these 4 bytes are used by the game at runtime to
		//									   // indicate if it already loaded the texture, the
		// file
		//									   // should always have 0 here

		//	// read entries
		//	const std::uint32_t textureCount = mainStream->Read<std::uint32_t>();
		//	struct TextureEntry
		//	{
		//		std::uint32_t Offset;
		//		std::uint32_t UnkZero;
		//		std::uint32_t NameHash;
		//	};
		//	std::vector<TextureEntry> entries{};
		//	entries.reserve(textureCount);
		//	for (std::size_t i = 0; i < textureCount; i++)
		//	{
		//		entries.emplace_back(mainStream->Read<TextureEntry>());
		//	}

		//	// open a window for each texture
		//	for (std::size_t i = 0; i < entries.size(); i++)
		//	{
		//		const TextureEntry& e = entries[i];
		//		// this expects the entries to be sorted by offset, not sure if that is always the
		// case 		const std::size_t size = (i < (entries.size() - 1)) ?
		// (entries[i
		// + 1].Offset - e.Offset) : 									 (vramStreamSize -
		// e.Offset); std::unique_ptr<std::byte[]> buffer = std::make_unique<std::byte[]>(size);
		// vramStream->Seek(e.Offset); 		vramStream->Read(buffer.get(), size);

		//		const wxImage img =
		//			CreateImageFromDDS({ buffer.get(), gsl::narrow<std::ptrdiff_t>(size) });

		//		std::string title = noire::CHashDatabase::Instance().TryGetString(e.NameHash);
		//		title += " | " + std::string{ filePath.String() };
		//		CImageWindow* imgWin = new CImageWindow(mMainWindow, wxID_ANY, title, img);
		//		imgWin->Show();
		//	}
		//	return true;
		//}

		return false;
	}

	bool App::OpenFile(PathView filePath)
	{
		return OpenUniqueTextureVRamFile(filePath) || OpenShaderProgramFile(filePath) ||
			   OpenAttributeFile(filePath) || OpenDDSFile(filePath);
	}
}
