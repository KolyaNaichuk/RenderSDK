#include "Common/Material.h"

Material::Material(std::wstring name)
	: m_Name(std::move(name))
	, m_DiffuseColor(Vector3f::ZERO)
	, m_SpecularColor(Vector3f::ZERO)
	, m_Shininess(1.0f)
{
}
