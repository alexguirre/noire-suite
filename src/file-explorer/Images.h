#pragma once

class wxImageList;

namespace noire::explorer
{
	class Images
	{
	public:
		enum Icon
		{
			IconBlankFile,
			IconBlueFolder,
			IconFolder,
			IconNoire,

			IconCount,
		};

		static wxImageList* Icons();

	private:
		Images() = delete;
	};
}
