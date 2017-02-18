#pragma once

#include "Math/Vector4.h"
#include "Math/Frustum.h"
#include "Common/SceneObject.h"

class Camera : public SceneObject
{
public:
	enum ProjType
	{
		ProjType_Ortho = 0,
		ProjType_Perspective
	};

	enum ClearFlags
	{
		ClearFlag_None = 0,
		ClearFlag_Color = 1,
		ClearFlag_Depth = 2,
		ClearFlag_DepthStencil = 4,
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

	u8 GetClearFlags() const;
	void SetClearFlags(u8 clearFlags);
		
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
	u8 m_ClearFlags;
	f32 m_FovYInRadians;
	f32 m_SizeY;

	mutable Matrix4f m_ProjMatrix;
	mutable u8 m_DirtyFlags;
};

const Frustum ExtractWorldFrustum(const Camera& camera);
