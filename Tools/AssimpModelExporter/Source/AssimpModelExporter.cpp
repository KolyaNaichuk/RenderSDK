#include "AssimpModelExporter.h"
#include "Commands.h"
#include "assimp/Importer.hpp"
#include "assimp/postprocess.h"
#include "assimp/scene.h"

// ToDo
// When loading assimp model SceneLoader should automatically figure out optimal index format
// Remove unnecessary import flags from SceneLoader
// Add support for opaque textures

namespace
{
	std::filesystem::path ExtractFileExtension(const aiString& filePath);
	std::filesystem::path GenerateTextureFileName(std::filesystem::path textureNamePrefix, const std::filesystem::path& suffix, const std::filesystem::path& extension);
}

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

void AssimpModelExporter::GenerateTextureCommands(aiScene& scene, const ExportParams& params)
{
	assert(scene.HasMaterials());

	assert(m_CreateColorTextureCommands.empty());
	assert(m_CreateFloatTextureCommands.empty());
	assert(m_CopyTextureCommands.empty());
	assert(m_UpdateMaterialCommands.empty());

	const SourceMaterialConfig& sourceMaterialConfig = params.m_SourceMaterialConfig;

	const std::filesystem::path newTextureExtension(".DDS");
	const std::filesystem::path baseColorTextureNameSuffix(" base");
	const std::filesystem::path metalnessTextureNameSuffix(" metalness");
	const std::filesystem::path roughnessTextureNameSuffix(" roughness");
	const std::filesystem::path emissiveColorTextureNameSuffix(" emissive");
	
	aiString materialName;
	aiString texturePath;
	aiColor3D textureColor;

	for (decltype(scene.mNumMaterials) index = 0; index < scene.mNumMaterials; ++index)
	{		
		aiMaterial& material = *scene.mMaterials[index];

		aiReturn result = material.Get(AI_MATKEY_NAME, materialName);
		assert(result == aiReturn_SUCCESS);
		assert(materialName.length > 0);

		const std::filesystem::path textureNamePrefix(materialName.C_Str());

		// Base color texture
		
		std::filesystem::path baseColorTextureRelativePath;
		result = material.Get(AI_MATKEY_TEXTURE(sourceMaterialConfig.m_BaseColorTextureKey, 0), texturePath);
		if (result == aiReturn_SUCCESS)
		{
			std::filesystem::path destTextureName = GenerateTextureFileName(textureNamePrefix, baseColorTextureNameSuffix, ExtractFileExtension(texturePath));
			baseColorTextureRelativePath = params.m_DestTexturesFolderName / destTextureName;

			std::filesystem::path sourceTexturePath(params.m_SourceFolderPath / texturePath.C_Str());
			std::filesystem::path destTexturePath(params.m_DestFolderPath / baseColorTextureRelativePath);

			m_CopyTextureCommands.emplace_back(std::move(sourceTexturePath), std::move(destTexturePath));
		}
		else if (result == aiReturn_FAILURE)
		{
			assert(!sourceMaterialConfig.m_BaseColorKey.empty());
			result = material.Get(sourceMaterialConfig.m_BaseColorKey.c_str(), 0, 0, textureColor);
			assert(result == aiReturn_SUCCESS);

			std::filesystem::path destTextureName = GenerateTextureFileName(textureNamePrefix, baseColorTextureNameSuffix, newTextureExtension);
			baseColorTextureRelativePath = params.m_DestTexturesFolderName / destTextureName;

			std::filesystem::path destTexturePath(params.m_DestFolderPath / baseColorTextureRelativePath);
			m_CreateColorTextureCommands.emplace_back(textureColor, std::move(destTexturePath));
		}
		else
		{
			assert(false);
		}

		// Metalness texture
		std::filesystem::path metalnessTextureRelativePath;
		
		// Roughness texture
		std::filesystem::path roughnessTextureRelativePath;
		
		// Emissive color texture
		std::filesystem::path emissiveColorTextureRelativePath;
		
		// Common

		m_UpdateMaterialCommands.emplace_back(material, std::move(baseColorTextureRelativePath), std::move(metalnessTextureRelativePath),
			std::move(roughnessTextureRelativePath), std::move(emissiveColorTextureRelativePath));
	}
}

void AssimpModelExporter::ProcessTextureCommands(const DestMaterialConfig& destMaterialConfig)
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

const char* GetColorKey(const char* pKey, unsigned int /*type*/, unsigned int /*index*/)
{
	return pKey;
}

namespace
{
	std::filesystem::path ExtractFileExtension(const aiString& filePath)
	{
		return std::filesystem::path(filePath.C_Str()).extension();
	}

	std::filesystem::path GenerateTextureFileName(std::filesystem::path textureNamePrefix, const std::filesystem::path& suffix, const std::filesystem::path& extension)
	{
		assert(!textureNamePrefix.empty());
		assert(!suffix.empty());
		assert(!extension.empty());

		return ((textureNamePrefix += suffix) += extension);
	}
}

/*
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
