#include "Scene/Light.h"
#include "Math/Math.h"

PointLight::PointLight(const Vector3f& worldPosition, const Vector3f& radiantPower, f32 range, f32 shadowNearPlane, f32 expShadowMapConstant)
	: m_WorldPosition(worldPosition)
	, m_RadiantPower(radiantPower)
	, m_Range(range)
	, m_ShadowNearPlane(shadowNearPlane)
	, m_ExpShadowMapConstant(expShadowMapConstant)
{
}

const Vector3f& PointLight::GetWorldPosition() const
{
	return m_WorldPosition;
}

void PointLight::SetWorldPosition(const Vector3f& worldPosition)
{
	m_WorldPosition = worldPosition;
}

Vector3f PointLight::EvaluateRadiantIntensity() const
{
	return (m_RadiantPower / (4.0f * PI));
}

const Vector3f& PointLight::GetRadiantPower() const
{
	return m_RadiantPower;
}

void PointLight::SetRadiantPower(const Vector3f& radiantPower)
{
	m_RadiantPower = radiantPower;
}

f32 PointLight::GetRange() const
{
	return m_Range;
}

void PointLight::SetRange(f32 range)
{
	m_Range = range;
}

f32 PointLight::GetShadowNearPlane() const
{
	return m_ShadowNearPlane;
}

void PointLight::SetShadowNearPlane(float shadowNearPlane)
{
	m_ShadowNearPlane = shadowNearPlane;
}

f32 PointLight::GetExpShadowMapConstant() const
{
	return m_ExpShadowMapConstant;
}

void PointLight::SetExpShadowMapConstant(f32 expShadowMapConstant)
{
	m_ExpShadowMapConstant = expShadowMapConstant;
}

SpotLight::SpotLight(const Vector3f& worldPosition, const BasisAxes& worldOrientation, const Vector3f& radiantPower, f32 range,
	f32 innerConeAngleInRadians, f32 outerConeAngleInRadians, f32 shadowNearPlane, f32 expShadowMapConstant)
	: m_WorldPosition(worldPosition)
	, m_WorldOrientation(worldOrientation)
	, m_RadiantPower(radiantPower)
	, m_Range(range)
	, m_InnerConeAngleInRadians(innerConeAngleInRadians)
	, m_OuterConeAngleInRadians(outerConeAngleInRadians)
	, m_ShadowNearPlane(shadowNearPlane)
	, m_ExpShadowMapConstant(expShadowMapConstant)
{
}

const Vector3f& SpotLight::GetWorldPosition() const
{
	return m_WorldPosition;
}

void SpotLight::SetWorldPosition(const Vector3f& worldPosition)
{
	m_WorldPosition = worldPosition;
}

const BasisAxes& SpotLight::GetWorldOrientation() const
{
	return m_WorldOrientation;
}

void SpotLight::SetWorldOrientation(const BasisAxes& worldOrientation)
{
	m_WorldOrientation = worldOrientation;
}

const Vector3f& SpotLight::GetRadiantPower() const
{
	return m_RadiantPower;
}

void SpotLight::SetRadiantPower(const Vector3f& radiantPower)
{
	m_RadiantPower = radiantPower;
}

Vector3f SpotLight::EvaluateRadiantIntensity() const
{
	return (m_RadiantPower / (TWO_PI * (1.0f - Cos(0.5f * m_OuterConeAngleInRadians))));
}

f32 SpotLight::GetRange() const
{
	return m_Range;
}

void SpotLight::SetRange(f32 range)
{
	m_Range = range;
}

f32 SpotLight::GetInnerConeAngle() const
{
	return m_InnerConeAngleInRadians;
}

void SpotLight::SetInnerConeAngle(f32 innerConeAngleInRadians)
{
	m_InnerConeAngleInRadians = innerConeAngleInRadians;
}

f32 SpotLight::GetOuterConeAngle() const
{
	return m_OuterConeAngleInRadians;
}

void SpotLight::SetOuterConeAngle(f32 outerConeAngleInRadians)
{
	m_OuterConeAngleInRadians = outerConeAngleInRadians;
}

f32 SpotLight::GetShadowNearPlane() const
{
	return m_ShadowNearPlane;
}

void SpotLight::SetShadowNearPlane(f32 shadowNearPlane)
{
	m_ShadowNearPlane = shadowNearPlane;
}

f32 SpotLight::GetExpShadowMapConstant() const
{
	return m_ExpShadowMapConstant;
}

void SpotLight::SetExpShadowMapConstant(f32 expShadowMapConstant)
{
	m_ExpShadowMapConstant = expShadowMapConstant;
}
