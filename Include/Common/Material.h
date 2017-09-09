#pragma once

#include "Math/Vector3.h"

struct Material
{
	Material(const std::wstring& name);

	std::wstring m_Name;

	Vector3f m_AmbientColor;
	std::wstring m_AmbientMapFilePath;

	Vector3f m_DiffuseColor;
	std::wstring m_DiffuseMapFilePath;

	Vector3f m_SpecularColor;
	std::wstring m_SpecularMapFilePath;

	f32 m_Shininess;
	std::wstring m_ShininessMapFilePath;

	Vector3f m_EmissiveColor;
	std::wstring m_EmissiveMapFilePath;

	f32 m_Opacity;
	std::wstring m_OpacityMapFilePath;

	f32 m_IndexOfRefraction;
};
