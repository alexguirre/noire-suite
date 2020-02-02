#pragma once
#include "Math.h"
#include <cstddef>

class CCamera final
{
public:
	CCamera();
	CCamera(const CCamera&) = default;
	CCamera(CCamera&&) = default;

	CCamera& operator=(const CCamera&) = default;
	CCamera& operator=(CCamera&&) = default;

	void Position(const Vector3& p) noexcept
	{
		mPosition = p;
		mUp = mPosition + Vector3::Up;
		mTarget = mPosition + Vector3::Forward;
		UpdateMatrices();
	}
	const Vector3& Position() const noexcept { return mPosition; }

	const Vector3 Up() const noexcept { return mUp - mPosition; }

	const Vector3& Target() const noexcept { return mTarget; }
	const Vector3 LookAtTarget() const noexcept { return mTarget - mPosition; }

	const Matrix& Projection() const noexcept { return mProjection; }
	const Matrix& View() const noexcept { return mView; }
	const Matrix& ViewProjection() const noexcept { return mViewProjection; }

	void Resize(std::size_t width, std::size_t height);

private:
	void UpdateMatrices();

	Vector3 mPosition;
	Vector3 mUp; // up vector end position
	Vector3 mTarget;
	Matrix mProjection;
	Matrix mView;
	Matrix mViewProjection;
	std::size_t mClientWidth;
	std::size_t mClientHeight;
};