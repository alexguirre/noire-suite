#pragma once

class wxImageList;

class CImages
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
	CImages() = delete;
};