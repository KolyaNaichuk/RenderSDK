#include "Common/Material.h"

Material::Material(const std::wstring& name)
	: m_Name(name)
	, m_AmbientColor(Vector3f::ZERO)
	, m_DiffuseColor(Vector3f::ZERO)
	, m_SpecularColor(Vector3f::ZERO)
	, m_Shininess(1.0f)
	, m_EmissiveColor(Vector3f::ZERO)
	, m_Opacity(1.0f)
	, m_IndexOfRefraction(1.0f)
{
}
