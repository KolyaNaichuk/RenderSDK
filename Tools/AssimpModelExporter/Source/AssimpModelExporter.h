#pragma once

#include "assimp/material.h"

struct AssimpMaterialConfig
{
	aiTextureType m_BaseColorTextureKey = aiTextureType_UNKNOWN;
	const char* m_pBaseColorKey = nullptr;

	aiTextureType m_MetalnessTextureKey = aiTextureType_UNKNOWN;
	const char* m_pMetalnessKey = nullptr;

	aiTextureType m_RoughnessTextureKey = aiTextureType_UNKNOWN;
	const char* m_pRoughnessKey = nullptr;

	aiTextureType m_EmissiveTextureKey = aiTextureType_UNKNOWN;
	const char* m_pEmissiveKey = nullptr;
};

struct ExportParams
{
	const wchar_t* m_pAssimpModelPath = nullptr;
	AssimpMaterialConfig m_AssimpModelMaterialConfig;

	const wchar_t* m_pExportedModelPath = nullptr;
};

class AssimpModelExporter
{
public:
	bool Export(const ExportParams* pParams);
};