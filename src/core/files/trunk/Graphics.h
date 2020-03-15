#pragma once
#include "Common.h"
#include "Section.h"

namespace noire
{
	class Trunk;
}

namespace noire::trunk
{
	struct Graphics
	{
		Trunk& Owner;
		Section Main;
		Section VRAM;

		Graphics(Trunk& owner, Section main, Section vram);
	};
}
