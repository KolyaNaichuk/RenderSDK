#include "AssimpModelExporter.h"

int main()
{
	ExportParams params;
	
	params.m_SourceMaterialConfig.m_TextureTypes[BaseColorProp] = aiTextureType_DIFFUSE;
	params.m_SourceMaterialConfig.m_TextureTypes[MetalnessProp] = aiTextureType_REFLECTION;
	params.m_SourceMaterialConfig.m_TextureTypes[RoughnessProp] = aiTextureType_SHININESS;
	params.m_SourceMaterialConfig.m_TextureTypes[EmissiveColorProp] = aiTextureType_EMISSIVE;
	params.m_SourceMaterialConfig.m_Keys[BaseColorProp] = GetColorKey(AI_MATKEY_COLOR_DIFFUSE);
	params.m_SourceMaterialConfig.m_Keys[MetalnessProp] = GetColorKey(AI_MATKEY_REFLECTIVITY);
	params.m_SourceMaterialConfig.m_Keys[RoughnessProp] = GetColorKey(AI_MATKEY_SHININESS);
	params.m_SourceMaterialConfig.m_Keys[EmissiveColorProp] = GetColorKey(AI_MATKEY_COLOR_EMISSIVE);
	params.m_SourceFolderPath = "D:\\Assimp Models\\Model 1";
	params.m_SourceOBJFileName = "Model.obj";
	
	params.m_DestMaterialConfig.m_TextureTypes[BaseColorProp] = aiTextureType_DIFFUSE;
	params.m_DestMaterialConfig.m_TextureTypes[MetalnessProp] = aiTextureType_REFLECTION;
	params.m_DestMaterialConfig.m_TextureTypes[RoughnessProp] = aiTextureType_SHININESS;
	params.m_DestMaterialConfig.m_TextureTypes[EmissiveColorProp] = aiTextureType_EMISSIVE;
	params.m_DestFolderPath = "D:\\Assimp Exported Models\\Model 1";
	params.m_DestOBJFileName = "Model.obj";
	params.m_DestTexturesFolderName = "Textures";

	AssimpModelExporter exporter;
	bool result = exporter.Export(params);
	assert(result);
	
	return 0;
}