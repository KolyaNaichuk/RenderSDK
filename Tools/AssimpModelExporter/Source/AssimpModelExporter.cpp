#include "AssimpModelExporter.h"
#include "Commands.h"
#include "assimp/Importer.hpp"
#include "assimp/postprocess.h"
#include "assimp/scene.h"

// ToDo
// When loading assimp model SceneLoader should automatically figure out optimal index format

bool AssimpModelExporter::Export(const ExportParams& params)
{
	assert(!params.m_SourceFolderPath.empty());
	assert(!params.m_SourceOBJFileName.empty());
	assert(!params.m_DestFolderPath.empty());
	assert(!params.m_DestOBJFileName.empty());
	assert(!params.m_DestTexturesFolderName.empty());
	
	const u32 importFlags = aiProcess_CalcTangentSpace |
		aiProcess_GenNormals |
		aiProcess_Triangulate |
		aiProcess_SortByPType |
		aiProcess_MakeLeftHanded |
		aiProcess_FlipUVs |
		aiProcess_FlipWindingOrder |
		aiProcess_JoinIdenticalVertices |
		aiProcess_OptimizeMeshes |
		aiProcess_RemoveRedundantMaterials;
	
	std::filesystem::path sourceOBJFilePath(params.m_SourceFolderPath / params.m_SourceOBJFileName);
	std::string sourceOBJFilePathString(sourceOBJFilePath.string());

	Assimp::Importer importer;
	if (importer.ReadFile(sourceOBJFilePathString.c_str(), importFlags) == nullptr)
	{
		OutputDebugStringA(importer.GetErrorString());
		assert(false);
		return false;
	}

	aiScene* pScene = importer.GetOrphanedScene();
		
	SafeDelete(pScene);
	return true;
}

void AssimpModelExporter::GenerateCommands(aiScene& scene, const ExportParams& params)
{
	assert(scene.HasMaterials());

	aiString materialName;
	for (decltype(scene.mNumMaterials) index = 0; index < scene.mNumMaterials; ++index)
	{
		aiMaterial& material = *scene.mMaterials[index];

		aiReturn result = material.Get(AI_MATKEY_NAME, materialName);
		assert(result == aiReturn_SUCCESS);

		/*
		CorrectBaseColorTexture(material, materialName, params);
		CorrectMetalnessTexture(material, materialName, params);
		CorrectRoughnessTexture(material, materialName, params);
		CorrectEmissiveColorTexture(material, materialName, params);
		*/
	}
}

void AssimpModelExporter::ExecuteCommands(const DestMaterialConfig& destMaterialConfig)
{
	for (const CreateColorTextureCommand& command : m_CreateColorTextureCommands)
		command.Execute();	

	for (const CreateFloatTextureCommand& command : m_CreateFloatTextureCommands)
		command.Execute();

	for (const CopyTextureCommand& command : m_CopyTextureCommands)
		command.Execute();
	
	for (UpdateMaterialCommand& command : m_UpdateMaterialCommands)
		command.Execute(destMaterialConfig);	

	m_CreateColorTextureCommands.clear();
	m_CreateFloatTextureCommands.clear();
	m_CopyTextureCommands.clear();
	m_UpdateMaterialCommands.clear();
}

/*
void AssimpModelExporter::CorrectBaseColorTexture(aiMaterial& material, const aiString& materialName, const ExportParams& params)
{	
	const MaterialConfig& materialConfig = params.m_SourceMaterialConfig;

	aiString textureName;
	aiReturn result = material.GetTexture(materialConfig.m_BaseColorTextureKey, 0, &textureName);
	
	if (result == aiReturn_SUCCESS)
	{
		std::filesystem::path sourceTextureRelativePath(textureName.C_Str());
		std::filesystem::path sourceTextureFullPath
	}
	else if (result == aiReturn_FAILURE)
	{
		assert(!materialConfig.m_BaseColorKey.empty());

		aiColor3D color;
		result = material.Get(materialConfig.m_BaseColorKey.c_str(), 0, 0, color);
		assert(result == aiReturn_SUCCESS);

		assert((0.0f <= color.r) && (color.r <= 1.0f));
		assert((0.0f <= color.g) && (color.g <= 1.0f));
		assert((0.0f <= color.b) && (color.b <= 1.0f));
		u8 pixelBytes[4] = {u8(255.0f * color.r), u8(255.0f * color.g), u8(255.0f * color.b), 255};
		
		DirectX::Image image;
		GenerateImage(image, 1, 1, DXGI_FORMAT_R8G8B8A8_UNORM, pixelBytes);

		std::filesystem::path textureRelativePath(params.m_ExportTexturesDirectory);
		textureRelativePath /= materialName.C_Str();
		textureRelativePath += " base.dds";
		
		std::filesystem::path textureFullPath = params.m_ExportModelPath / textureRelativePath;
		std::wstring textureFullPathString(textureFullPath.wstring());		
		VerifyD3DResult(SaveToDDSFile(image, DirectX::DDS_FLAGS_NONE, textureFullPathString.c_str()));

		aiString textureRelativePathString(textureRelativePath.string());
		material.AddProperty(&textureRelativePathString, materialConfig.m_BaseColorTextureKey, 0, 0);
	}
	else
	{
		assert(false);
	}
}

void AssimpModelExporter::CorrectMetalnessTexture(aiMaterial& material, const aiString& materialName, const ExportParams& params)
{
	result = pAssimpMaterial->GetTexture(pParams->m_MetalnessTextureKey, 0, &assimpString);
	if (result == aiReturn_SUCCESS)
	{
		pMaterial->m_MetalnessTexturePath = materialDirectoryPath / Path(AnsiToWideString(assimpString.C_Str()));
	}
	else
	{
		assert(pParams->m_pMetalnessKey != nullptr);

		result = pAssimpMaterial->Get(pParams->m_pMetalnessKey, 0, 0, pMaterial->m_Metalness);
		assert(result == aiReturn_SUCCESS);
	}
}

void AssimpModelExporter::CorrectRoughnessTexture(aiMaterial& material, const aiString& materialName, const ExportParams& params)
{
	result = pAssimpMaterial->GetTexture(pParams->m_RoughnessTextureKey, 0, &assimpString);
	if (result == aiReturn_SUCCESS)
	{
		pMaterial->m_RoughnessTexturePath = materialDirectoryPath / Path(AnsiToWideString(assimpString.C_Str()));
	}
	else
	{
		assert(pParams->m_pRoughnessKey != nullptr);

		result = pAssimpMaterial->Get(pParams->m_pRoughnessKey, 0, 0, pMaterial->m_Roughness);
		assert(result == aiReturn_SUCCESS);
	}
}

void AssimpModelExporter::CorrectEmissiveColorTexture(aiMaterial& material, const aiString& materialName, const ExportParams& params)
{
	result = pAssimpMaterial->GetTexture(pParams->m_EmissiveTextureKey, 0, &assimpString);
	if (result == aiReturn_SUCCESS)
	{
		pMaterial->m_EmissiveTexturePath = materialDirectoryPath / Path(AnsiToWideString(assimpString.C_Str()));
	}
	else
	{
		assert(pParams->m_pEmissiveKey != nullptr);

		result = pAssimpMaterial->Get(pParams->m_pEmissiveKey, 0, 0, assimpColor);
		assert(result == aiReturn_SUCCESS);

		pMaterial->m_EmissiveColor = ToVector3f(assimpColor);
	}
}
*/
