#pragma once
#include "Common.h"
#include <string>
#include <string_view>

namespace noire
{
	struct PathView
	{
		static constexpr char DirectorySeparator{ '/' };
		static const PathView Root;

		constexpr PathView() noexcept : mPathStr{} {}
		constexpr PathView(const char* pathStr) noexcept : mPathStr{ pathStr } {}
		constexpr PathView(const std::string_view pathStr) noexcept : mPathStr{ pathStr } {}
		PathView(const std::string& pathStr) noexcept : mPathStr{ pathStr } {}

		constexpr PathView(const PathView&) noexcept = default;
		constexpr PathView& operator=(const PathView&) noexcept = default;

		constexpr PathView(PathView&&) noexcept = default;
		constexpr PathView& operator=(PathView&&) noexcept = default;

		[[nodiscard]] constexpr bool IsEmpty() const noexcept { return mPathStr.size() == 0; }

		[[nodiscard]] constexpr bool IsDirectory() const noexcept
		{
			Expects(!IsEmpty());

			return mPathStr.back() == DirectorySeparator;
		}

		[[nodiscard]] constexpr bool IsFile() const noexcept
		{
			Expects(!IsEmpty());

			return !IsDirectory();
		}

		[[nodiscard]] constexpr bool IsAbsolute() const noexcept
		{
			Expects(!IsEmpty());

			return mPathStr.front() == DirectorySeparator;
		}

		[[nodiscard]] constexpr bool IsRelative() const noexcept
		{
			Expects(!IsEmpty());

			return !IsAbsolute();
		}

		[[nodiscard]] constexpr bool IsRoot() const noexcept
		{
			Expects(!IsEmpty());

			return mPathStr.size() == 1 && IsDirectory();
		}

		[[nodiscard]] constexpr bool HasParent() const noexcept { return !Parent().IsEmpty(); }
		[[nodiscard]] constexpr PathView Parent() const noexcept
		{
			Expects(!IsEmpty());

			if (IsRoot())
			{
				return PathView{};
			}

			const size parentPos =
				mPathStr.rfind(DirectorySeparator, mPathStr.size() - (IsDirectory() ? 2 : 1));

			return (parentPos != std::string_view::npos) ? mPathStr.substr(0, parentPos + 1) :
														   PathView{};
		}

		[[nodiscard]] constexpr std::string_view Name() const noexcept
		{
			Expects(!IsEmpty());

			if (IsRoot())
			{
				return std::string_view{};
			}

			const bool isDir = IsDirectory();
			size parentPos = mPathStr.rfind(DirectorySeparator, mPathStr.size() - (isDir ? 2 : 1));
			parentPos = (parentPos == std::string_view::npos) ? 0 : (parentPos + 1);

			const size nameSize = mPathStr.size() - parentPos - (isDir ? 1 : 0);
			return mPathStr.substr(parentPos, nameSize);
		}

		[[nodiscard]] constexpr PathView RelativeTo(const PathView rootPath) const noexcept
		{
			if (rootPath.IsEmpty())
			{
				return IsRelative() ? mPathStr : PathView{};
			}

			Expects(rootPath.IsDirectory());

			const size rootPos = mPathStr.find(rootPath.mPathStr);
			return rootPos == 0 ? mPathStr.substr(rootPath.String().size()) : PathView{};
		}

		[[nodiscard]] constexpr std::string_view String() const noexcept { return mPathStr; }
		[[nodiscard]] constexpr bool Equal(const PathView other) const noexcept
		{
			return mPathStr == other.mPathStr;
		}

	private:
		std::string_view mPathStr;
	};

	struct Path
	{
		static constexpr char DirectorySeparator{ PathView::DirectorySeparator };
		static const Path Root;

		Path() noexcept : mPathStr{} {}
		Path(const char* pathStr) : mPathStr{ pathStr } {}
		explicit Path(const PathView path) : mPathStr{ path.String() } {}
		explicit Path(const std::string_view pathStr) : mPathStr{ pathStr } {}
		explicit Path(const std::string& pathStr) : mPathStr{ pathStr } {}

		Path(const Path& other) : mPathStr{ other.mPathStr } {}
		Path& operator=(const Path& other)
		{
			if (this != &other)
			{
				mPathStr = other.mPathStr;
			}

			return *this;
		}

		Path& operator=(const PathView path)
		{
			mPathStr = path.String();
			return *this;
		}

		Path& operator=(const char* path)
		{
			mPathStr = path;
			return *this;
		}

		Path(Path&&) noexcept = default;
		Path& operator=(Path&&) noexcept = default;

		[[nodiscard]] auto IsEmpty() const noexcept { return View().IsEmpty(); }
		[[nodiscard]] auto IsDirectory() const noexcept { return View().IsDirectory(); }
		[[nodiscard]] auto IsFile() const noexcept { return View().IsFile(); }
		[[nodiscard]] auto IsAbsolute() const noexcept { return View().IsAbsolute(); }
		[[nodiscard]] auto IsRelative() const noexcept { return View().IsRelative(); }
		[[nodiscard]] auto IsRoot() const noexcept { return View().IsRoot(); }
		[[nodiscard]] auto HasParent() const noexcept { return View().HasParent(); }
		[[nodiscard]] auto Parent() const noexcept { return View().Parent(); }
		[[nodiscard]] auto Name() const noexcept { return View().Name(); }
		[[nodiscard]] auto RelativeTo(const PathView rootPath) const noexcept
		{
			return View().RelativeTo(rootPath);
		}

		[[nodiscard]] const std::string& String() const noexcept { return mPathStr; }
		[[nodiscard]] bool Equal(const Path& other) const noexcept
		{
			return mPathStr == other.mPathStr;
		}

		Path& Append(const PathView path);

		Path& operator/=(const Path& other) { return Append(other); }
		Path& operator/=(const PathView other) { return Append(other); }

		Path& Concat(const std::string_view str);
		Path& Concat(const char c);

		Path& operator+=(const std::string_view str) { return Concat(str); }
		Path& operator+=(const char c) { return Concat(c); }

		Path& Normalize();

		operator PathView() const noexcept { return View(); }

	private:
		PathView View() const noexcept { return PathView{ mPathStr }; }

		std::string mPathStr;
	};

	[[nodiscard]] inline Path operator/(const Path& left, const Path& right)
	{
		return Path{ left } /= right;
	}

	[[nodiscard]] inline Path operator/(const Path& left, const PathView right)
	{
		return Path{ left } /= right;
	}

	[[nodiscard]] inline Path operator/(const PathView left, const Path& right)
	{
		return Path{ left } /= right;
	}

	[[nodiscard]] inline Path operator+(const Path& left, const std::string_view right)
	{
		return Path{ left } += right;
	}

	[[nodiscard]] inline Path operator+(const Path& left, const char right)
	{
		return Path{ left } += right;
	}

	[[nodiscard]] inline bool operator==(const Path& left, const Path& right) noexcept
	{
		return left.Equal(right);
	}

	[[nodiscard]] inline bool operator!=(const Path& left, const Path& right) noexcept
	{
		return !left.Equal(right);
	}

	[[nodiscard]] constexpr bool operator==(const PathView left, const PathView right) noexcept
	{
		return left.Equal(right);
	}

	[[nodiscard]] constexpr bool operator!=(const PathView left, const PathView right) noexcept
	{
		return !left.Equal(right);
	}

	[[nodiscard]] inline bool operator==(const Path& left, const PathView right) noexcept
	{
		return right.Equal(left);
	}

	[[nodiscard]] inline bool operator!=(const Path& left, const PathView right) noexcept
	{
		return !right.Equal(left);
	}

	[[nodiscard]] inline bool operator==(const PathView left, const Path& right) noexcept
	{
		return left.Equal(right);
	}

	[[nodiscard]] inline bool operator!=(const PathView left, const Path& right) noexcept
	{
		return !left.Equal(right);
	}
}

namespace std
{
	template<>
	struct hash<noire::PathView>
	{
		std::size_t operator()(const noire::PathView& s) const noexcept
		{
			return hash<std::string_view>{}(s.String());
		}
	};

	template<>
	struct hash<noire::Path>
	{
		std::size_t operator()(const noire::Path& s) const noexcept
		{
			return hash<std::string>{}(s.String());
		}
	};
}
