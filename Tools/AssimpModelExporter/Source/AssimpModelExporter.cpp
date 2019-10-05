#include "AssimpModelExporter.h"
#include "Commands.h"
#include "Common/StringUtilities.h"
#include "assimp/Importer.hpp"
#include "assimp/Exporter.hpp"
#include "assimp/postprocess.h"
#include "assimp/scene.h"

// ToDo
// When loading assimp model SceneLoader should automatically figure out optimal index format
// Remove unnecessary import flags from SceneLoader
// Add support for opaque/transparent textures

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

	std::cout << "Loading " << sourceOBJFilePathString << "\n";
	if (importer.ReadFile(sourceOBJFilePathString.c_str(), importFlags) == nullptr)
	{
		std::cout << "Could not load file. Error: " << importer.GetErrorString() << "\n";
		return false;
	}
	std::cout << "Loading completed successfully\n";

	bool destFolderCreated = std::filesystem::create_directories(params.m_DestFolderPath);
	assert(destFolderCreated);

	bool destTexturesFolderCreated = std::filesystem::create_directories(params.m_DestFolderPath / params.m_DestTexturesFolderName);
	assert(destTexturesFolderCreated);

	aiScene* pScene = importer.GetOrphanedScene();
	std::cout << "Generating texture commands\n";
	GenerateTextureCommands(*pScene, params);

	std::cout << "Processing texture commands\n";
	ProcessTextureCommands(params.m_DestMaterialConfig);

	Assimp::Exporter exporter;
	assert(exporter.GetExportFormatCount() > 0);

	const aiExportFormatDesc* OBJFileFormatDesc = nullptr;
	for (size_t index = 0; (index < exporter.GetExportFormatCount()) && (OBJFileFormatDesc == nullptr); ++index)
	{
		const aiExportFormatDesc* formatDesc = exporter.GetExportFormatDescription(index);
		if (AreEqual(formatDesc->fileExtension, "OBJ") || AreEqual(formatDesc->fileExtension, "obj"))
			OBJFileFormatDesc = formatDesc;
	}
	assert(OBJFileFormatDesc != nullptr);
	
	std::filesystem::path destOBJFilePath(params.m_DestFolderPath / params.m_DestOBJFileName);
	std::string destOBJFilePathString(destOBJFilePath.string());
		
	std::cout << "Exporting " << destOBJFilePathString << "\n";
	aiReturn exportResult = exporter.Export(pScene, OBJFileFormatDesc->id, destOBJFilePathString.c_str());
	SafeDelete(pScene);

	if (exportResult == aiReturn_SUCCESS)
	{
		std::cout << "Export completed successfully\n";
		return true;
	}
	else
	{
		std::cout << "Export failed. Error: " << exporter.GetErrorString() << "\n";
		return false;
	}
}

void AssimpModelExporter::GenerateTextureCommands(aiScene& scene, const ExportParams& params)
{
	assert(scene.HasMaterials());

	assert(m_CreateColorTextureCommands.empty());
	assert(m_CreateFloatTextureCommands.empty());
	assert(m_CopyTextureCommands.empty());
	assert(m_UpdateMaterialCommands.empty());

	m_CreateColorTextureCommands.reserve(2 * scene.mNumMaterials);
	m_CreateFloatTextureCommands.reserve(2 * scene.mNumMaterials);
	m_CopyTextureCommands.reserve(4 * scene.mNumMaterials);
	m_UpdateMaterialCommands.reserve(scene.mNumMaterials);
	
	const std::filesystem::path newTextureExtension(".DDS");
	const std::filesystem::path roughnessTextureNameSuffix(" roughness");
	const std::filesystem::path emissiveColorTextureNameSuffix(" emissive");
	
	aiString materialName;	
	for (decltype(scene.mNumMaterials) index = 0; index < scene.mNumMaterials; ++index)
	{		
		aiMaterial& material = *scene.mMaterials[index];

		aiReturn result = material.Get(AI_MATKEY_NAME, materialName);
		assert(result == aiReturn_SUCCESS);
		assert(materialName.length > 0);

		const std::filesystem::path textureNamePrefix(materialName.C_Str());
		std::filesystem::path textureRelativePaths[NumMaterialProps];
				
		GenerateBaseColorTextureCommands(material, textureRelativePaths[BaseColorProp], params, textureNamePrefix, newTextureExtension);
		//GenerateMetalnessTextureCommands(material, textureRelativePaths[MetalnessProp], params, textureNamePrefix, newTextureExtension);

		m_UpdateMaterialCommands.emplace_back(material, textureRelativePaths);
	}
}

void AssimpModelExporter::GenerateBaseColorTextureCommands(aiMaterial& material, std::filesystem::path& textureRelativePath,
	const ExportParams& params, const std::filesystem::path& textureNamePrefix, const std::filesystem::path& newTextureExtension)
{
	const std::filesystem::path textureNameSuffix(" base");

	aiString texturePath;
	aiReturn result = material.Get(AI_MATKEY_TEXTURE(params.m_SourceMaterialConfig.m_TextureTypes[BaseColorProp], 0), texturePath);
	if (result == aiReturn_SUCCESS)
	{
		std::filesystem::path destTextureName = GenerateTextureFileName(textureNamePrefix, textureNameSuffix, ExtractFileExtension(texturePath));
		textureRelativePath = params.m_DestTexturesFolderName / destTextureName;

		std::filesystem::path sourceTexturePath(params.m_SourceFolderPath / texturePath.C_Str());
		std::filesystem::path destTexturePath(params.m_DestFolderPath / textureRelativePath);

		m_CopyTextureCommands.emplace_back(std::move(sourceTexturePath), std::move(destTexturePath));
	}
	else if (result == aiReturn_FAILURE)
	{
		assert(!params.m_SourceMaterialConfig.m_Keys[BaseColorProp].empty());
		
		aiColor3D textureColor;
		result = material.Get(params.m_SourceMaterialConfig.m_Keys[BaseColorProp].c_str(), 0, 0, textureColor);
		assert(result == aiReturn_SUCCESS);

		std::filesystem::path destTextureName = GenerateTextureFileName(textureNamePrefix, textureNameSuffix, newTextureExtension);
		textureRelativePath = params.m_DestTexturesFolderName / destTextureName;

		std::filesystem::path destTexturePath(params.m_DestFolderPath / textureRelativePath);
		m_CreateColorTextureCommands.emplace_back(textureColor, std::move(destTexturePath));
	}
	else
	{
		assert(false);
	}
}

void AssimpModelExporter::GenerateMetalnessTextureCommands(aiMaterial& material, std::filesystem::path& textureRelativePath,
	const ExportParams& params, const std::filesystem::path& textureNamePrefix, const std::filesystem::path& newTextureExtension)
{
	const std::filesystem::path textureNameSuffix(" metalness");

	aiString texturePath;
	aiReturn result = material.Get(AI_MATKEY_TEXTURE(params.m_SourceMaterialConfig.m_TextureTypes[MetalnessProp], 0), texturePath);
	if (result == aiReturn_SUCCESS)
	{
		std::filesystem::path destTextureName = GenerateTextureFileName(textureNamePrefix, textureNameSuffix, ExtractFileExtension(texturePath));
		textureRelativePath = params.m_DestTexturesFolderName / destTextureName;

		std::filesystem::path sourceTexturePath(params.m_SourceFolderPath / texturePath.C_Str());
		std::filesystem::path destTexturePath(params.m_DestFolderPath / textureRelativePath);

		m_CopyTextureCommands.emplace_back(std::move(sourceTexturePath), std::move(destTexturePath));
	}
	else if (result == aiReturn_FAILURE)
	{
		assert(!params.m_SourceMaterialConfig.m_Keys[MetalnessProp].empty());

		f32 metalness = 0.0f;
		result = material.Get(params.m_SourceMaterialConfig.m_Keys[MetalnessProp].c_str(), 0, 0, metalness);
		assert(result == aiReturn_SUCCESS);

		std::filesystem::path destTextureName = GenerateTextureFileName(textureNamePrefix, textureNameSuffix, newTextureExtension);
		textureRelativePath = params.m_DestTexturesFolderName / destTextureName;

		std::filesystem::path destTexturePath(params.m_DestFolderPath / textureRelativePath);
		m_CreateFloatTextureCommands.emplace_back(metalness, std::move(destTexturePath));
	}
	else
	{
		assert(false);
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
