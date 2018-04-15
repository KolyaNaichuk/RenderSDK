#pragma once

#include "RenderPasses/ShadowMapTileAllocator.h"
#include "Math/Matrix4.h"
#include "Math/Plane.h"
#include "Math/Sphere.h"
#include "Math/Vector3.h"

struct LightFrustum
{
	Plane m_LeftPlane;
	Plane m_RightPlane;
	Plane m_TopPlane;
	Plane m_BottomPlane;
};

struct PointLightRenderData
{
	Vector3f m_Color;
	Sphere m_WorldBounds;
	u32 m_AffectedScreenArea;
	f32 m_LightViewNearPlane;
	f32 m_LightRcpViewClipRange;
	f32 m_LightProjMatrix43;
	f32 m_LightProjMatrix33;
	Matrix4f m_ViewProjMatrices[kNumCubeMapFaces];
	ShadowMapTile m_ShadowMapTiles[kNumCubeMapFaces];
	LightFrustum m_WorldFrustums[kNumCubeMapFaces];
};

struct SpotLightRenderData
{
	Vector3f m_Color;
	Vector3f m_WorldSpaceDir;
	Sphere m_WorldBounds;
	f32 m_LightRange;
	f32 m_CosHalfInnerConeAngle;
	f32 m_CosHalfOuterConeAngle;
	f32 m_LightViewNearPlane;
	f32 m_LightRcpViewClipRange;
	f32 m_LightProjMatrix43;
	f32 m_LightProjMatrix33;
	u32 m_AffectedScreenArea;
	Matrix4f m_ViewProjMatrix;
	ShadowMapTile m_ShadowMapTile;
	LightFrustum m_WorldFrustum;
};