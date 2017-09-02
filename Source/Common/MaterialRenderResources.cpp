#include "Common/MaterialRenderResources.h"
#include "Common/Material.h"
#include "Common/FileUtilities.h"
#include "D3DWrapper/CommandList.h"
#include "D3DWrapper/GraphicsDevice.h"
#include "D3DWrapper/GraphicsResource.h"
#include "D3DWrapper/RenderEnv.h"
#include "External/DirectXTex/DirectXTex.h"
#include "Math/Vector4.h"

namespace
{
	ColorTexture* LoadTexture(RenderEnv* pRenderEnv, const wchar_t* pFilePath, bool forceSRGB);
}

MaterialRenderResources::MaterialRenderResources(RenderEnv* pRenderEnv, u32 numMaterials, Material** ppMaterials)
{
	const u8 numTexturesPerMaterial = 3;
	const u8 texturePlaceholderSize = 4;
	const u32 maxNumTextures = numMaterials * numTexturesPerMaterial;
	
	std::vector<std::tuple<ColorTexture*, Vector4f>> texturePlaceholders;
	texturePlaceholders.reserve(maxNumTextures);

	m_Textures.reserve(maxNumTextures);
	for (u32 index = 0; index < numMaterials; ++index)
	{
		const Material* pMaterial = ppMaterials[index];

		std::wstring diffuseMapDebugName = pMaterial->m_Name + L" Diffuse Map";
		std::wstring specularMapDebugName = pMaterial->m_Name + L" Specular Map";
		std::wstring shininessMapDebugName = pMaterial->m_Name + L" Shininess Map";

		if (pMaterial->m_DiffuseMapName.empty())
		{
			Vector4f textureColor(pMaterial->m_DiffuseColor.m_X, pMaterial->m_DiffuseColor.m_Y, pMaterial->m_DiffuseColor.m_Z, 1.0f);
												
			assert(false && "Fix format");
			ColorTexture2DDesc textureDesc(DXGI_FORMAT_R16G16B16A16_FLOAT, texturePlaceholderSize, texturePlaceholderSize, true, true, false);
			ColorTexture* pTexture = new ColorTexture(pRenderEnv, pRenderEnv->m_pDefaultHeapProps, &textureDesc,
				D3D12_RESOURCE_STATE_RENDER_TARGET, &textureColor.m_X, diffuseMapDebugName.c_str());

			m_Textures.emplace_back(pTexture);
			texturePlaceholders.emplace_back(pTexture, textureColor);
		}
		else
		{
		}
		
		if (pMaterial->m_SpecularMapName.empty())
		{
			Vector4f textureColor(pMaterial->m_SpecularColor.m_X, pMaterial->m_SpecularColor.m_Y, pMaterial->m_SpecularColor.m_Z, 1.0f);
			
			
			assert(false && "Fix format");
			ColorTexture2DDesc textureDesc(DXGI_FORMAT_R16G16B16A16_FLOAT, texturePlaceholderSize, texturePlaceholderSize, true, true, false);
			ColorTexture* pTexture = new ColorTexture(pRenderEnv, pRenderEnv->m_pDefaultHeapProps, &textureDesc,
				D3D12_RESOURCE_STATE_RENDER_TARGET, &textureColor.m_X, specularMapDebugName.c_str());

			m_Textures.emplace_back(pTexture);
			texturePlaceholders.emplace_back(pTexture, textureColor);
		}
		else
		{
			assert(false && "Missing impl");
		}

		if (pMaterial->m_ShininessMapName.empty())
		{
			Vector4f textureColor(pMaterial->m_Shininess, pMaterial->m_Shininess, pMaterial->m_Shininess, 1.0f);
						
			assert(false && "Fix format");
			ColorTexture2DDesc textureDesc(DXGI_FORMAT_R16_FLOAT, texturePlaceholderSize, texturePlaceholderSize, true, true, false);
			ColorTexture* pTexture = new ColorTexture(pRenderEnv, pRenderEnv->m_pDefaultHeapProps, &textureDesc,
				D3D12_RESOURCE_STATE_RENDER_TARGET, &textureColor.m_X, shininessMapDebugName.c_str());

			m_Textures.emplace_back(pTexture);
			texturePlaceholders.emplace_back(pTexture, textureColor);
		}
		else
		{
			assert(false && "Missing impl");
		}
	}

	std::vector<ResourceBarrier> placeholderBarriers;
	placeholderBarriers.reserve(texturePlaceholders.size());
		
	CommandList* pCommandList = pRenderEnv->m_pCommandListPool->Create(L"pMaterialCommandList");
	pCommandList->Begin();
	
	for (decltype(texturePlaceholders.size()) index = 0; index < texturePlaceholders.size(); ++index)
	{
		ColorTexture* pTexture = std::get<0>(texturePlaceholders[index]);
		const Vector4f& textureColor = std::get<1>(texturePlaceholders[index]);

		pCommandList->ClearRenderTargetView(pTexture->GetRTVHandle(), &textureColor.m_X);
		placeholderBarriers.emplace_back(pTexture, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	}
	
	pCommandList->ResourceBarrier(placeholderBarriers.size(), placeholderBarriers.data());
	pCommandList->End();
	
	++pRenderEnv->m_LastSubmissionFenceValue;
	pRenderEnv->m_pCommandQueue->ExecuteCommandLists(1, &pCommandList, pRenderEnv->m_pFence, pRenderEnv->m_LastSubmissionFenceValue);
	pRenderEnv->m_pFence->WaitForSignalOnCPU(pRenderEnv->m_LastSubmissionFenceValue);
}

MaterialRenderResources::~MaterialRenderResources()
{
	for (decltype(m_Textures.size()) index = 0; index < m_Textures.size(); ++index)
		SafeDelete(m_Textures[index]);
}

namespace
{
	ColorTexture* LoadTexture(RenderEnv* pRenderEnv, const wchar_t* pFilePath, bool forceSRGB)
	{
		DirectX::ScratchImage image;

		const std::wstring fileName = ExtractFileNameWithExtension(pFilePath);
		const std::wstring fileExtension = ExtractFileExtension(pFilePath);
		
		if ((fileExtension == L"DDS") || (fileExtension == L"dds"))
		{
			VerifyD3DResult(DirectX::LoadFromDDSFile(pFilePath, DirectX::DDS_FLAGS_NONE, nullptr, image));
		}
		else if ((fileExtension == L"TGA") || (fileExtension == L"tga"))
		{
			DirectX::ScratchImage tempImage;
			VerifyD3DResult(DirectX::LoadFromTGAFile(pFilePath, nullptr, tempImage));
			VerifyD3DResult(DirectX::GenerateMipMaps(*tempImage.GetImage(0, 0, 0), DirectX::TEX_FILTER_DEFAULT, 0, image, false));
		}
		else
		{
			DirectX::ScratchImage tempImage;
			VerifyD3DResult(DirectX::LoadFromWICFile(pFilePath, DirectX::WIC_FLAGS_NONE, nullptr, tempImage));
			VerifyD3DResult(DirectX::GenerateMipMaps(*tempImage.GetImage(0, 0, 0), DirectX::TEX_FILTER_DEFAULT, 0, image, false));
		}
				
		const DirectX::TexMetadata& metaData = image.GetMetadata();
		const DXGI_FORMAT format = forceSRGB ? DirectX::MakeSRGB(metaData.format) : metaData.format;

		D3D12_RESOURCE_DIMENSION dimension = D3D12_RESOURCE_DIMENSION_UNKNOWN;
		if (metaData.dimension == DirectX::TEX_DIMENSION_TEXTURE1D)
			dimension = D3D12_RESOURCE_DIMENSION_TEXTURE1D;
		else if (metaData.dimension == DirectX::TEX_DIMENSION_TEXTURE2D)
			dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		else if (metaData.dimension == DirectX::TEX_DIMENSION_TEXTURE3D)
			dimension = D3D12_RESOURCE_DIMENSION_TEXTURE3D;

		assert(dimension != D3D12_RESOURCE_DIMENSION_UNKNOWN);
		assert(!metaData.IsCubemap());

		const UINT16 depthOrArraySize = (dimension == D3D12_RESOURCE_DIMENSION_TEXTURE3D) ? UINT16(metaData.depth) : UINT16(metaData.arraySize);

		ColorTextureDesc textureDesc(dimension, format, UINT64(metaData.width), UINT(metaData.height), false, true, false, depthOrArraySize);
		ColorTexture* pTexture = new ColorTexture(pRenderEnv, pRenderEnv->m_pDefaultHeapProps, &textureDesc,
			D3D12_RESOURCE_STATE_COPY_DEST, nullptr, fileName.c_str());
		
		static const UINT maxNumSubresources = 64;
		static D3D12_PLACED_SUBRESOURCE_FOOTPRINT subresourceFootprints[maxNumSubresources];
		static UINT numRows[maxNumSubresources];
		static UINT64 rowSizeInBytes[maxNumSubresources];
		
		const UINT numSubresources = UINT(metaData.mipLevels * metaData.arraySize);
		assert(numSubresources <= maxNumSubresources);

		Buffer* pUploadBuffer = nullptr;
		assert(pUploadBuffer != nullptr);
		UINT* pUploadStartMem = nullptr;
		assert(pUploadStartMem != nullptr);		
		UINT64 uploadOffsetInBytes = 0;
		assert(false && "baseOffsetInBytes");
		
		UINT64 textureSizeInBytes = 0;
		pRenderEnv->m_pDevice->GetCopyableFootprints(&textureDesc, 0, numSubresources, uploadOffsetInBytes, subresourceFootprints, numRows, rowSizeInBytes, &textureSizeInBytes);
		
		for (decltype(metaData.arraySize) arrayIndex = 0; arrayIndex < metaData.arraySize; ++arrayIndex)
		{
			for (decltype(metaData.mipLevels) mipIndex = 0; mipIndex < metaData.mipLevels; ++mipIndex)
			{
				const UINT subresourceIndex = UINT(mipIndex + (arrayIndex * metaData.mipLevels));

				const D3D12_PLACED_SUBRESOURCE_FOOTPRINT& subresourceFootprint = subresourceFootprints[subresourceIndex];
				const UINT subresourceNumRows = numRows[subresourceIndex];
				const UINT subresourceRowPitch = subresourceFootprint.Footprint.RowPitch;
				const UINT subresourceDepth = subresourceFootprint.Footprint.Depth;
				UINT* pDestSubresourceMem = pUploadStartMem + subresourceFootprint.Offset;

				for (UINT slice = 0; slice < subresourceDepth; ++slice)
				{
					const DirectX::Image* pSubimage = image.GetImage(mipIndex, arrayIndex, slice);
					assert(pSubimage != nullptr);
					const UINT8* pSrcSubresourceMem = pSubimage->pixels;
					
					const UINT bytesToCopy = Min(subresourceRowPitch, pSubimage->rowPitch);
					for (UINT row = 0; row < subresourceNumRows; ++row)
					{
						std::memcpy(pDestSubresourceMem, pSrcSubresourceMem, bytesToCopy);

						pDestSubresourceMem += subresourceRowPitch;
						pSrcSubresourceMem += pSubimage->rowPitch;
					}
				}
			}
		}

		CommandList* pCommandList = nullptr;
		assert(pCommandList != nullptr);
		for (UINT subresourceIndex = 0; subresourceIndex < numSubresources; ++subresourceIndex)
		{
			TextureCopyLocation destLocation(pTexture, subresourceIndex);
			TextureCopyLocation sourceLocation(pUploadBuffer, subresourceFootprints[subresourceIndex]);

			pCommandList->CopyTextureRegion(&destLocation, 0, 0, 0, &sourceLocation, nullptr);
		}

		return pTexture;
	}
}
