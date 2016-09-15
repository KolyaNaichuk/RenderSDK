#pragma once

#include "Math/Matrix4.h"
#include "Math/Vector3.h"
#include "Math/Vector4.h"
#include "Math/Quaternion.h"
#include "Math/Frustum.h"

struct EulerAngles;
struct AxisAngle;

class Transform
{
public:
	Transform();
	Transform(const Quaternion& rotation, const Vector3f& position);
	Transform(const Vector3f& scaling, const Quaternion& rotation, const Vector3f& position);
	
	const Vector3f& GetScaling() const;
	void SetScaling(const Vector3f& scaling);

	const Quaternion& GetRotation() const;
	void SetRotation(const Quaternion& rotation);

	const Vector3f& GetPosition() const;
	void SetPosition(const Vector3f& position);

	const Matrix4f& GetLocalToWorldMatrix() const;
	const Matrix4f& GetWorldToLocalMatrix() const;	

private:
	enum DirtyFlags
	{
		DirtyFlag_None = 0,
		DirtyFlag_LocalToWorldMatrix = 1 << 0,
		DirtyFlag_WorldToLocalMatrix = 1 << 1,
		DirtyFlag_All = DirtyFlag_LocalToWorldMatrix | DirtyFlag_WorldToLocalMatrix
	};
	
	Vector3f m_Scaling;
	Quaternion m_Rotation;
	Vector3f m_Position;

	mutable Matrix4f m_LocalToWorldMatrix;
	mutable Matrix4f m_WorldToLocalMatrix;
	mutable u8 m_DirtyFlags;
};

const Matrix4f CreateTranslationMatrix(const Vector3f& offset);
const Matrix4f CreateTranslationMatrix(f32 xOffset, f32 yOffset, f32 zOffset);

const Matrix4f CreateScalingMatrix(const Vector3f& scale);
const Matrix4f CreateScalingMatrix(f32 xScale, f32 yScale, f32 zScale);
const Matrix4f CreateScalingMatrix(f32 scale);

const Matrix4f CreateRotationXMatrix(const Radian& angle);
const Matrix4f CreateRotationYMatrix(const Radian& angle);
const Matrix4f CreateRotationZMatrix(const Radian& angle);
const Matrix4f CreateRotationMatrix(const AxisAngle& axisAngle);
const Matrix4f CreateRotationMatrix(const EulerAngles& eulerAngles);
const Matrix4f CreateRotationMatrix(const Quaternion& quat);
const Matrix4f CreateRotationMatrix(const BasisAxes& basis);

const Matrix4f CreateLookAtMatrix(const Vector3f& eyePos, const Vector3f& lookAtPos, const Vector3f& upDir);

const Matrix4f CreateOrthoOffCenterProjMatrix(f32 leftX, f32 rightX, f32 bottomY, f32 topY, f32 nearZ, f32 farZ);
const Matrix4f CreateOrthoProjMatrix(f32 width, f32 height, f32 nearZ, f32 farZ);

const Matrix4f CreatePerspectiveProjMatrix(f32 nearWidth, f32 nearHeight, f32 nearZ, f32 farZ);
const Matrix4f CreatePerspectiveFovProjMatrix(const Radian& fovY, f32 aspectRatio, f32 nearZ, f32 farZ);

const BasisAxes ExtractBasisAxes(const Matrix4f& matrix);