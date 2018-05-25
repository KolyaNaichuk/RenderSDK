#pragma once

#include "Math/Matrix4.h"
#include "Math/Sphere.h"
#include "Math/Vector3.h"
#include "Math/Frustum.h"

struct SpotLightRenderData
{
	Vector3f m_Color;
	Vector3f m_WorldSpacePos;
	Vector3f m_WorldSpaceDir;
	Frustum m_WorldFrustum;
	f32 m_LightRange;
	f32 m_CosHalfInnerConeAngle;
	f32 m_CosHalfOuterConeAngle;
	f32 m_LightViewNearPlane;
	f32 m_LightRcpViewClipRange;
	f32 m_LightProjMatrix43;
	f32 m_LightProjMatrix33;
	Matrix4f m_ViewProjMatrix;
};