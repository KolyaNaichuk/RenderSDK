#include "Scene/Camera.h"
#include "Math/Transform.h"

namespace
{
	f32 NormalizeAngle(f32 angleInRadians)
	{
		while (angleInRadians < -PI)
			angleInRadians += TWO_PI;

		while (angleInRadians > PI)
			angleInRadians -= TWO_PI;

		return angleInRadians;
	}
}

Camera::Camera(const Vector3f& worldPosition, const BasisAxes& basisAxes,
	f32 fovYInRadians, f32 aspectRatio, f32 nearClipDist, f32 farClipDist,
	const Vector3f& moveSpeed, const Vector3f& rotationSpeed)
	: m_WorldPosition(worldPosition)
	, m_BasisAxes(basisAxes)
	, m_FovYInRadians(fovYInRadians)
	, m_AspectRatio(aspectRatio)
	, m_NearClipDist(nearClipDist)
	, m_FarClipDist(farClipDist)
	, m_MoveSpeed(moveSpeed)
	, m_RotationSpeed(rotationSpeed)
	, m_Dirty(true)
{
	m_RotationInRadians.m_X = ArcSin(-basisAxes.m_ZAxis.m_Y);
	f32 cosX = Cos(m_RotationInRadians.m_X);

	assert(!AreEqual(cosX, 0.0f, EPSILON) && "Encountered Gimbal lock - needs special handling");
	f32 rcpCosX = Rcp(cosX);

	m_RotationInRadians.m_Y = ArcTan(basisAxes.m_ZAxis.m_X * rcpCosX, basisAxes.m_ZAxis.m_Z * rcpCosX);
	m_RotationInRadians.m_Z = ArcTan(basisAxes.m_XAxis.m_Y * rcpCosX, basisAxes.m_YAxis.m_Y * rcpCosX);

	RecalcMatricesIfDirty();
}

const Vector3f& Camera::GetWorldPosition() const
{
	return m_WorldPosition;
}

void Camera::SetWorldPosition(const Vector3f& worldPosition)
{
	m_WorldPosition = worldPosition;
	m_Dirty = true;
}

const BasisAxes& Camera::GetBasisAxes() const
{
	return m_BasisAxes;
}

void Camera::SetBasisAxes(const BasisAxes& basisAxes)
{
	m_BasisAxes = basisAxes;
	m_Dirty = true;
}

f32 Camera::GetFieldOfViewY() const
{
	return m_FovYInRadians;
}

void Camera::SetFieldOfViewY(f32 fovYInRadians)
{
	m_FovYInRadians = fovYInRadians;
}

f32 Camera::GetAspectRatio() const
{
	return m_AspectRatio;
}

void Camera::SetAspectRatio(f32 aspectRatio)
{
	m_AspectRatio = aspectRatio;
	m_Dirty = true;
}

f32 Camera::GetNearClipDistance() const
{
	return m_NearClipDist;
}

void Camera::SetNearClipDistance(f32 nearClipDist)
{
	m_NearClipDist = nearClipDist;
	m_Dirty = true;
}

f32 Camera::GetFarClipDistance() const
{
	return m_FarClipDist;
}

void Camera::SetFarClipDistance(f32 farClipDist)
{
	m_FarClipDist = farClipDist;
	m_Dirty = true;
}

const Vector3f& Camera::GetMoveSpeed() const
{
	return m_MoveSpeed;
}

void Camera::SetMoveSpeed(const Vector3f& moveSpeed)
{
	m_MoveSpeed = moveSpeed;
}

const Vector3f& Camera::GetRotationSpeed() const
{
	return m_RotationSpeed;
}

void Camera::SetRotationSpeed(const Vector3f& rotationSpeed)
{
	m_RotationSpeed = rotationSpeed;
}

const Matrix4f& Camera::GetViewMatrix() const
{
	RecalcMatricesIfDirty();
	return m_ViewMatrix;
}

const Matrix4f& Camera::GetProjMatrix() const
{
	RecalcMatricesIfDirty();
	return m_ProjMatrix;
}

const Matrix4f& Camera::GetViewProjMatrix() const
{
	RecalcMatricesIfDirty();
	return m_ViewProjMatrix;
}

void Camera::Move(const Vector3f& moveDelta, f32 deltaTime)
{
	const Vector3f moveOffset = (deltaTime * m_MoveSpeed) * moveDelta;
	
	m_WorldPosition += moveOffset.m_X * m_BasisAxes.m_XAxis;
	m_WorldPosition += moveOffset.m_Y * m_BasisAxes.m_YAxis;
	m_WorldPosition += moveOffset.m_Z * m_BasisAxes.m_ZAxis;
	
	m_Dirty = true;
}

void Camera::Rotate(const Vector3f& rotationDeltaInRadians, f32 deltaTime)
{	
	m_RotationInRadians.m_Y += (deltaTime * m_RotationSpeed.m_Y) * rotationDeltaInRadians.m_Y;
	m_RotationInRadians.m_X += (deltaTime * m_RotationSpeed.m_X) * rotationDeltaInRadians.m_X;
	m_RotationInRadians.m_Z += (deltaTime * m_RotationSpeed.m_Z) * rotationDeltaInRadians.m_Z;

	m_RotationInRadians.m_Y = NormalizeAngle(m_RotationInRadians.m_Y);
	m_RotationInRadians.m_X = Clamp(-PI_DIV_2, PI_DIV_2, m_RotationInRadians.m_X);
	m_RotationInRadians.m_Z = NormalizeAngle(m_RotationInRadians.m_Z);
	
	m_BasisAxes = BasisAxes(CreateRotationZXYMatrix(m_RotationInRadians));
	m_Dirty = true;
}

void Camera::RecalcMatricesIfDirty() const
{
	if (m_Dirty)
	{
		m_ViewMatrix = CreateLookAtMatrix(m_WorldPosition, m_BasisAxes);
		m_ProjMatrix = CreatePerspectiveFovProjMatrix(m_FovYInRadians, m_AspectRatio, m_NearClipDist, m_FarClipDist);
		m_ViewProjMatrix = m_ViewMatrix * m_ProjMatrix;

		m_Dirty = false;
	}
}
