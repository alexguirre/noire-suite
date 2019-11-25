#include "ShaderProgramFile.h"
#include <d3d11.h>
#include <d3dcompiler.h>
#include <gsl/gsl>

namespace noire
{
	std::string CShaderProgramFile::Shader::Disassemble() const
	{
		if (ID3DBlob* disassembly = nullptr;
			SUCCEEDED(D3DDisassemble(Bytecode.data(),
									 Bytecode.size(),
									 D3D_DISASM_ENABLE_DEFAULT_VALUE_PRINTS,
									 "",
									 &disassembly)))
		{
			std::string str = { reinterpret_cast<char*>(disassembly->GetBufferPointer()),
								disassembly->GetBufferSize() };
			disassembly->Release();
			return str;
		}
		else
		{
			return {};
		}
	}

	CShaderProgramFile::CShaderProgramFile(fs::IFileStream& stream)
		: mVertexShader{}, mPixelShader{}
	{
		Load(stream);
	}

	void CShaderProgramFile::Load(fs::IFileStream& stream)
	{
		stream.Seek(0);
		ReadShaderChunk(stream, mVertexShader);
		ReadShaderChunk(stream, mPixelShader);
	}

	void CShaderProgramFile::ReadShaderChunk(fs::IFileStream& stream, Shader& shader)
	{
		const std::uint32_t chunkSize = stream.Read<std::uint32_t>();
		const std::uint32_t bytecodeSize = stream.Read<std::uint32_t>();
		shader.Unk08 = stream.Read<std::uint32_t>();

		shader.Bytecode.resize(bytecodeSize);
		if (bytecodeSize != 0)
		{
			stream.Read(shader.Bytecode.data(), bytecodeSize);
		}

		const std::size_t nameLen = chunkSize - bytecodeSize - 12;
		shader.Name.resize(nameLen);
		stream.Read(shader.Name.data(), nameLen);
	}

	bool CShaderProgramFile::IsValid(fs::IFileStream& stream)
	{
		const auto resetStreamPos = gsl::finally([&stream]() { stream.Seek(0); });

		const fs::FileStreamSize streamSize = stream.Size();
		if (streamSize <= 16) // min required bytes
		{
			return false;
		}

		const std::uint32_t vertexShaderSize = stream.Read<std::uint32_t>();
		if (vertexShaderSize > (streamSize - 4))
		{
			return false;
		}
		const std::uint32_t vertexShaderBytecodeSize = stream.Read<std::uint32_t>();

		stream.Seek(vertexShaderSize);
		const std::uint32_t pixelShaderSize = stream.Read<std::uint32_t>();

		const fs::FileStreamSize totalSize =
			static_cast<fs::FileStreamSize>(vertexShaderSize) + pixelShaderSize;
		if (totalSize != streamSize)
		{
			return false;
		}

		const std::uint32_t pixelShaderBytecodeSize = stream.Read<std::uint32_t>();

		// check headers of shaders bytecode
		const std::uint32_t HeaderMagic{ 0x43425844 }; // DXBC

		if (vertexShaderBytecodeSize != 0)
		{
			stream.Seek(12);
			if (stream.Read<uint32_t>() != HeaderMagic)
			{
				return false;
			}
		}

		if (pixelShaderBytecodeSize != 0)
		{
			stream.Seek(static_cast<fs::FileStreamSize>(vertexShaderSize) + 12);
			if (stream.Read<uint32_t>() != HeaderMagic)
			{
				return false;
			}
		}

		return true;
	}
}