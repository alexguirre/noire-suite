#include "Camera.h"

namespace noire::explorer
{
	Camera::Camera() noexcept
		: mTransform{},
		  mProjection{},
		  mView{},
		  mViewProjection{},
		  mClientWidth{ 1920 },
		  mClientHeight{ 1080 }
	{
		Transform(Matrix::Identity);
	}

	void Camera::Transform(const Matrix& t) noexcept
	{
		mTransform = t;
		UpdateMatrices();
	}

	void Camera::Resize(size width, size height) noexcept
	{
		if (mClientWidth != width || mClientHeight != height)
		{
			mClientWidth = width;
			mClientHeight = height;

			UpdateMatrices();
		}
	}

	void Camera::UpdateMatrices() noexcept
	{
		// calculate projection
		{
			constexpr float FOV{ DirectX::XMConvertToRadians(45.0f) };
			constexpr float NearPlane{ 0.01f };
			constexpr float FarPlane{ 1000.0f };

			const float aspectRatio = static_cast<float>(mClientWidth) / mClientHeight;
			mProjection =
				Matrix::CreatePerspectiveFieldOfView(FOV, aspectRatio, NearPlane, FarPlane);
		}

		// calculate view
		{
			const Vector3 pos = mTransform.Translation();
			const Vector3 up = mTransform.Up();
			const Vector3 target = pos + mTransform.Forward();

			mView = Matrix::CreateLookAt(pos, target, up);
		}

		// calculate view-projection
		{
			mViewProjection = mView * mProjection;
		}
	}
}
