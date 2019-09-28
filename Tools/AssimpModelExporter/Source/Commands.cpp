#include "Commands.h"
#include "MaterialConfigs.h"
#include "Math/Math.h"
#include "D3DWrapper/GraphicsUtils.h"
#include "DirectXTex/DirectXTex.h"

namespace
{
	void FillImage(DirectX::Image& image, UINT width, UINT height, DXGI_FORMAT format, u8* pPixelBytes);
}

CreateColorTextureCommand::CreateColorTextureCommand(const aiColor3D& color, std::filesystem::path destPath)
	: m_Color(color)
	, m_DestPath(std::move(destPath))
{
}

void CreateColorTextureCommand::Execute() const
{
	DirectX::Image image;

	assert(IsInRange(0.0f, 1.0f, m_Color.r));
	assert(IsInRange(0.0f, 1.0f, m_Color.g));
	assert(IsInRange(0.0f, 1.0f, m_Color.b));
		
	u8 pixelBytes[4] = {u8(255.0f * m_Color.r), u8(255.0f * m_Color.g), u8(255.0f * m_Color.b), 255};
	FillImage(image, 1, 1, DXGI_FORMAT_R8G8B8A8_UNORM, pixelBytes);

	assert(m_DestPath.extension() == ".DDS");
	std::wstring destPathString(m_DestPath.wstring());
	VerifyD3DResult(SaveToDDSFile(image, DirectX::DDS_FLAGS_NONE, destPathString.c_str()));
}

CreateFloatTextureCommand::CreateFloatTextureCommand(f32 value, std::filesystem::path destPath)
	: m_Value(value)
	, m_DestPath(std::move(destPath))
{
}

void CreateFloatTextureCommand::Execute() const
{
}

CopyTextureCommand::CopyTextureCommand(std::filesystem::path sourceTexturePath, std::filesystem::path destTexturePath)
	: m_SourceTexturePath(std::move(sourceTexturePath))
	, m_DestTexturePath(std::move(destTexturePath))
{
}

void CopyTextureCommand::Execute() const
{
}

UpdateMaterialCommand::UpdateMaterialCommand(aiMaterial& material, std::filesystem::path baseColorTexturePath, std::filesystem::path metalnessTexturePath,
	std::filesystem::path roughnessTexturePath, std::filesystem::path emissiveTexturePath)
	: m_Material(material)
	, m_BaseColorTexturePath(std::move(baseColorTexturePath))
	, m_MetalnessTexturePath(std::move(metalnessTexturePath))
	, m_RoughnessTexturePath(std::move(roughnessTexturePath))
	, m_EmissiveTexturePath(std::move(emissiveTexturePath))
{
}

void UpdateMaterialCommand::Execute(const DestMaterialConfig& destMaterialConfig)
{
	m_Material.Clear();
	
	aiString texturePathString(m_BaseColorTexturePath.string());
	m_Material.AddProperty(&texturePathString, AI_MATKEY_TEXTURE(destMaterialConfig.m_BaseColorTextureKey, 0));
	
	texturePathString = m_MetalnessTexturePath.string();
	m_Material.AddProperty(&texturePathString, AI_MATKEY_TEXTURE(destMaterialConfig.m_MetalnessTextureKey, 0));

	texturePathString = m_RoughnessTexturePath.string();
	m_Material.AddProperty(&texturePathString, AI_MATKEY_TEXTURE(destMaterialConfig.m_RoughnessTextureKey, 0));

	texturePathString = m_EmissiveTexturePath.string();
	m_Material.AddProperty(&texturePathString, AI_MATKEY_TEXTURE(destMaterialConfig.m_EmissiveTextureKey, 0));
}

namespace
{
	void FillImage(DirectX::Image& image, UINT width, UINT height, DXGI_FORMAT format, u8* pPixelBytes)
	{
		image.width = width;
		image.height = height;
		image.format = format;
		image.rowPitch = width * GetSizeInBytes(format);
		image.slicePitch = height * image.rowPitch;
		image.pixels = pPixelBytes;
	}
}
