#pragma once

#include "assimp/material.h"
#include <string>

enum MaterialProps
{
	BaseColorProp,
	MetalnessProp,
	RoughnessProp,
	EmissiveColorProp,
	NumMaterialProps
};

struct SourceMaterialConfig
{
	aiTextureType m_TextureTypes[NumMaterialProps] = {aiTextureType_UNKNOWN, aiTextureType_UNKNOWN, aiTextureType_UNKNOWN, aiTextureType_UNKNOWN};
	std::string m_Keys[NumMaterialProps];
};

struct DestMaterialConfig
{
	aiTextureType m_TextureTypes[NumMaterialProps] = {aiTextureType_UNKNOWN, aiTextureType_UNKNOWN, aiTextureType_UNKNOWN, aiTextureType_UNKNOWN};
};