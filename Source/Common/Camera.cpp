#include "Common/Camera.h"
#include "Common/Color.h"
#include "Math/Math.h"

Camera::Camera(ProjType projType, f32 nearClipPlane, f32 farClipPlane, f32 aspectRatio)
	: SceneObject("Camera")
	, m_ProjType(projType)
	, m_BackgroundColor(Color::BLUE)
	, m_NearClipPlane(nearClipPlane)
	, m_FarClipPlane(farClipPlane)
	, m_AspectRatio(aspectRatio)
	, m_ClearFlags(ClearFlag_None)
	, m_CullingMask(0)
	, m_UseOcclusionCulling(true)
	, m_OpaqueSortMode(OpaqueSortMode_FrontToBack)
	, m_FovY(0.25f * PI)
	, m_SizeY(10.0f)
	, m_DirtyFlags(DirtyFlag_ProjMatrix)
{
}

f32 Camera::GetAspectRatio() const
{
	return m_AspectRatio;
}

void Camera::SetAspectRatio(f32 aspectRatio)
{
	m_AspectRatio = aspectRatio;
	m_DirtyFlags |= DirtyFlag_ProjMatrix;
}

u8 Camera::GetClearFlags() const
{
	return m_ClearFlags;
}

void Camera::SetClearFlags(u8 clearFlags)
{
	m_ClearFlags = clearFlags;
}

u8 Camera::GetCullingMask() const
{
	assert(false && "Needs impl");
	return m_CullingMask;
}

void Camera::SetCullingMask(u8 cullingMask)
{
	assert(false && "Needs impl");
	m_CullingMask = cullingMask;
}

bool Camera::UseOcclusionCulling() const
{
	return m_UseOcclusionCulling;
}

void Camera::SetUseOcclusionCulling(bool useOcclusionCulling)
{
	m_UseOcclusionCulling = useOcclusionCulling;
}

Camera::OpaqueSortMode Camera::GetOpaqueSortMode() const
{
	return m_OpaqueSortMode;
}

void Camera::SetOpaqueSortMode(Camera::OpaqueSortMode opaqueSortMode)
{
	m_OpaqueSortMode = opaqueSortMode;
}

const Radian& Camera::GetFovY() const
{
	assert(m_ProjType == ProjType_Perspective);
	return m_FovY;
}

void Camera::SetFovY(const Radian& fovY)
{
	assert(m_ProjType == ProjType_Perspective);
	m_FovY = fovY;
	m_DirtyFlags |= DirtyFlag_ProjMatrix;
}

f32 Camera::GetSizeY() const
{
	assert(m_ProjType == ProjType_Ortho);
	return m_SizeY;
}

void Camera::SetSizeY(f32 sizeY)
{
	assert(m_ProjType == ProjType_Ortho);
	m_SizeY = sizeY;
	m_DirtyFlags |= DirtyFlag_ProjMatrix;
}

Camera::ProjType Camera::GetProjType() const
{
	return m_ProjType;
}

void Camera::SetProjType(Camera::ProjType projType)
{
	m_ProjType = projType;
	m_DirtyFlags |= DirtyFlag_ProjMatrix;
}

const Vector4f& Camera::GetBackgroundColor() const
{
	return m_BackgroundColor;
}

void Camera::SetBackgroundColor(const Vector4f& backgroundColor)
{
	m_BackgroundColor = backgroundColor;
}

f32 Camera::GetNearClipPlane() const
{
	return m_NearClipPlane;
}

void Camera::SetNearClipPlane(f32 nearClipPlane)
{
	m_NearClipPlane = nearClipPlane;
	m_DirtyFlags |= DirtyFlag_ProjMatrix;
}

f32 Camera::GetFarClipPlane() const
{
	return m_FarClipPlane;
}

void Camera::SetFarClipPlane(f32 farClipPlane)
{
	m_FarClipPlane = farClipPlane;
	m_DirtyFlags |= DirtyFlag_ProjMatrix;
}

const Matrix4f& Camera::GetViewMatrix() const
{
	return GetTransform().GetWorldToLocalMatrix();
}

const Matrix4f& Camera::GetProjMatrix() const
{
	if (m_DirtyFlags & DirtyFlag_ProjMatrix)
	{
		if (m_ProjType == ProjType_Ortho)
		{
			m_ProjMatrix = CreateOrthoProjMatrix(m_AspectRatio * m_SizeY, m_SizeY, m_NearClipPlane, m_FarClipPlane);
		}
		else if (m_ProjType == ProjType_Perspective)
		{
			m_ProjMatrix = CreatePerspectiveFovProjMatrix(m_FovY, m_AspectRatio, m_NearClipPlane, m_FarClipPlane);
		}
		else
		{
			assert(false);
		}
		m_DirtyFlags &= ~DirtyFlag_ProjMatrix;
	}
	return m_ProjMatrix;
}
