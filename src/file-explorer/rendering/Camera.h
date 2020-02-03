#pragma once
#include "Math.h"
#include <cstddef>

class CCamera final
{
public:
	CCamera() noexcept;
	CCamera(const CCamera&) noexcept = default;
	CCamera(CCamera&&) noexcept = default;

	CCamera& operator=(const CCamera&) noexcept = default;
	CCamera& operator=(CCamera&&) noexcept = default;

	void Transform(const Matrix& t) noexcept;
	const Matrix& Transform() const noexcept { return mTransform; }

	const Matrix& Projection() const noexcept { return mProjection; }
	const Matrix& View() const noexcept { return mView; }
	const Matrix& ViewProjection() const noexcept { return mViewProjection; }

	void Resize(std::size_t width, std::size_t height) noexcept;

private:
	void UpdateMatrices() noexcept;

	Matrix mTransform;
	Matrix mProjection;
	Matrix mView;
	Matrix mViewProjection;
	std::size_t mClientWidth;
	std::size_t mClientHeight;
};