#include "Scene/Camera.h"
#include "Math/Transform.h"
#include "Common/KeyboardInput.h"

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

Camera::Camera(const Vector3f& worldPosition, const BasisAxes& worldOrientation,
	f32 fovYInRadians, f32 aspectRatio, f32 nearClipDist, f32 farClipDist,
	const Vector3f& moveSpeed, const Vector3f& rotationSpeed)
	: m_WorldPosition(worldPosition)
	, m_WorldOrientation(worldOrientation)
	, m_FovYInRadians(fovYInRadians)
	, m_AspectRatio(aspectRatio)
	, m_NearClipDist(nearClipDist)
	, m_FarClipDist(farClipDist)
	, m_MoveSpeed(moveSpeed)
	, m_RotationSpeed(rotationSpeed)
	, m_Dirty(true)
{
	m_RotationInRadians.m_X = ArcSin(-worldOrientation.m_ZAxis.m_Y);
	f32 cosX = Cos(m_RotationInRadians.m_X);

	assert(!AreEqual(cosX, 0.0f, EPSILON) && "Encountered Gimbal lock - needs special handling");
	f32 rcpCosX = Rcp(cosX);

	m_RotationInRadians.m_Y = ArcTan(worldOrientation.m_ZAxis.m_X * rcpCosX, worldOrientation.m_ZAxis.m_Z * rcpCosX);
	m_RotationInRadians.m_Z = ArcTan(worldOrientation.m_XAxis.m_Y * rcpCosX, worldOrientation.m_YAxis.m_Y * rcpCosX);

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

const BasisAxes& Camera::GetWorldOrientation() const
{
	return m_WorldOrientation;
}

void Camera::SetWorldOrientation(const BasisAxes& worldOrientation)
{
	m_WorldOrientation = worldOrientation;
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

void Camera::Move(const Vector3f& moveDir, f32 deltaTimeInMS)
{
	const Vector3f strength = (deltaTimeInMS * m_MoveSpeed) * moveDir;
	
	m_WorldPosition += strength.m_X * m_WorldOrientation.m_XAxis;
	m_WorldPosition += strength.m_Y * m_WorldOrientation.m_YAxis;
	m_WorldPosition += strength.m_Z * m_WorldOrientation.m_ZAxis;
	
	m_Dirty = true;
}

void Camera::Rotate(const Vector3f& rotationDir, f32 deltaTimeInMS)
{	
	m_RotationInRadians.m_Y += (deltaTimeInMS * m_RotationSpeed.m_Y) * rotationDir.m_Y;
	m_RotationInRadians.m_X += (deltaTimeInMS * m_RotationSpeed.m_X) * rotationDir.m_X;
	m_RotationInRadians.m_Z += (deltaTimeInMS * m_RotationSpeed.m_Z) * rotationDir.m_Z;

	m_RotationInRadians.m_Y = NormalizeAngle(m_RotationInRadians.m_Y);
	m_RotationInRadians.m_X = Clamp(-PI_DIV_2, PI_DIV_2, m_RotationInRadians.m_X);
	m_RotationInRadians.m_Z = NormalizeAngle(m_RotationInRadians.m_Z);
	
	m_WorldOrientation = BasisAxes(CreateRotationZXYMatrix(m_RotationInRadians));
	m_Dirty = true;
}

void Camera::Update(f32 deltaTime)
{
	Vector3f moveDir(0.0f, 0.0f, 0.0f);
	Vector3f rotationDir(0.0f, 0.0f, 0.0f);

	if (KeyboardInput::IsKeyDown(KeyboardInput::Key_S))
		moveDir.m_Z = -1.0f;
	else if (KeyboardInput::IsKeyDown(KeyboardInput::Key_W))
		moveDir.m_Z = 1.0f;
	else if (KeyboardInput::IsKeyDown(KeyboardInput::Key_A))
		moveDir.m_X = -1.0f;
	else if (KeyboardInput::IsKeyDown(KeyboardInput::Key_D))
		moveDir.m_X = 1.0f;
	else if (KeyboardInput::IsKeyDown(KeyboardInput::Key_E))
		moveDir.m_Y = -1.0f;
	else if (KeyboardInput::IsKeyDown(KeyboardInput::Key_Q))
		moveDir.m_Y = 1.0f;

	if (KeyboardInput::IsKeyDown(KeyboardInput::Key_Up))
		rotationDir.m_X = -1.0f;
	else if (KeyboardInput::IsKeyDown(KeyboardInput::Key_Down))
		rotationDir.m_X = 1.0f;
	else if (KeyboardInput::IsKeyDown(KeyboardInput::Key_Left))
		rotationDir.m_Y = -1.0f;
	else if (KeyboardInput::IsKeyDown(KeyboardInput::Key_Right))
		rotationDir.m_Y = 1.0f;

	Move(moveDir, deltaTime);
	Rotate(rotationDir, deltaTime);
}

void Camera::RecalcMatricesIfDirty() const
{
	if (m_Dirty)
	{
		m_ViewMatrix = CreateLookAtMatrix(m_WorldPosition, m_WorldOrientation);
		m_ProjMatrix = CreatePerspectiveFovProjMatrix(m_FovYInRadians, m_AspectRatio, m_NearClipDist, m_FarClipDist);
		m_ViewProjMatrix = m_ViewMatrix * m_ProjMatrix;

		m_Dirty = false;
	}
}
