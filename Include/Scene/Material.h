#pragma once

#include "Common/Common.h"

struct Material
{
	Material(std::wstring name)
		: m_Name(std::move(name))
	{
	}
	
	enum
	{
		BaseColorTextureIndex,
		MetalnessTextureIndex,
		RougnessTextureIndex,
		NumTextures
	};
	
	std::wstring m_Name;
	std::wstring m_FilePaths[NumTextures];
};
