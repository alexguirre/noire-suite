#pragma once
#include <formats/WADFile.h>
#include <stack>

class CDirectoryHistory
{
public:
	CDirectoryHistory();

	const noire::WADChildDirectory& Current() const;
	bool HasCurrent() const;
	void Push(const noire::WADChildDirectory& dir);
	void GoBack();
	void GoForward();
	void GoUp();
	bool CanGoBack() const;
	bool CanGoForward() const;
	bool CanGoUp() const;

private:
	std::stack<const noire::WADChildDirectory*> mBackStack;
	std::stack<const noire::WADChildDirectory*> mForwardStack;
};