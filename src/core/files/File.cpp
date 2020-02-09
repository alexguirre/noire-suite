#include "File.h"
#include "streams/FileStream.h"

namespace noire
{
	File::File(std::shared_ptr<Stream> input) : mInput{ input } {}

	void File::Load()
	{
		// empty
	}

	void File::Save(Stream& output) { mInput->CopyTo(output); }

}
