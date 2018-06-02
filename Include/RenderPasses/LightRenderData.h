#pragma once

#include "Math/Matrix4.h"
#include "Math/Sphere.h"
#include "Math/Vector3.h"
#include "Math/Frustum.h"

struct SpotLightRenderData
{
	Matrix4f m_LightViewProjMatrix;
	Vector3f m_Color;
	f32 m_LightRange;
	Vector3f m_WorldSpacePos;
	f32 m_CosHalfInnerConeAngle;
	Vector3f m_WorldSpaceDir;
	f32 m_CosHalfOuterConeAngle;
	Frustum m_WorldFrustum;
	Sphere m_WorldBounds;
	f32 m_NegativeExpShadowMapConstant;
	f32 m_LightViewNearPlane;
	f32 m_LightRcpViewClipRange;
	f32 m_LightProjMatrix43;
	f32 m_LightProjMatrix33;
	u32 m_LightID;
};