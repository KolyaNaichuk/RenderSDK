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
	assert(!m_DestPath.empty());
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
	assert(!m_DestPath.empty());
}

void CreateFloatTextureCommand::Execute() const
{
	assert(false);
}

CopyTextureCommand::CopyTextureCommand(std::filesystem::path sourceTexturePath, std::filesystem::path destTexturePath)
	: m_SourceTexturePath(std::move(sourceTexturePath))
	, m_DestTexturePath(std::move(destTexturePath))
{
	assert(!m_SourceTexturePath.empty());
	assert(!m_DestTexturePath.empty());
}

void CopyTextureCommand::Execute() const
{
	bool result = std::filesystem::copy_file(m_SourceTexturePath, m_DestTexturePath, std::filesystem::copy_options::overwrite_existing);
	assert(result);
}

UpdateMaterialCommand::UpdateMaterialCommand(aiMaterial& material, std::filesystem::path texturePaths[NumMaterialProps])
	: m_Material(material)
{
	for (u8 index = 0; index < NumMaterialProps; ++index)
	{
		assert(!texturePaths[index].empty());
		m_TexturePaths[index] = std::move(texturePaths[index]);
	}
}

void UpdateMaterialCommand::Execute(const DestMaterialConfig& destMaterialConfig)
{
	aiString string;
	aiReturn result = m_Material.Get(AI_MATKEY_NAME, string);
	assert(result == aiReturn_SUCCESS);
	assert(string.length > 0);
	
	m_Material.Clear();
	
	result = m_Material.AddProperty(&string, AI_MATKEY_NAME);
	assert(result == aiReturn_SUCCESS);

	for (u8 index = 0; index < NumMaterialProps; ++index)
	{
		assert(destMaterialConfig.m_TextureTypes[index] != aiTextureType_UNKNOWN);
		
		string = m_TexturePaths[index].string();
		result = m_Material.AddProperty(&string, AI_MATKEY_TEXTURE(destMaterialConfig.m_TextureTypes[index], 0));
		assert(result == aiReturn_SUCCESS);
	}
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
