#include "Camera.h"

CCamera::CCamera()
	: mPosition{},
	  mUp{},
	  mTarget{},
	  mProjection{ Matrix::Identity },
	  mView{ Matrix::Identity },
	  mViewProjection{ Matrix::Identity }
{
	Position({ 0.0f, 0.0f, 1.0f });
}

void CCamera::Resize(std::size_t width, std::size_t height)
{
	if (mClientWidth != width || mClientHeight != height)
	{
		mClientWidth = width;
		mClientHeight = height;

		UpdateMatrices();
	}
}

void CCamera::UpdateMatrices()
{
	// calculate projection
	{
		constexpr float FOV{ DirectX::XMConvertToRadians(45.0f) };
		constexpr float NearPlane{ 0.01f };
		constexpr float FarPlane{ 1000.0f };

		const float aspectRatio = static_cast<float>(mClientWidth) / mClientHeight;
		mProjection = Matrix::CreatePerspectiveFieldOfView(FOV, aspectRatio, NearPlane, FarPlane);
	}

	// calculate view
	{
		mView = Matrix::CreateLookAt(mPosition, mTarget, Up());
	}

	// calculate view-projection
	{
		mViewProjection = mView * mProjection;
	}
}
