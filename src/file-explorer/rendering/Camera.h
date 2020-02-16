#pragma once
#include "Math.h"
#include <core/Common.h>

namespace noire::explorer
{
	class Camera final
	{
	public:
		Camera() noexcept;
		Camera(const Camera&) noexcept = default;
		Camera(Camera&&) noexcept = default;

		Camera& operator=(const Camera&) noexcept = default;
		Camera& operator=(Camera&&) noexcept = default;

		void Transform(const Matrix& t) noexcept;
		const Matrix& Transform() const noexcept { return mTransform; }

		const Matrix& Projection() const noexcept { return mProjection; }
		const Matrix& View() const noexcept { return mView; }
		const Matrix& ViewProjection() const noexcept { return mViewProjection; }

		void Resize(size width, size height) noexcept;

	private:
		void UpdateMatrices() noexcept;

		Matrix mTransform;
		Matrix mProjection;
		Matrix mView;
		Matrix mViewProjection;
		size mClientWidth;
		size mClientHeight;
	};
}
