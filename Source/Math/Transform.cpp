#include "Math/Transform.h"
#include "Math/Vector3f.h"
#include "Math/Math.h"

Transform::Transform()
	: m_Scaling(Vector3f::ONE)
	, m_Translation(Vector3f::ZERO)
	, m_LocalToWorldMatrix(Matrix4f::IDENTITY)
	, m_WorldToLocalMatrix(Matrix4f::IDENTITY)
	, m_DirtyFlags(DirtyFlag_None)
{
}

Transform::Transform(const Vector3f& scaling, const EulerAngles& rotation, const Vector3f& translation)
	: m_Scaling(scaling)
	, m_Rotation(rotation)
	, m_Translation(translation)
	, m_DirtyFlags(DirtyFlag_All)
{
}

const Vector3f& Transform::GetScaling() const
{
	return m_Scaling;
}

void Transform::SetScaling(const Vector3f& scaling)
{
	m_Scaling = scaling;
	m_DirtyFlags = DirtyFlag_All;
}

const EulerAngles& Transform::GetRotation() const
{
	return m_Rotation;
}

void Transform::SetRotation(const EulerAngles& rotation)
{
	m_Rotation = rotation;
	m_DirtyFlags = DirtyFlag_All;
}

const Vector3f& Transform::GetTranslation() const
{
	return m_Translation;
}

void Transform::SetTranslation(const Vector3f& translation)
{
	m_Translation = translation;
	m_DirtyFlags = DirtyFlag_All;
}

const Matrix4f& Transform::GetLocalToWorldMatrix() const
{
	if (m_DirtyFlags & DirtyFlag_LocalToWorldMatrix)
	{
		Matrix4f scalingMatrix = CreateScalingMatrix(m_Scaling);
		Matrix4f rotationMatrix = CreateRotationMatrix(m_Rotation);
		Matrix4f translationMatrix = CreateTranslationMatrix(m_Translation);

		m_LocalToWorldMatrix = scalingMatrix * rotationMatrix * translationMatrix;
		m_DirtyFlags &= ~DirtyFlag_LocalToWorldMatrix;
	}
	return m_LocalToWorldMatrix;
}

const Matrix4f& Transform::GetWorldToLocalMatrix() const
{
	if (m_DirtyFlags & DirtyFlag_WorldToLocalMatrix)
	{
		Matrix4f invScalingMatrix = CreateScalingMatrix(Rcp(m_Scaling));
		Matrix4f invRotationMatrix = CreateRotationMatrix(Negate(m_Rotation));
		Matrix4f invTranslationMatrix = CreateTranslationMatrix(Negate(m_Translation));

		m_WorldToLocalMatrix = invTranslationMatrix * invRotationMatrix * invScalingMatrix;
		m_DirtyFlags &= ~DirtyFlag_WorldToLocalMatrix;
	}
	return m_WorldToLocalMatrix;
}

const Vector4f Transform::TransformVector(const Vector4f& vec) const
{
	const Matrix4f& matrix = GetLocalToWorldMatrix();

	return Vector4f(vec.m_X * matrix.m_00 + vec.m_Y * matrix.m_10 + vec.m_Z * matrix.m_20 + vec.m_W * matrix.m_30,
					vec.m_X * matrix.m_01 + vec.m_Y * matrix.m_11 + vec.m_Z * matrix.m_21 + vec.m_W * matrix.m_31,
					vec.m_X * matrix.m_02 + vec.m_Y * matrix.m_12 + vec.m_Z * matrix.m_22 + vec.m_W * matrix.m_32,
					vec.m_X * matrix.m_03 + vec.m_Y * matrix.m_13 + vec.m_Z * matrix.m_23 + vec.m_W * matrix.m_33);
}

const Vector4f Transform::TransformNormal(const Vector4f& vec) const
{
	const Matrix4f& matrix = GetWorldToLocalMatrix();

	return Vector4f(vec.m_X * matrix.m_00 + vec.m_Y * matrix.m_01 + vec.m_Z * matrix.m_02 + vec.m_W * matrix.m_03,
					vec.m_X * matrix.m_10 + vec.m_Y * matrix.m_11 + vec.m_Z * matrix.m_12 + vec.m_W * matrix.m_13,
					vec.m_X * matrix.m_20 + vec.m_Y * matrix.m_21 + vec.m_Z * matrix.m_22 + vec.m_W * matrix.m_23,
					vec.m_X * matrix.m_30 + vec.m_Y * matrix.m_31 + vec.m_Z * matrix.m_32 + vec.m_W * matrix.m_33);
}

const Matrix4f CreateTranslationMatrix(const Vector3f& offset)
{
	return CreateTranslationMatrix(offset.m_X, offset.m_Y, offset.m_Z);
}

const Matrix4f CreateTranslationMatrix(f32 xOffset, f32 yOffset, f32 zOffset)
{
	return Matrix4f(1.0f, 0.0f, 0.0f, 0.0f,
					0.0f, 1.0f, 0.0f, 0.0f,
					0.0f, 0.0f, 1.0f, 0.0f,
					xOffset, yOffset, zOffset, 1.0f);
}

const Matrix4f CreateScalingMatrix(const Vector3f& scale)
{
	return CreateScalingMatrix(scale.m_X, scale.m_Y, scale.m_Z);
}

const Matrix4f CreateScalingMatrix(f32 xScale, f32 yScale, f32 zScale)
{
	return Matrix4f(xScale, 0.0f, 0.0f, 0.0f,
					0.0f, yScale, 0.0f, 0.0f,
					0.0f, 0.0f, zScale, 0.0f,
					0.0f, 0.0f, 0.0f, 1.0f);
}

const Matrix4f CreateScalingMatrix(f32 scale)
{
	return CreateScalingMatrix(scale, scale, scale);
}

const Matrix4f CreateRotationXMatrix(const Radian& angle)
{
	f32 sinAngle = Sin(angle);
	f32 cosAngle = Cos(angle);

	return Matrix4f(1.0f, 0.0f, 0.0f, 0.0f,
					0.0f, cosAngle, sinAngle, 0.0f,
					0.0f, -sinAngle, cosAngle, 0.0f,
					0.0f, 0.0f, 0.0f, 1.0f);
}

const Matrix4f CreateRotationYMatrix(const Radian& angle)
{
	f32 sinAngle = Sin(angle);
	f32 cosAngle = Cos(angle);

	return Matrix4f(cosAngle, 0.0f, -sinAngle, 0.0f,
					0.0f, 1.0f, 0.0f, 0.0f,
					sinAngle, 0.0f, cosAngle, 0.0f,
					0.0f, 0.0f, 0.0f, 1.0f);
}

const Matrix4f CreateRotationZMatrix(const Radian& angle)
{
	f32 sinAngle = Sin(angle);
	f32 cosAngle = Cos(angle);

	return Matrix4f(cosAngle, sinAngle, 0.0f, 0.0f,
				   -sinAngle, cosAngle, 0.0f, 0.0f,
					0.0f, 0.0f, 1.0f, 0.0f,
					0.0f, 0.0f, 0.0f, 1.0f);
}

const Matrix4f CreateRotationMatrix(const EulerAngles& eulerAngles)
{
	assert(false && "Needs impl");
	return Matrix4f();
}

const Matrix4f CreateLookAtMatrix(const Vector3f& eyePos, const Vector3f& lookAtPos, const Vector3f& upDir)
{
	Vector3f zAxis = Normalize(lookAtPos - eyePos);
	Vector3f xAxis = Normalize(Cross(upDir, zAxis));
	Vector3f yAxis = Cross(zAxis, xAxis);

	return Matrix4f(xAxis.m_X, yAxis.m_X, zAxis.m_X, 0.0f,
					xAxis.m_Y, yAxis.m_Y, zAxis.m_Y, 0.0f,
					xAxis.m_Z, yAxis.m_Z, zAxis.m_Z, 0.0f,
				   -Dot(xAxis, eyePos), -Dot(yAxis, eyePos), -Dot(zAxis, eyePos), 1.0f);
}

const Matrix4f CreateOrthoOffCenterProjMatrix(f32 leftX, f32 rightX, f32 bottomY, f32 topY, f32 nearZ, f32 farZ)
{
	return Matrix4f(2.0f / (rightX - leftX), 0.0f, 0.0f, 0.0f,
					0.0f, 2.0f / (topY - bottomY), 0.0f, 0.0f,
					0.0f, 0.0f, 1.0f / (farZ - nearZ), 0.0f,
				   (leftX + rightX) / (leftX - rightX), (topY + bottomY) / (bottomY - topY), nearZ / (nearZ / farZ), 1.0f);
}

const Matrix4f CreateOrthoProjMatrix(f32 width, f32 height, f32 nearZ, f32 farZ)
{
	return Matrix4f(2.0f / width, 0.0f, 0.0f, 0.0f,
					0.0f, 2.0f / height, 0.0f, 0.0f,
					0.0f, 0.0f, 1.0f / (farZ - nearZ), 0.0f,
					0.0f, 0.0f, -nearZ / (farZ - nearZ), 1.0f);
}

const Matrix4f CreatePerspectiveProjMatrix(f32 nearWidth, f32 nearHeight, f32 nearZ, f32 farZ)
{
	return Matrix4f(2.0f * nearZ / nearWidth, 0.0f, 0.0f, 0.0f,
					0.0f, 2 * nearZ / nearHeight, 0.0f, 0.0f,
					0.0f, 0.0f, farZ / (farZ - nearZ), 1.0f,
					0.0f, 0.0f, nearZ * farZ / (nearZ - farZ), 0.0f);
}

const Matrix4f CreatePerspectiveFovProjMatrix(const Radian& fovY, f32 aspectRatio, f32 nearZ, f32 farZ)
{
	f32 yScale = Rcp(Tan(0.5f * fovY));
	f32 xScale = yScale / aspectRatio;

	return Matrix4f(xScale, 0.0f, 0.0f, 0.0f,
					0.0f, yScale, 0.0f, 0.0f,
					0.0f, 0.0f, farZ / (farZ - nearZ), 1.0f,
					0.0f, 0.0f, nearZ * farZ / (nearZ - farZ), 0.0f);
}

const Matrix4f CreateMatrixFromUpDirection(const Vector3f& upDir)
{
	assert(false && "Needs impl");
	return Matrix4f();
}

const Matrix4f CreateMatrixFromForwardDirection(const Vector3f& forwardDir)
{
	assert(false && "Needs impl");
	return Matrix4f();
}

const Vector3f GetUpDirection(const Matrix4f& matrix)
{
	assert(false && "Needs impl");
	return Vector3f();
}

const Vector3f GetForwardDirection(const Matrix4f& matrix)
{
	assert(false && "Needs impl");
	return Vector3f();
}

const Vector3f GetRightDirection(const Matrix4f& matrix)
{
	assert(false && "Needs impl");
	return Vector3f();
}
