#include "Common/MaterialRenderResources.h"
#include "Common/Material.h"
#include "Common/FileUtilities.h"
#include "D3DWrapper/CommandList.h"
#include "D3DWrapper/GraphicsDevice.h"
#include "D3DWrapper/GraphicsResource.h"
#include "D3DWrapper/GraphicsUtils.h"
#include "D3DWrapper/RenderEnv.h"
#include "External/DirectXTex/DirectXTex.h"
#include "Math/Vector4.h"

namespace
{
	void GenerateImageData(UINT width, UINT height, DXGI_FORMAT format, std::uint8_t* pPixelBytes, DirectX::ScratchImage& image);
	void LoadImageDataFromFile(const std::wstring& filePath, DirectX::ScratchImage& image);
		
	void SetupImageDataForUpload(RenderEnv* pRenderEnv,
		const DirectX::ScratchImage& image,
		const std::wstring& debugImageName,
		bool forceSRGB,
		CommandList* pUploadCommandList,
		std::vector<Buffer*>& uploadHeapBuffers,
		std::vector<ColorTexture*>& defaultHeapTextures,
		std::vector<ResourceBarrier>& pendingBarriers);
}

MaterialRenderResources::MaterialRenderResources(RenderEnv* pRenderEnv, u32 numMaterials, Material** ppMaterials)
{
	static const bool forceSRGB = true;
	static const u8 numTexturesPerMaterial = 3;
	
	const u32 maxNumTextures = numMaterials * numTexturesPerMaterial;
	m_Textures.reserve(maxNumTextures);

	std::vector<Buffer*> uploadBuffers;
	uploadBuffers.reserve(maxNumTextures);

	std::vector<ResourceBarrier> pendingBarriers;	
	pendingBarriers.reserve(maxNumTextures);

	CommandList* pUploadCommandList = pRenderEnv->m_pCommandListPool->Create(L"UploadCommandList");
	pUploadCommandList->Begin();
	
	for (u32 index = 0; index < numMaterials; ++index)
	{
		const Material* pMaterial = ppMaterials[index];
		{
			const std::wstring debugMapName = L"Diffuse Map: " + pMaterial->m_Name;

			DirectX::ScratchImage image;
			if (pMaterial->m_DiffuseMapName.empty())
			{
				BYTE pixelBytes[4] =
				{
					BYTE(255.0f * pMaterial->m_DiffuseColor.m_X),
					BYTE(255.0f * pMaterial->m_DiffuseColor.m_Y),
					BYTE(255.0f * pMaterial->m_DiffuseColor.m_Z),
					255
				};
				GenerateImageData(1, 1, DXGI_FORMAT_R8G8B8A8_UNORM, pixelBytes, image);
			}
			else
			{
				assert(false && "Uninitialized path");
				std::wstring mapPath;
				LoadImageDataFromFile(mapPath, image);
			}
			SetupImageDataForUpload(pRenderEnv, image, debugMapName, forceSRGB, pUploadCommandList, uploadBuffers, m_Textures, pendingBarriers);
		}
		{
			const std::wstring debugMapName = L"Specular Map: " + pMaterial->m_Name;

			DirectX::ScratchImage image;
			if (pMaterial->m_SpecularMapName.empty())
			{
				BYTE pixelBytes[4] =
				{
					BYTE(255.0f * pMaterial->m_SpecularColor.m_X),
					BYTE(255.0f * pMaterial->m_SpecularColor.m_Y),
					BYTE(255.0f * pMaterial->m_SpecularColor.m_Z),
					255
				};
				GenerateImageData(1, 1, DXGI_FORMAT_R8G8B8A8_UNORM, pixelBytes, image);
			}
			else
			{
				assert(false && "Uninitialized path");
				std::wstring mapPath;
				LoadImageDataFromFile(mapPath, image);
			}
			SetupImageDataForUpload(pRenderEnv, image, debugMapName, forceSRGB, pUploadCommandList, uploadBuffers, m_Textures, pendingBarriers);
		}
		{
			const std::wstring debugMapName = L"Shininess Map: " + pMaterial->m_Name;
			
			DirectX::ScratchImage image;
			if (pMaterial->m_ShininessMapName.empty())
			{
				const std::size_t numBytes = sizeof(f32);
				static_assert(sizeof(pMaterial->m_Shininess) == numBytes, "Shininess is expected to be 32 bit");

				BYTE pixelBytes[numBytes];
				std::memcpy(pixelBytes, &pMaterial->m_Shininess, numBytes);
							
				GenerateImageData(1, 1, DXGI_FORMAT_R32_FLOAT, pixelBytes, image);
			}
			else
			{
				assert(false && "Uninitialized path");
				std::wstring mapPath;
				LoadImageDataFromFile(mapPath, image);
			}
			SetupImageDataForUpload(pRenderEnv, image, debugMapName, forceSRGB, pUploadCommandList, uploadBuffers, m_Textures, pendingBarriers);
		}
	}

	pUploadCommandList->ResourceBarrier(pendingBarriers.size(), pendingBarriers.data());
	pUploadCommandList->End();

	++pRenderEnv->m_LastSubmissionFenceValue;
	pRenderEnv->m_pCommandQueue->ExecuteCommandLists(1, &pUploadCommandList, pRenderEnv->m_pFence, pRenderEnv->m_LastSubmissionFenceValue);
	pRenderEnv->m_pFence->WaitForSignalOnCPU(pRenderEnv->m_LastSubmissionFenceValue);

	for (Buffer* pBuffer : uploadBuffers)
		SafeDelete(pBuffer);
}

MaterialRenderResources::~MaterialRenderResources()
{
	for (ColorTexture* pTexture : m_Textures)
		SafeDelete(pTexture);
}

namespace
{
	void GenerateImageData(UINT width, UINT height, DXGI_FORMAT format, std::uint8_t* pPixelBytes, DirectX::ScratchImage& image)
	{
		DirectX::Image sourceImage;
		sourceImage.width = width;
		sourceImage.height = height;
		sourceImage.format = format;
		sourceImage.rowPitch = sourceImage.width * GetSizeInBytes(format);
		sourceImage.slicePitch = sourceImage.height * sourceImage.rowPitch;
		sourceImage.pixels = pPixelBytes;
		
		image.InitializeFromImage(sourceImage);
	}
	
	void LoadImageDataFromFile(const std::wstring& filePath, DirectX::ScratchImage& image)
	{
		const std::wstring fileName = ExtractFileNameWithExtension(filePath);
		const std::wstring fileExtension = ExtractFileExtension(filePath);

		if ((fileExtension == L"DDS") || (fileExtension == L"dds"))
		{
			VerifyD3DResult(DirectX::LoadFromDDSFile(filePath.c_str(), DirectX::DDS_FLAGS_NONE, nullptr, image));
		}
		else if ((fileExtension == L"TGA") || (fileExtension == L"tga"))
		{
			DirectX::ScratchImage tempImage;
			VerifyD3DResult(DirectX::LoadFromTGAFile(filePath.c_str(), nullptr, tempImage));
			VerifyD3DResult(DirectX::GenerateMipMaps(*tempImage.GetImage(0, 0, 0), DirectX::TEX_FILTER_DEFAULT, 0, image, false));
		}
		else
		{
			DirectX::ScratchImage tempImage;
			VerifyD3DResult(DirectX::LoadFromWICFile(filePath.c_str(), DirectX::WIC_FLAGS_NONE, nullptr, tempImage));
			VerifyD3DResult(DirectX::GenerateMipMaps(*tempImage.GetImage(0, 0, 0), DirectX::TEX_FILTER_DEFAULT, 0, image, false));
		}
	}

	void SetupImageDataForUpload(RenderEnv* pRenderEnv,
		const DirectX::ScratchImage& image,
		const std::wstring& debugImageName,
		bool forceSRGB,
		CommandList* pUploadCommandList,
		std::vector<Buffer*>& uploadHeapBuffers,
		std::vector<ColorTexture*>& defaultHeapTextures,
		std::vector<ResourceBarrier>& pendingBarriers)
	{
		static const UINT maxNumSubresources = 64;
		static D3D12_PLACED_SUBRESOURCE_FOOTPRINT subresourceFootprints[maxNumSubresources];
		static UINT numRows[maxNumSubresources];
		static UINT64 rowSizeInBytes[maxNumSubresources];

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
			D3D12_RESOURCE_STATE_COPY_DEST, nullptr, debugImageName.c_str());

		defaultHeapTextures.emplace_back(pTexture);
		pendingBarriers.emplace_back(pTexture, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

		const UINT numSubresources = UINT(metaData.mipLevels * metaData.arraySize);
		assert(numSubresources <= maxNumSubresources);

		UINT64 uploadOffsetInBytes = 0;
		UINT64 textureSizeInBytes = 0;
		pRenderEnv->m_pDevice->GetCopyableFootprints(&textureDesc, 0, numSubresources, uploadOffsetInBytes, subresourceFootprints, numRows, rowSizeInBytes, &textureSizeInBytes);

		std::wstring debugBufferName = L"UploadBuffer: " + debugImageName;
		StructuredBufferDesc uploadBufferDesc(1, textureSizeInBytes, false, false);
		Buffer* pUploadBuffer = new Buffer(pRenderEnv, pRenderEnv->m_pUploadHeapProps, &uploadBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, debugBufferName.c_str());
		uploadHeapBuffers.emplace_back(pUploadBuffer);

		BYTE* pUploadMem = (BYTE*)pUploadBuffer->Map(0, textureSizeInBytes);
		for (decltype(metaData.arraySize) arrayIndex = 0; arrayIndex < metaData.arraySize; ++arrayIndex)
		{
			for (decltype(metaData.mipLevels) mipIndex = 0; mipIndex < metaData.mipLevels; ++mipIndex)
			{
				const UINT subresourceIndex = UINT(mipIndex + (arrayIndex * metaData.mipLevels));

				const D3D12_PLACED_SUBRESOURCE_FOOTPRINT& subresourceFootprint = subresourceFootprints[subresourceIndex];
				const UINT subresourceNumRows = numRows[subresourceIndex];
				const UINT subresourceRowPitch = subresourceFootprint.Footprint.RowPitch;
				const UINT subresourceDepth = subresourceFootprint.Footprint.Depth;
				BYTE* pDestSubresourceMem = pUploadMem + subresourceFootprint.Offset;

				for (UINT slice = 0; slice < subresourceDepth; ++slice)
				{
					const DirectX::Image* pSubimage = image.GetImage(mipIndex, arrayIndex, slice);
					assert(pSubimage != nullptr);
					const BYTE* pSrcSubresourceMem = pSubimage->pixels;

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
		for (UINT subresourceIndex = 0; subresourceIndex < numSubresources; ++subresourceIndex)
		{
			TextureCopyLocation destLocation(pTexture, subresourceIndex);
			TextureCopyLocation sourceLocation(pUploadBuffer, subresourceFootprints[subresourceIndex]);

			pUploadCommandList->CopyTextureRegion(&destLocation, 0, 0, 0, &sourceLocation, nullptr);
		}
	}
}
