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
	void GenerateTextureCommands(aiScene& scene, const ExportParams& params);
	void ProcessTextureCommands(const DestMaterialConfig& destMaterialConfig);
				
private:
	std::vector<CreateColorTextureCommand> m_CreateColorTextureCommands;
	std::vector<CreateFloatTextureCommand> m_CreateFloatTextureCommands;
	std::vector<CopyTextureCommand> m_CopyTextureCommands;
	std::vector<UpdateMaterialCommand> m_UpdateMaterialCommands;
};

const char* GetColorKey(const char* pKey, unsigned int type, unsigned int index);
