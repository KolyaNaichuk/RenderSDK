#pragma once

#include "Math/Vector4.h"
#include "Common/SceneObject.h"

class Camera : public SceneObject
{
public:
	enum ProjType
	{
		ProjType_Ortho = 0,
		ProjType_Perspective
	};

	Camera(ProjType projType, f32 nearClipPlane, f32 farClipPlane, f32 aspectRatio);

	ProjType GetProjType() const;
	void SetProjType(ProjType projType);

	const Vector4f& GetBackgroundColor() const;
	void SetBackgroundColor(const Vector4f& backgroundColor);

	f32 GetNearClipPlane() const;
	void SetNearClipPlane(f32 nearClipPlane);

	f32 GetFarClipPlane() const;
	void SetFarClipPlane(f32 farClipPlane);

	f32 GetAspectRatio() const;
	void SetAspectRatio(f32 aspectRatio);
			
	f32 GetFovY() const;
	void SetFovY(f32 fovYInRadians);

	f32 GetSizeY() const;
	void SetSizeY(f32 sizeY);

	const Matrix4f& GetViewMatrix() const;
	const Matrix4f& GetProjMatrix() const;

private:
	enum DirtyFlags
	{
		DirtyFlag_None = 0,
		DirtyFlag_ProjMatrix = 1 << 0
	};

	ProjType m_ProjType;
	Vector4f m_BackgroundColor;
	f32 m_NearClipPlane;
	f32 m_FarClipPlane;
	f32 m_AspectRatio;
	f32 m_FovYInRadians;
	f32 m_SizeY;

	mutable Matrix4f m_ProjMatrix;
	mutable u8 m_DirtyFlags;
};
