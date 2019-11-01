#include "DirectoryHistory.h"
#include <gsl/gsl>

using namespace noire;

CDirectoryHistory::CDirectoryHistory() {}

const WADChildDirectory& CDirectoryHistory::Current() const
{
	Expects(HasCurrent());

	return *mBackStack.top();
}

bool CDirectoryHistory::HasCurrent() const
{
	return mBackStack.size() > 0;
}

void CDirectoryHistory::Push(const noire::WADChildDirectory& dir)
{
	mBackStack.push(&dir);
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

	Push(*Current().Parent());
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
	return HasCurrent() && Current().Parent() != nullptr;
}
