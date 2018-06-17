#include "Math/Transform.h"
#include "Math/AxisAngle.h"
#include "Math/Vector3.h"

Transform::Transform()
	: m_Scaling(Vector3f::ONE)
	, m_Position(Vector3f::ZERO)
	, m_LocalToWorldMatrix(Matrix4f::IDENTITY)
	, m_WorldToLocalMatrix(Matrix4f::IDENTITY)
	, m_DirtyFlags(DirtyFlag_None)
{
}

Transform::Transform(const Quaternion& rotation, const Vector3f& position)
	: Transform(Vector3f::ONE, rotation, position)
{
}

Transform::Transform(const Vector3f& scaling, const Quaternion& rotation, const Vector3f& position)
	: m_Scaling(scaling)
	, m_Rotation(rotation)
	, m_Position(position)
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

const Quaternion& Transform::GetRotation() const
{
	return m_Rotation;
}

void Transform::SetRotation(const Quaternion& rotation)
{
	m_Rotation = rotation;
	m_DirtyFlags = DirtyFlag_All;
}

const Vector3f& Transform::GetPosition() const
{
	return m_Position;
}

void Transform::SetPosition(const Vector3f& position)
{
	m_Position = position;
	m_DirtyFlags = DirtyFlag_All;
}

const Matrix4f& Transform::GetLocalToWorldMatrix() const
{
	if (m_DirtyFlags & DirtyFlag_LocalToWorldMatrix)
	{
		Matrix4f scalingMatrix = CreateScalingMatrix(m_Scaling);
		Matrix4f rotationMatrix = CreateRotationMatrix(m_Rotation);
		Matrix4f translationMatrix = CreateTranslationMatrix(m_Position);

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
		Matrix4f invRotationMatrix = Transpose(CreateRotationMatrix(m_Rotation));
		Matrix4f invTranslationMatrix = CreateTranslationMatrix(-m_Position);

		m_WorldToLocalMatrix = invTranslationMatrix * invRotationMatrix * invScalingMatrix;
		m_DirtyFlags &= ~DirtyFlag_WorldToLocalMatrix;
	}
	return m_WorldToLocalMatrix;
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

const Matrix4f CreateRotationXMatrix(f32 angleInRadians)
{
	f32 sinAngle, cosAngle;
	SinCos(sinAngle, cosAngle, angleInRadians);

	return Matrix4f(1.0f, 0.0f, 0.0f, 0.0f,
					0.0f, cosAngle, sinAngle, 0.0f,
					0.0f, -sinAngle, cosAngle, 0.0f,
					0.0f, 0.0f, 0.0f, 1.0f);
}

const Matrix4f CreateRotationYMatrix(f32 angleInRadians)
{
	f32 sinAngle, cosAngle;
	SinCos(sinAngle, cosAngle, angleInRadians);

	return Matrix4f(cosAngle, 0.0f, -sinAngle, 0.0f,
					0.0f, 1.0f, 0.0f, 0.0f,
					sinAngle, 0.0f, cosAngle, 0.0f,
					0.0f, 0.0f, 0.0f, 1.0f);
}

const Matrix4f CreateRotationZMatrix(f32 angleInRadians)
{
	f32 sinAngle, cosAngle;
	SinCos(sinAngle, cosAngle, angleInRadians);

	return Matrix4f(cosAngle, sinAngle, 0.0f, 0.0f,
				   -sinAngle, cosAngle, 0.0f, 0.0f,
					0.0f, 0.0f, 1.0f, 0.0f,
					0.0f, 0.0f, 0.0f, 1.0f);
}

const Matrix4f CreateRotationYXZMatrix(const Vector3f& anglesInRadians)
{
	return CreateRotationYMatrix(anglesInRadians.m_Y) * CreateRotationXMatrix(anglesInRadians.m_X) * CreateRotationZMatrix(anglesInRadians.m_Z);
}

const Matrix4f CreateRotationZXYMatrix(const Vector3f& anglesInRadians)
{
	f32 sinZ, cosZ;
	f32 sinX, cosX;
	f32 sinY, cosY;

	SinCos(sinZ, cosZ, anglesInRadians.m_Z);
	SinCos(sinX, cosX, anglesInRadians.m_X);
	SinCos(sinY, cosY, anglesInRadians.m_Y);
		
	return Matrix4f(cosY * cosZ + sinY * sinX * sinZ, sinZ * cosX, -sinY * cosZ + cosY * sinX * sinZ, 0.0f,
				   -cosY * sinZ + sinY * sinX * cosZ, cosZ * cosX, sinZ * sinY + cosY * sinX * cosZ, 0.0f,
					sinY * cosX, -sinX, cosY * cosX, 0.0f,
					0.0f, 0.0f, 0.0f, 1.0f);
}

const Matrix4f CreateRotationMatrix(const AxisAngle& axisAngle)
{
	assert(IsNormalized(axisAngle.m_Axis));

	f32 sinAngle, cosAngle;
	SinCos(sinAngle, cosAngle, axisAngle.m_AngleInRadians);

	Vector3f sinAxis = sinAngle * axisAngle.m_Axis;
	Vector3f cosAxis = (1.0f - cosAngle) * axisAngle.m_Axis;

	Vector3f xAxis = axisAngle.m_Axis.m_X * cosAxis + Vector3f(cosAngle, sinAxis.m_Z, -sinAxis.m_Y);
	Vector3f yAxis = axisAngle.m_Axis.m_Y * cosAxis + Vector3f(-sinAxis.m_Z, cosAngle, sinAxis.m_X);
	Vector3f zAxis = axisAngle.m_Axis.m_Z * cosAxis + Vector3f(sinAxis.m_Y, -sinAxis.m_X, cosAngle);

	return CreateRotationMatrix(BasisAxes(xAxis, yAxis, zAxis));
}

const Matrix4f CreateRotationMatrix(const Quaternion& quat)
{
	return CreateRotationMatrix(ExtractBasisAxes(quat));
}

const Matrix4f CreateRotationMatrix(const BasisAxes& basis)
{
	return Matrix4f(basis.m_XAxis.m_X, basis.m_XAxis.m_Y, basis.m_XAxis.m_Z, 0.0f,
					basis.m_YAxis.m_X, basis.m_YAxis.m_Y, basis.m_YAxis.m_Z, 0.0f,
					basis.m_ZAxis.m_X, basis.m_ZAxis.m_Y, basis.m_ZAxis.m_Z, 0.0f,
					0.0f, 0.0f, 0.0f, 1.0f);
}

const Matrix4f CreateLookAtMatrix(const Vector3f& eyePos, const BasisAxes& basisAxes)
{
	const Vector3f& xAxis = basisAxes.m_XAxis;
	const Vector3f& yAxis = basisAxes.m_YAxis;
	const Vector3f& zAxis = basisAxes.m_ZAxis;

	return Matrix4f(xAxis.m_X, yAxis.m_X, zAxis.m_X, 0.0f,
					xAxis.m_Y, yAxis.m_Y, zAxis.m_Y, 0.0f,
					xAxis.m_Z, yAxis.m_Z, zAxis.m_Z, 0.0f,
				   -Dot(xAxis, eyePos), -Dot(yAxis, eyePos), -Dot(zAxis, eyePos), 1.0f);
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

const Matrix4f CreatePerspectiveFovProjMatrix(f32 fovYInRadians, f32 aspectRatio, f32 nearZ, f32 farZ)
{
	f32 yScale = Rcp(Tan(0.5f * fovYInRadians));
	f32 xScale = yScale / aspectRatio;

	return Matrix4f(xScale, 0.0f, 0.0f, 0.0f,
					0.0f, yScale, 0.0f, 0.0f,
					0.0f, 0.0f, farZ / (farZ - nearZ), 1.0f,
					0.0f, 0.0f, nearZ * farZ / (nearZ - farZ), 0.0f);
}

