#pragma once

#include "assimp/material.h"

struct SourceMaterialConfig
{
	aiTextureType m_BaseColorTextureKey = aiTextureType_UNKNOWN;
	std::string m_BaseColorKey;

	aiTextureType m_MetalnessTextureKey = aiTextureType_UNKNOWN;
	std::string m_MetalnessKey;

	aiTextureType m_RoughnessTextureKey = aiTextureType_UNKNOWN;
	std::string m_RoughnessKey;

	aiTextureType m_EmissiveTextureKey = aiTextureType_UNKNOWN;
	std::string m_EmissiveKey;
};

struct DestMaterialConfig
{
	aiTextureType m_BaseColorTextureKey = aiTextureType_UNKNOWN;
	aiTextureType m_MetalnessTextureKey = aiTextureType_UNKNOWN;
	aiTextureType m_RoughnessTextureKey = aiTextureType_UNKNOWN;
	aiTextureType m_EmissiveTextureKey = aiTextureType_UNKNOWN;
};