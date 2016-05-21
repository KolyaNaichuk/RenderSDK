#include "Common/Material.h"

Material::Material()
	: m_AmbientColor(Vector4f::ZERO)
	, m_DiffuseColor(Vector4f::ZERO)
	, m_SpecularColor(Vector4f::ZERO)
	, m_SpecularPower(0.0f)
	, m_EmissiveColor(Vector4f::ZERO)
{
}

Material::Material(const Vector4f& ambientColor, const Vector4f& diffuseColor,
	const Vector4f& specularColor, f32 specularPower, const Vector4f& emissiveColor)
	: m_AmbientColor(ambientColor)
	, m_DiffuseColor(diffuseColor)
	, m_SpecularColor(specularColor)
	, m_SpecularPower(specularPower)
	, m_EmissiveColor(emissiveColor)
{
}
