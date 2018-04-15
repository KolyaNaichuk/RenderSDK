#include "Scene/Light.h"
#include "Common/Color.h"

Light::Light(const std::string& name)
	: SceneObject(name)
	, m_Intensity(1.0f)
	, m_Color(Vector3f::ONE)
{
}

f32 Light::GetIntensity() const
{
	return m_Intensity;
}

void Light::SetIntensity(f32 inensity)
{
	m_Intensity = inensity;
}

const Vector3f& Light::GetColor() const
{
	return m_Color;
}

void Light::SetColor(const Vector3f& color)
{
	m_Color = color;
}

PointLight::PointLight(const std::string& name, f32 range, f32 shadowNearPlane)
	: Light(name)
	, m_Range(range)
	, m_ShadowNearPlane(shadowNearPlane)
{
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

void PointLight::SetShadowNearPlane(f32 shadowNearPlane)
{
	m_ShadowNearPlane = shadowNearPlane;
}

SpotLight::SpotLight(const std::string& name, f32 range, f32 innerConeAngleInRadians, f32 outerConeAngleInRadians, f32 shadowNearPlane)
	: Light(name)
	, m_Range(range)
	, m_InnerConeAngleInRadians(innerConeAngleInRadians)
	, m_OuterConeAngleInRadians(outerConeAngleInRadians)
	, m_ShadowNearPlane(shadowNearPlane)
{
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

DirectionalLight::DirectionalLight(const std::string& name)
	: Light(name)
{
}
