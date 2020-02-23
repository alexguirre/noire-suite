#pragma once
#include "Common.h"
#include "Path.h"
#include <functional>
#include <memory>

namespace noire
{
	class File;
	class ReadOnlyStream;

	using DeviceVisitCallback = std::function<void(PathView)>;

	class Device
	{
	public:
		virtual ~Device() = default;

		virtual bool Exists(PathView path) const = 0;
		virtual std::shared_ptr<File> Open(PathView path) = 0;
		virtual std::shared_ptr<File> Create(PathView path, size fileTypeId) = 0;
		virtual bool Delete(PathView path) = 0;
		virtual void Visit(DeviceVisitCallback visitDirectory,
						   DeviceVisitCallback visitFile,
						   PathView path,
						   bool recursive = true) = 0;
		virtual std::shared_ptr<ReadOnlyStream> OpenStream(PathView path) = 0;
	};
}