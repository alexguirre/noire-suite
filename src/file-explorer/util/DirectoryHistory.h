#pragma once
#include <core/Path.h>
#include <stack>

namespace noire::explorer
{
	class DirectoryHistory
	{
	public:
		DirectoryHistory();

		PathView Current() const;
		bool HasCurrent() const;
		void Push(PathView dir);
		void GoBack();
		void GoForward();
		void GoUp();
		bool CanGoBack() const;
		bool CanGoForward() const;
		bool CanGoUp() const;

	private:
		std::stack<Path> mBackStack;
		std::stack<Path> mForwardStack;
	};
}