#include "App.h"
#include "windows/AttributeWindow.h"
#include "windows/ImageWindow.h"
#include "windows/MainWindow.h"
#include "windows/ShaderProgramWindow.h"
#include "windows/TrunkWindow.h"
#include <IL/il.h>
#include <core/Common.h>
#include <core/devices/LocalDevice.h>
#include <core/files/File.h>
#include <core/files/Trunk.h>
#include <core/streams/FileStream.h>
#include <processthreadsapi.h>
#include <thread>

wxIMPLEMENT_APP(noire::explorer::App);

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

		mRootDevice = std::make_unique<MultiDevice>();
		mRootDevice->Mount(PathView::Root, std::make_shared<LocalDevice>(path));

		mMainWindow->OnRootPathChanged();
	}

	/*static*/ wxImage CreateImageFromDDS(gsl::span<const byte> ddsData)
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
		std::shared_ptr file = mRootDevice->Open(filePath);
		if (!file)
		{
			return false;
		}

		Stream& s = file->Raw();
		s.Seek(0, StreamSeekOrigin::Begin);

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

	bool App::OpenTrunkFile(PathView filePath)
	{
		std::shared_ptr file = mRootDevice->Open(filePath);

		if (std::shared_ptr trunkFile = std::dynamic_pointer_cast<Trunk>(file); trunkFile)
		{
			wxString title =
				"Trunk - " + wxString{ filePath.String().data(), filePath.String().size() };
			TrunkWindow* trunkWin = new TrunkWindow(mMainWindow, wxID_ANY, title, trunkFile);
			trunkWin->Show();
			return true;
		}

		return false;
	}

	bool App::OpenFile(PathView filePath)
	{
		return OpenTrunkFile(filePath) || OpenShaderProgramFile(filePath) ||
			   OpenAttributeFile(filePath) || OpenDDSFile(filePath);
	}
}
