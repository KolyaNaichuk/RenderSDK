#pragma once

#include "Math/Vector3.h"
#include "Math/Vector4.h"
#include "Math/Matrix4.h"
#include "Math/BasisAxes.h"

class Camera
{
public:
	Camera(const Vector3f& worldPosition, const BasisAxes& worldOrientation,
		f32 fovYInRadians, f32 aspectRatio, f32 nearClipDist, f32 farClipDist,
		const Vector3f& moveSpeed = Vector3f::ZERO,
		const Vector3f& rotationSpeed = Vector3f::ZERO);
	
	const Vector3f& GetWorldPosition() const;
	void SetWorldPosition(const Vector3f& worldPosition);

	const BasisAxes& GetWorldOrientation() const;
	void SetWorldOrientation(const BasisAxes& worldOrientation);

	f32 GetFieldOfViewY() const;
	void SetFieldOfViewY(f32 fovYInRadians);

	f32 GetAspectRatio() const;
	void SetAspectRatio(f32 aspectRatio);

	f32 GetNearClipDistance() const;
	void SetNearClipDistance(f32 nearClipDist);

	f32 GetFarClipDistance() const;
	void SetFarClipDistance(f32 farClipDist);

	const Vector3f& GetMoveSpeed() const;
	void SetMoveSpeed(const Vector3f& moveSpeed);

	const Vector3f& GetRotationSpeed() const;
	void SetRotationSpeed(const Vector3f& rotationSpeed);
	
	const Matrix4f& GetViewMatrix() const;
	const Matrix4f& GetProjMatrix() const;
	const Matrix4f& GetViewProjMatrix() const;
	
	void Move(const Vector3f& moveDelta, f32 deltaTime);
	void Rotate(const Vector3f& rotationDeltaInRadians, f32 deltaTime);

private:
	void RecalcMatricesIfDirty() const;

private:
	Vector3f m_WorldPosition;
	BasisAxes m_WorldOrientation;
	Vector3f m_RotationInRadians;

	f32 m_FovYInRadians;
	f32 m_AspectRatio;
	f32 m_NearClipDist;
	f32 m_FarClipDist;
	Vector3f m_MoveSpeed;
	Vector3f m_RotationSpeed;
	
	mutable bool m_Dirty;
	mutable Matrix4f m_ViewMatrix;
	mutable Matrix4f m_ProjMatrix;
	mutable Matrix4f m_ViewProjMatrix;
};
