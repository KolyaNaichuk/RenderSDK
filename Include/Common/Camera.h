#pragma once

#include "Math/Vector4f.h"
#include "Math/Radian.h"
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

	enum OpaqueSortMode
	{
		OpaqueSortMode_FrontToBack = 0,
		OpaqueSortMode_NoDistanceSort
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

	u8 GetCullingMask() const;
	void SetCullingMask(u8 cullingMask);

	bool UseOcclusionCulling() const;
	void SetUseOcclusionCulling(bool useOcclusionCulling);

	OpaqueSortMode GetOpaqueSortMode() const;
	void SetOpaqueSortMode(OpaqueSortMode opaqueSortMode);

	const Radian& GetFovY() const;
	void SetFovY(const Radian& fovY);

	f32 GetSizeY() const;
	void SetSizeY(f32 sizeY);

private:
	ProjType m_ProjType;
	Vector4f m_BackgroundColor;
	f32 m_NearClipPlane;
	f32 m_FarClipPlane;
	f32 m_AspectRatio;
	u8 m_ClearFlags;
	u8 m_CullingMask;
	bool m_UseOcclusionCulling;
	OpaqueSortMode m_OpaqueSortMode;
	Radian m_FovY;
	f32 m_SizeY;
};
