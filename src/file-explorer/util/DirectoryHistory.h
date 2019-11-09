#pragma once
#include <formats/fs/Path.h>
#include <stack>

class CDirectoryHistory
{
public:
	CDirectoryHistory();

	noire::fs::SPathView Current() const;
	bool HasCurrent() const;
	void Push(noire::fs::SPathView dir);
	void GoBack();
	void GoForward();
	void GoUp();
	bool CanGoBack() const;
	bool CanGoForward() const;
	bool CanGoUp() const;

private:
	std::stack<noire::fs::SPath> mBackStack;
	std::stack<noire::fs::SPath> mForwardStack;
};