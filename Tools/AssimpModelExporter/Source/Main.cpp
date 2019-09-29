#include "AssimpModelExporter.h"

int main()
{
	ExportParams params;
	
	params.m_SourceMaterialConfig.m_BaseColorTextureKey = aiTextureType_DIFFUSE;
	params.m_SourceMaterialConfig.m_BaseColorKey = GetColorKey(AI_MATKEY_COLOR_DIFFUSE);
	params.m_SourceMaterialConfig.m_MetalnessTextureKey = aiTextureType_REFLECTION;
	params.m_SourceMaterialConfig.m_MetalnessKey = GetColorKey(AI_MATKEY_REFLECTIVITY);
	params.m_SourceMaterialConfig.m_RoughnessTextureKey = aiTextureType_SHININESS;
	params.m_SourceMaterialConfig.m_RoughnessKey = GetColorKey(AI_MATKEY_SHININESS);
	params.m_SourceMaterialConfig.m_EmissiveColorTextureKey = aiTextureType_EMISSIVE;
	params.m_SourceMaterialConfig.m_EmissiveColorKey = GetColorKey(AI_MATKEY_COLOR_EMISSIVE);
	params.m_SourceFolderPath = "D:\\Assimp Models\\Model 1";
	params.m_SourceOBJFileName = "Model.obj";
	
	params.m_DestMaterialConfig.m_BaseColorTextureKey = aiTextureType_DIFFUSE;
	params.m_DestMaterialConfig.m_MetalnessTextureKey = aiTextureType_REFLECTION;
	params.m_DestMaterialConfig.m_RoughnessTextureKey = aiTextureType_SHININESS;
	params.m_DestMaterialConfig.m_EmissiveColorTextureKey = aiTextureType_EMISSIVE;
	params.m_DestFolderPath = "D:\\Assimp Exported Models\\Model 1";
	params.m_DestOBJFileName = "Model.obj";
	params.m_DestTexturesFolderName = "Textures";

	AssimpModelExporter exporter;
	bool result = exporter.Export(params);
	assert(result);
	
	return 0;
}