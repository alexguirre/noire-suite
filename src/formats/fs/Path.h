#pragma once

#include <gsl/gsl_assert>
#include <string>
#include <string_view>

namespace noire::fs
{
	struct SPathView
	{
		static constexpr char DirectorySeparator{ '/' };

		constexpr SPathView() noexcept : mPathStr{} {}
		constexpr SPathView(const char* pathStr) noexcept : mPathStr{ pathStr } {}
		constexpr SPathView(const std::string_view pathStr) noexcept : mPathStr{ pathStr } {}
		SPathView(const std::string& pathStr) noexcept : mPathStr{ pathStr } {}

		constexpr SPathView(const SPathView&) noexcept = default;
		constexpr SPathView& operator=(const SPathView&) noexcept = default;

		constexpr SPathView(SPathView&&) noexcept = default;
		constexpr SPathView& operator=(SPathView&&) noexcept = default;

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
		[[nodiscard]] constexpr SPathView Parent() const noexcept
		{
			Expects(!IsEmpty());

			if (IsRoot())
			{
				return SPathView{};
			}

			const std::size_t parentPos =
				mPathStr.rfind(DirectorySeparator, mPathStr.size() - (IsDirectory() ? 2 : 1));

			return (parentPos != std::string_view::npos) ? mPathStr.substr(0, parentPos + 1) :
														   SPathView{};
		}

		[[nodiscard]] constexpr std::string_view Name() const noexcept
		{
			Expects(!IsEmpty());

			if (IsRoot())
			{
				return std::string_view{};
			}

			const bool isDir = IsDirectory();
			std::size_t parentPos =
				mPathStr.rfind(DirectorySeparator, mPathStr.size() - (isDir ? 2 : 1));
			parentPos = (parentPos == std::string_view::npos) ? 0 : (parentPos + 1);

			const std::size_t nameSize = mPathStr.size() - parentPos - (isDir ? 1 : 0);
			return mPathStr.substr(parentPos, nameSize);
		}

		[[nodiscard]] constexpr SPathView RelativeTo(const SPathView rootPath) const noexcept
		{
			if (rootPath.IsEmpty())
			{
				return IsRelative() ? mPathStr : SPathView{};
			}

			Expects(rootPath.IsDirectory());

			const std::size_t rootPos = mPathStr.find(rootPath.mPathStr);
			return rootPos == 0 ? mPathStr.substr(rootPath.String().size()) : SPathView{};
		}

		[[nodiscard]] constexpr std::string_view String() const noexcept { return mPathStr; }
		[[nodiscard]] constexpr bool Equal(const SPathView other) const noexcept
		{
			return mPathStr == other.mPathStr;
		}

	private:
		std::string_view mPathStr;
	};

	struct SPath
	{
		static constexpr char DirectorySeparator{ SPathView::DirectorySeparator };

		SPath() noexcept : mPathStr{} {}
		SPath(const char* pathStr) : mPathStr{ pathStr } {}
		explicit SPath(const SPathView path) : mPathStr{ path.String() } {}
		explicit SPath(const std::string_view pathStr) : mPathStr{ pathStr } {}
		explicit SPath(const std::string& pathStr) : mPathStr{ pathStr } {}

		SPath(const SPath& other) : mPathStr{ other.mPathStr } {}
		SPath& operator=(const SPath& other)
		{
			if (this != &other)
			{
				mPathStr = other.mPathStr;
			}

			return *this;
		}

		SPath& operator=(const SPathView path)
		{
			mPathStr = path.String();
			return *this;
		}

		SPath& operator=(const char* path)
		{
			mPathStr = path;
			return *this;
		}

		SPath(SPath&&) noexcept = default;
		SPath& operator=(SPath&&) noexcept = default;

		[[nodiscard]] auto IsEmpty() const noexcept { return View().IsEmpty(); }
		[[nodiscard]] auto IsDirectory() const noexcept { return View().IsDirectory(); }
		[[nodiscard]] auto IsFile() const noexcept { return View().IsFile(); }
		[[nodiscard]] auto IsAbsolute() const noexcept { return View().IsAbsolute(); }
		[[nodiscard]] auto IsRelative() const noexcept { return View().IsRelative(); }
		[[nodiscard]] auto IsRoot() const noexcept { return View().IsRoot(); }
		[[nodiscard]] auto HasParent() const noexcept { return View().HasParent(); }
		[[nodiscard]] auto Parent() const noexcept { return View().Parent(); }
		[[nodiscard]] auto Name() const noexcept { return View().Name(); }
		[[nodiscard]] auto RelativeTo(const SPathView rootPath) const noexcept
		{
			return View().RelativeTo(rootPath);
		}

		[[nodiscard]] const std::string& String() const noexcept { return mPathStr; }
		[[nodiscard]] bool Equal(const SPath& other) const noexcept
		{
			return mPathStr == other.mPathStr;
		}

		SPath& Append(const SPathView path);

		SPath& operator/=(const SPath& other) { return Append(other); }
		SPath& operator/=(const SPathView other) { return Append(other); }

		SPath& Concat(const std::string_view str);
		SPath& Concat(const char c);

		SPath& operator+=(const std::string_view str) { return Concat(str); }
		SPath& operator+=(const char c) { return Concat(c); }

		SPath& Normalize();

		operator SPathView() const noexcept { return View(); }

	private:
		SPathView View() const noexcept { return SPathView{ mPathStr }; }

		std::string mPathStr;
	};

	[[nodiscard]] inline SPath operator/(const SPath& left, const SPath& right)
	{
		return SPath{ left } /= right;
	}

	[[nodiscard]] inline SPath operator/(const SPath& left, const SPathView right)
	{
		return SPath{ left } /= right;
	}

	[[nodiscard]] inline SPath operator/(const SPathView left, const SPath& right)
	{
		return SPath{ left } /= right;
	}

	[[nodiscard]] inline SPath operator+(const SPath& left, const std::string_view right)
	{
		return SPath{ left } += right;
	}

	[[nodiscard]] inline SPath operator+(const SPath& left, const char right)
	{
		return SPath{ left } += right;
	}

	[[nodiscard]] inline bool operator==(const SPath& left, const SPath& right) noexcept
	{
		return left.Equal(right);
	}

	[[nodiscard]] inline bool operator!=(const SPath& left, const SPath& right) noexcept
	{
		return !left.Equal(right);
	}

	[[nodiscard]] constexpr bool operator==(const SPathView left, const SPathView right) noexcept
	{
		return left.Equal(right);
	}

	[[nodiscard]] constexpr bool operator!=(const SPathView left, const SPathView right) noexcept
	{
		return !left.Equal(right);
	}

	[[nodiscard]] inline bool operator==(const SPath& left, const SPathView right) noexcept
	{
		return right.Equal(left);
	}

	[[nodiscard]] inline bool operator!=(const SPath& left, const SPathView right) noexcept
	{
		return !right.Equal(left);
	}

	[[nodiscard]] inline bool operator==(const SPathView left, const SPath& right) noexcept
	{
		return left.Equal(right);
	}

	[[nodiscard]] inline bool operator!=(const SPathView left, const SPath& right) noexcept
	{
		return !left.Equal(right);
	}
}
