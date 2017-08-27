#pragma once

#include "Math/Vector3.h"

struct Material
{
	Material(const std::wstring& name);

	std::wstring m_Name;

	Vector3f m_AmbientColor;
	std::wstring m_AmbientMapName;

	Vector3f m_DiffuseColor;
	std::wstring m_DiffuseMapName;

	Vector3f m_SpecularColor;
	std::wstring m_SpecularMapName;

	f32 m_Shininess;
	std::wstring m_ShininessMapName;

	Vector3f m_EmissiveColor;
	std::wstring m_EmissiveMapName;

	f32 m_Opacity;
	std::wstring m_OpacityMapName;

	f32 m_IndexOfRefraction;
};
