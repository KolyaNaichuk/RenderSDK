#include "Common/Material.h"

Material::Material(const Vector4f& ambientColor, const Vector4f& diffuseColor, const Vector4f& specularColor,
	f32 specularPower, const Vector4f& emissiveColor)
	: m_AmbientColor(ambientColor)
	, m_DiffuseColor(diffuseColor)
	, m_SpecularColor(specularColor)
	, m_SpecularPower(specularPower)
	, m_EmissiveColor(emissiveColor)
{
}
