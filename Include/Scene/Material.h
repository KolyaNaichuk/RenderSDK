#pragma once

#include "Math/Vector3.h"

struct Material
{
	Material(std::wstring name)
		: m_Name(std::move(name))
	{
	}

	std::wstring m_Name;
	
	std::wstring m_BaseColorTexturePath;
	Vector3f m_BaseColor = Vector3f::ZERO;
		
	std::wstring m_MetalnessTexturePath;
	f32 m_Metalness = 0.0f;
		
	std::wstring m_RoughnessTexturePath;
	f32 m_Roughness = 0.0f;

	std::wstring m_EmissiveTexturePath;
	Vector3f m_EmissiveColor = Vector3f::ZERO;
};
