#pragma once

#include "Math/Matrix4f.h"
#include "Math/Vector3f.h"
#include "Math/Vector4f.h"
#include "Math/EulerAngles.h"

class Transform
{
public:
	Transform();
	Transform(const Vector3f& translation, const EulerAngles& rotation, const Vector3f& scale);
	Transform(const Matrix4f& localToWorldMatrix);

	const Vector3f& GetTranslation() const;
	void SetTranslation(const Vector3f& translation);

	const EulerAngles& GetRotation() const;
	void SetRotation(const EulerAngles& rotation);

	const Vector3f& GetScale() const;
	void SetScale(const Vector3f& scale);

	const Vector3f& GetPosition() const;
	void SetPosition(const Vector3f& position);

	const Matrix4f CalcLocalToWorldMatrix() const;
	const Matrix4f CalcWorldToLocalMatrix() const;

	const Vector3f CalcUpAxis() const;
	const Vector3f CalcForwardAxis() const;
	const Vector3f CalcRightAxis() const;

	static const Vector4f TransformVector(const Vector4f& vec, const Matrix4f& matrix);
	static const Vector4f TransformNormal(const Vector4f& vec, const Matrix4f& matrix);

private:
	Vector3f m_Translation;
	EulerAngles m_Rotation;
	Vector3f m_Scale;
};

const Matrix4f CreateTranslationMatrix(f32 xOffset, f32 yOffset, f32 zOffset);
const Matrix4f CreateScalingMatrix(f32 xScale, f32 yScale, f32 zScale);
const Matrix4f CreateRotationXMatrix(const Radian& angle);
const Matrix4f CreateRotationYMatrix(const Radian& angle);
const Matrix4f CreateRotationZMatrix(const Radian& angle);
const Matrix4f CreateLookAtMatrix(const Vector3f& eyePos, const Vector3f& lookAtPos, const Vector3f& upDir);
const Matrix4f CreateOrthoOffCenterProjMatrix(f32 leftX, f32 rightX, f32 bottomY, f32 topY, f32 nearZ, f32 farZ);
const Matrix4f CreateOrthoProjMatrix(f32 width, f32 height, f32 nearZ, f32 farZ);
const Matrix4f CreatePerspectiveProjMatrix(f32 nearWidth, f32 nearHeight, f32 nearZ, f32 farZ);
const Matrix4f CreatePerspectiveFovProjMatrix(const Radian& fovY, f32 aspectRatio, f32 nearZ, f32 farZ);
const Matrix4f CreateMatrixFromUpDirection(const Vector3f& upDir);
const Matrix4f CreateMatrixFromForwardDirection(const Vector3f& forwardDir);

const Vector3f GetUpDirection(const Matrix4f& matrix);
const Vector3f GetForwardDirection(const Matrix4f& matrix);
const Vector3f GetRightDirection(const Matrix4f& matrix);