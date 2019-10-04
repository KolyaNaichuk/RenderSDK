#pragma once

#include "MaterialConfigs.h"
#include "Common/Common.h"

struct DestMaterialConfig;
struct aiMaterial;

class CreateColorTextureCommand
{
public:
	CreateColorTextureCommand(const aiColor3D& color, std::filesystem::path destPath);
	void Execute() const;

private:
	aiColor3D m_Color;
	std::filesystem::path m_DestPath;
};

class CreateFloatTextureCommand
{
public:
	CreateFloatTextureCommand(f32 value, std::filesystem::path destPath);
	void Execute() const;

private:
	f32 m_Value;
	std::filesystem::path m_DestPath;
};

class CopyTextureCommand
{
public:
	CopyTextureCommand(std::filesystem::path sourceTexturePath, std::filesystem::path destTexturePath);
	void Execute() const;

private:
	std::filesystem::path m_SourceTexturePath;
	std::filesystem::path m_DestTexturePath;
};

class UpdateMaterialCommand
{
public:
	UpdateMaterialCommand(aiMaterial& material, std::filesystem::path texturePaths[NumMaterialProps]);
	void Execute(const DestMaterialConfig& destMaterialConfig);

private:
	aiMaterial& m_Material;
	std::filesystem::path m_TexturePaths[NumMaterialProps];
};