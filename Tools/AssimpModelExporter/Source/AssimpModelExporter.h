#pragma once

#include "Commands.h"
#include "MaterialConfigs.h"

struct ExportParams
{
	SourceMaterialConfig m_SourceMaterialConfig;
	std::filesystem::path m_SourceFolderPath;
	std::string m_SourceOBJFileName;

	DestMaterialConfig m_DestMaterialConfig;
	std::filesystem::path m_DestFolderPath;
	std::string m_DestOBJFileName;
	std::string m_DestTexturesFolderName;
};

struct aiScene;

class AssimpModelExporter
{
public:
	bool Export(const ExportParams& params);

private:
	void GenerateCommands(aiScene& scene, const ExportParams& params);
	void GenerateCommandsForBaseColorTexture(aiMaterial& material, const ExportParams& params);
	void GenerateCommandsForMetalnessTexture(aiMaterial& material, const ExportParams& params);
	void GenerateCommandsForRoughnessTexture(aiMaterial& material, const ExportParams& params);
	void GenerateCommandsForEmissiveColorTexture(aiMaterial& material, const ExportParams& params);

	void ExecuteCommands(const DestMaterialConfig& destMaterialConfig);
		
private:
	std::vector<CreateColorTextureCommand> m_CreateColorTextureCommands;
	std::vector<CreateFloatTextureCommand> m_CreateFloatTextureCommands;
	std::vector<CopyTextureCommand> m_CopyTextureCommands;
	std::vector<UpdateMaterialCommand> m_UpdateMaterialCommands;
};