#pragma once
#include "File.h"
#include "fs/FileStream.h"
#include <cstddef>
#include <string>
#include <vector>

namespace noire
{
	class CShaderProgramFile
	{
	public:
		using Bytecode = std::vector<std::byte>;
		struct Shader
		{
			std::string Name;
			std::uint32_t Unk08;
			Bytecode Bytecode;

			std::string Disassemble() const;
		};

		CShaderProgramFile(fs::IFileStream& stream);

		const Shader& VertexShader() const { return mVertexShader; }
		const Shader& PixelShader() const { return mPixelShader; }

	private:
		void Load(fs::IFileStream& stream);
		void ReadShaderChunk(fs::IFileStream& stream, Shader& shader);

		Shader mVertexShader;
		Shader mPixelShader;

	public:
		static bool IsValid(fs::IFileStream& stream);
	};

	template<>
	struct TFileTraits<CShaderProgramFile>
	{
		static constexpr bool IsCollection{ false };
		static bool IsValid(fs::IFileStream& stream) { return CShaderProgramFile::IsValid(stream); }
	};
}