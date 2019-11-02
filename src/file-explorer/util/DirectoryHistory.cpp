#include "DirectoryHistory.h"
#include <formats/fs/FileSystem.h>
#include <gsl/gsl>

CDirectoryHistory::CDirectoryHistory() {}

const std::string& CDirectoryHistory::Current() const
{
	Expects(HasCurrent());

	return mBackStack.top();
}

bool CDirectoryHistory::HasCurrent() const
{
	return mBackStack.size() > 0;
}

void CDirectoryHistory::Push(std::string_view dir)
{
	mBackStack.emplace(dir);
	mForwardStack = {}; // clear the forward stack
}

void CDirectoryHistory::GoBack()
{
	Expects(CanGoBack());

	mForwardStack.push(mBackStack.top());
	mBackStack.pop();
}

void CDirectoryHistory::GoForward()
{
	Expects(CanGoForward());

	mBackStack.push(mForwardStack.top());
	mForwardStack.pop();
}

void CDirectoryHistory::GoUp()
{
	Expects(CanGoUp());

	const std::string& path = Current();
	std::size_t parentPos = path.rfind(noire::fs::CFileSystem::DirectorySeparator, path.size() - 2);
	if (parentPos != std::string_view::npos)
	{
		Push(path.substr(0, parentPos + 1));
	}
}

bool CDirectoryHistory::CanGoBack() const
{
	return mBackStack.size() > 1;
}

bool CDirectoryHistory::CanGoForward() const
{
	return mForwardStack.size() > 0;
}

bool CDirectoryHistory::CanGoUp() const
{
	if (HasCurrent())
	{
		auto& curr = Current();
		return std::count(curr.begin(), curr.end(), noire::fs::CFileSystem::DirectorySeparator) > 1;
	}
	else
	{
		return false;
	}
}
