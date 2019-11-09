#include "DirectoryHistory.h"
#include <formats/fs/FileSystem.h>
#include <gsl/gsl>

CDirectoryHistory::CDirectoryHistory() {}

noire::fs::SPathView CDirectoryHistory::Current() const
{
	Expects(HasCurrent());

	return mBackStack.top();
}

bool CDirectoryHistory::HasCurrent() const
{
	return mBackStack.size() > 0;
}

void CDirectoryHistory::Push(noire::fs::SPathView dir)
{
	Expects(dir.IsDirectory());

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

	Push(Current().Parent());
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
		return Current().HasParent();
	}
	else
	{
		return false;
	}
}
