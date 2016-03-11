#include "Common/Light.h"
#include "Common/Color.h"

Light::Light(const std::string& name)
	: SceneObject(name)
	, m_AffectsWorld(true)
	, m_Intensity(1.0f)
	, m_Color(Color::WHITE)
	, m_CastsShadows(false)
	, m_ShadowBias(0.0f)
{
}

bool Light::AffectsWorld() const
{
	return m_AffectsWorld;
}

void Light::SetAffectsWorld(bool affectsWorld)
{
	m_AffectsWorld = affectsWorld;
}

f32 Light::GetIntensity() const
{
	return m_Intensity;
}

void Light::SetIntensity(f32 inensity)
{
	m_Intensity = inensity;
}

const Vector4f& Light::GetColor() const
{
	return m_Color;
}

void Light::SetColor(const Vector4f& color)
{
	m_Color = color;
}

bool Light::CastsShadows() const
{
	return m_CastsShadows;
}

void Light::SetCastsShadows(bool castsShadows)
{
	m_CastsShadows = castsShadows;
}

f32 Light::GetShadowBias() const
{
	return m_ShadowBias;
}

void Light::SetShadowBias(f32 shadowBias)
{
	m_ShadowBias = shadowBias;
}

PointLight::PointLight(const std::string& name, f32 range)
	: Light(name)
	, m_Range(range)
	, m_AttenStartRange(0.0f)
	, m_AttenEndRange(range)
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

f32 PointLight::GetAttenStartRange() const
{
	return m_AttenStartRange;
}

void PointLight::SetAttenStartRange(f32 attenStartRange)
{
	m_AttenStartRange = attenStartRange;
}

f32 PointLight::GetAttenEndRange() const
{
	return m_AttenEndRange;
}

void PointLight::SetAttenEndRange(f32 attenEndRange)
{
	m_AttenEndRange = attenEndRange;
}

SpotLight::SpotLight(const std::string& name, f32 range, f32 falloff, const Radian& innerConeAngle, const Radian& outerConeAngle)
	: Light(name)
	, m_Range(range)
	, m_AttenStartRange(0.0f)
	, m_AttenEndRange(range)
	, m_FallOff(falloff)
	, m_InnerConeAngle(innerConeAngle)
	, m_OuterConeAngle(outerConeAngle)
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

f32 SpotLight::GetAttenStartRange() const
{
	return m_AttenStartRange;
}

void SpotLight::SetAttenStartRange(f32 attenStartRange)
{
	m_AttenStartRange = attenStartRange;
}

f32 SpotLight::GetAttenEndRange() const
{
	return m_AttenEndRange;
}

void SpotLight::SetAttenEndRange(f32 attenEndRange)
{
	m_AttenEndRange = attenEndRange;
}

f32 SpotLight::GetFalloff() const
{
	return m_FallOff;
}

void SpotLight::SetFalloff(f32 falloff)
{
	m_FallOff = falloff;
}

const Radian& SpotLight::GetInnerConeAngle() const
{
	return m_InnerConeAngle;
}

void SpotLight::SetInnerConeAngle(const Radian& innerConeAngle)
{
	m_InnerConeAngle = innerConeAngle;
}

const Radian& SpotLight::GetOuterConeAngle() const
{
	return m_OuterConeAngle;
}

void SpotLight::SetOuterConeAngle(const Radian& outerConeAngle)
{
	m_OuterConeAngle = outerConeAngle;
}

DirectionalLight::DirectionalLight(const std::string& name)
	: Light(name)
{
}
