#include "DirectoryHistory.h"
#include <gsl/gsl>

namespace noire::explorer
{
	DirectoryHistory::DirectoryHistory() {}

	PathView DirectoryHistory::Current() const
	{
		Expects(HasCurrent());

		return mBackStack.top();
	}

	bool DirectoryHistory::HasCurrent() const { return mBackStack.size() > 0; }

	void DirectoryHistory::Push(PathView dir)
	{
		Expects(dir.IsDirectory());

		mBackStack.emplace(dir);
		mForwardStack = {}; // clear the forward stack
	}

	void DirectoryHistory::GoBack()
	{
		Expects(CanGoBack());

		mForwardStack.push(mBackStack.top());
		mBackStack.pop();
	}

	void DirectoryHistory::GoForward()
	{
		Expects(CanGoForward());

		mBackStack.push(mForwardStack.top());
		mForwardStack.pop();
	}

	void DirectoryHistory::GoUp()
	{
		Expects(CanGoUp());

		Push(Current().Parent());
	}

	bool DirectoryHistory::CanGoBack() const { return mBackStack.size() > 1; }

	bool DirectoryHistory::CanGoForward() const { return mForwardStack.size() > 0; }

	bool DirectoryHistory::CanGoUp() const
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
}
