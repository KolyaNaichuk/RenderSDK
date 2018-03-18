#pragma once

#include "Math/Vector3.h"

struct Material
{
	Material(std::wstring name);
	std::wstring m_Name;
	
	Vector3f m_DiffuseColor;
	std::wstring m_DiffuseMapFilePath;

	Vector3f m_SpecularColor;
	std::wstring m_SpecularMapFilePath;

	f32 m_Shininess;
	std::wstring m_ShininessMapFilePath;
};
