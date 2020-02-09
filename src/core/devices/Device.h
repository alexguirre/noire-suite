#pragma once
#include "Common.h"
#include "Path.h"
#include <memory>

namespace noire
{
	class Stream;

	class Device
	{
	public:
		virtual ~Device() = default;

		virtual bool Exists(PathView path) const = 0;
		virtual std::shared_ptr<Stream> Open(PathView path) = 0;
		virtual std::shared_ptr<Stream> Create(PathView path) = 0;
		virtual bool Delete(PathView path) = 0;
	};
}