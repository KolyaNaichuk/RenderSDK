#pragma once

#include "Common/Common.h"
#include "assimp/types.h"

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
	UpdateMaterialCommand(aiMaterial& material, std::filesystem::path baseColorTexturePath, std::filesystem::path metalnessTexturePath,
		std::filesystem::path roughnessTexturePath, std::filesystem::path emissiveTexturePath);

	void Execute(const DestMaterialConfig& destMaterialConfig);

private:
	aiMaterial& m_Material;
	std::filesystem::path m_BaseColorTexturePath;
	std::filesystem::path m_MetalnessTexturePath;
	std::filesystem::path m_RoughnessTexturePath;
	std::filesystem::path m_EmissiveTexturePath;
};