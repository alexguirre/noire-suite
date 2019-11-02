#pragma once
#include <stack>
#include <string>

class CDirectoryHistory
{
public:
	CDirectoryHistory();

	const std::string& Current() const;
	bool HasCurrent() const;
	void Push(std::string_view dir);
	void GoBack();
	void GoForward();
	void GoUp();
	bool CanGoBack() const;
	bool CanGoForward() const;
	bool CanGoUp() const;

private:
	std::stack<std::string> mBackStack;
	std::stack<std::string> mForwardStack;
};