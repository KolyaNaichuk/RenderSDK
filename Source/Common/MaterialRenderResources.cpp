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
	void GenerateImageData(UINT width, UINT height, DXGI_FORMAT format, u8* pPixelBytes, DirectX::ScratchImage& image);
	void LoadImageDataFromFile(const std::wstring& filePath, DirectX::ScratchImage& image);
		
	void SetupImageDataForUpload(RenderEnv* pRenderEnv,
		const DirectX::ScratchImage& image,
		const std::wstring& debugImageName,
		bool forceSRGB,
		CommandList* pUploadCommandList,
		std::vector<Buffer*>& uploadHeapBuffers,
		std::vector<ColorTexture*>& defaultHeapTextures,
		std::vector<ResourceTransitionBarrier>& pendingTransitionBarriers);
}

MaterialRenderResources::MaterialRenderResources(RenderEnv* pRenderEnv, u16 numMaterials, Material** ppMaterials)
	: m_pMeshTypePerMaterialIDBuffer(nullptr)
	, m_pFirstResourceIndexPerMaterialIDBuffer(nullptr)
{
	assert(numMaterials > 0);

	InitMeshTypePerMaterialIDBuffer(pRenderEnv, numMaterials);
	InitFirstResourceIndexPerMaterialIDBuffer(pRenderEnv, numMaterials);
	InitTextures(pRenderEnv, numMaterials, ppMaterials);
}

MaterialRenderResources::~MaterialRenderResources()
{
	SafeDelete(m_pMeshTypePerMaterialIDBuffer);
	SafeDelete(m_pFirstResourceIndexPerMaterialIDBuffer);
	
	for (ColorTexture* pTexture : m_Textures)
		SafeDelete(pTexture);
}

void MaterialRenderResources::InitMeshTypePerMaterialIDBuffer(RenderEnv* pRenderEnv, u16 numMaterials)
{
	assert(m_pMeshTypePerMaterialIDBuffer == nullptr);

	const u16 meshType = 0;
	std::vector<u16> bufferData(1 + numMaterials, std::numeric_limits<u16>::max());
	for (u16 materialID = 1; materialID < bufferData.size(); ++materialID)
		bufferData[materialID] = meshType;
		
	FormattedBufferDesc bufferDesc(bufferData.size(), DXGI_FORMAT_R16_UINT, true, false);
	m_pMeshTypePerMaterialIDBuffer = new Buffer(pRenderEnv, pRenderEnv->m_pDefaultHeapProps, &bufferDesc,
		D3D12_RESOURCE_STATE_COPY_DEST, L"MaterialRenderResources::m_pMeshTypePerMaterialIDBuffer");

	UploadData(pRenderEnv, m_pMeshTypePerMaterialIDBuffer, bufferDesc, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		bufferData.data(), bufferData.size() * sizeof(bufferData[0]));
}

void MaterialRenderResources::InitFirstResourceIndexPerMaterialIDBuffer(RenderEnv* pRenderEnv, u16 numMaterials)
{
	assert(m_pFirstResourceIndexPerMaterialIDBuffer == nullptr);

	std::vector<u16> bufferData(1 + numMaterials, std::numeric_limits<u16>::max());
	for (u16 materialID = 1; materialID < bufferData.size(); ++materialID)
		bufferData[materialID] = 3 * (materialID - 1);

	FormattedBufferDesc bufferDesc(bufferData.size(), DXGI_FORMAT_R16_UINT, true, false);
	m_pFirstResourceIndexPerMaterialIDBuffer = new Buffer(pRenderEnv, pRenderEnv->m_pDefaultHeapProps, &bufferDesc,
		D3D12_RESOURCE_STATE_COPY_DEST, L"MaterialRenderResources::m_pFirstResourceIndexPerMaterialIDBuffer");

	UploadData(pRenderEnv, m_pFirstResourceIndexPerMaterialIDBuffer, bufferDesc, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		bufferData.data(), bufferData.size() * sizeof(bufferData[0]));
}

void MaterialRenderResources::InitTextures(RenderEnv* pRenderEnv, u16 numMaterials, Material** ppMaterials)
{
	assert(m_Textures.empty());

	static const bool forceSRGB = true;
	static const u8 numTexturesPerMaterial = 3;
	
	const u16 maxNumTextures = numMaterials * numTexturesPerMaterial;

	std::vector<Buffer*> uploadBuffers;
	std::vector<ResourceTransitionBarrier> pendingTransitionBarriers;

	m_Textures.reserve(maxNumTextures);
	uploadBuffers.reserve(maxNumTextures);
	pendingTransitionBarriers.reserve(maxNumTextures);

	CommandList* pUploadCommandList = pRenderEnv->m_pCommandListPool->Create(L"UploadCommandList");
	pUploadCommandList->Begin();

	for (u16 index = 0; index < numMaterials; ++index)
	{
		const Material* pMaterial = ppMaterials[index];
		{
			const std::wstring debugMapName = L"Diffuse Map: " + pMaterial->m_Name;

			DirectX::ScratchImage image;
			if (pMaterial->m_DiffuseMapFilePath.empty())
			{
				u8 pixelBytes[4] =
				{
					u8(255.0f * pMaterial->m_DiffuseColor.m_X),
					u8(255.0f * pMaterial->m_DiffuseColor.m_Y),
					u8(255.0f * pMaterial->m_DiffuseColor.m_Z),
					255
				};
				GenerateImageData(1, 1, DXGI_FORMAT_R8G8B8A8_UNORM, pixelBytes, image);
			}
			else
			{
				LoadImageDataFromFile(pMaterial->m_DiffuseMapFilePath, image);
			}
			SetupImageDataForUpload(pRenderEnv, image, debugMapName, forceSRGB, pUploadCommandList, uploadBuffers, m_Textures, pendingTransitionBarriers);
		}
		{
			const std::wstring debugMapName = L"Specular Map: " + pMaterial->m_Name;

			DirectX::ScratchImage image;
			if (pMaterial->m_SpecularMapFilePath.empty())
			{
				u8 pixelBytes[4] =
				{
					u8(255.0f * pMaterial->m_SpecularColor.m_X),
					u8(255.0f * pMaterial->m_SpecularColor.m_Y),
					u8(255.0f * pMaterial->m_SpecularColor.m_Z),
					255
				};
				GenerateImageData(1, 1, DXGI_FORMAT_R8G8B8A8_UNORM, pixelBytes, image);
			}
			else
			{
				LoadImageDataFromFile(pMaterial->m_SpecularMapFilePath, image);
			}
			SetupImageDataForUpload(pRenderEnv, image, debugMapName, forceSRGB, pUploadCommandList, uploadBuffers, m_Textures, pendingTransitionBarriers);
		}
		{
			const std::wstring debugMapName = L"Shininess Map: " + pMaterial->m_Name;

			DirectX::ScratchImage image;
			if (pMaterial->m_ShininessMapFilePath.empty())
			{
				const std::size_t numBytes = sizeof(f32);
				static_assert(sizeof(pMaterial->m_Shininess) == numBytes, "Shininess is expected to be 32 bit");

				u8 pixelBytes[numBytes];
				std::memcpy(pixelBytes, &pMaterial->m_Shininess, numBytes);

				GenerateImageData(1, 1, DXGI_FORMAT_R32_FLOAT, pixelBytes, image);
			}
			else
			{
				LoadImageDataFromFile(pMaterial->m_ShininessMapFilePath, image);
			}
			SetupImageDataForUpload(pRenderEnv, image, debugMapName, forceSRGB, pUploadCommandList, uploadBuffers, m_Textures, pendingTransitionBarriers);
		}
	}

	pUploadCommandList->ResourceBarrier(pendingTransitionBarriers.size(), pendingTransitionBarriers.data());
	pUploadCommandList->End();

	++pRenderEnv->m_LastSubmissionFenceValue;
	pRenderEnv->m_pCommandQueue->ExecuteCommandLists(1, &pUploadCommandList, pRenderEnv->m_pFence, pRenderEnv->m_LastSubmissionFenceValue);
	pRenderEnv->m_pFence->WaitForSignalOnCPU(pRenderEnv->m_LastSubmissionFenceValue);

	for (Buffer* pBuffer : uploadBuffers)
		SafeDelete(pBuffer);
}

namespace
{
	void GenerateImageData(UINT width, UINT height, DXGI_FORMAT format, u8* pPixelBytes, DirectX::ScratchImage& image)
	{
		DirectX::Image sourceImage;
		sourceImage.width = width;
		sourceImage.height = height;
		sourceImage.format = format;
		sourceImage.rowPitch = sourceImage.width * GetSizeInBytes(format);
		sourceImage.slicePitch = sourceImage.height * sourceImage.rowPitch;
		sourceImage.pixels = pPixelBytes;
		
		VerifyD3DResult(image.InitializeFromImage(sourceImage));
	}
	
	void LoadImageDataFromFile(const std::wstring& filePath, DirectX::ScratchImage& image)
	{
		const std::wstring fileExtension = ExtractFileExtension(filePath);
		if ((fileExtension == L"DDS") || (fileExtension == L"dds"))
		{
			VerifyD3DResult(DirectX::LoadFromDDSFile(filePath.c_str(), DirectX::DDS_FLAGS_NONE, nullptr, image));
		}
		else if ((fileExtension == L"TGA") || (fileExtension == L"tga"))
		{
			// KolyaMipLevels
			bool enableMipLevels = false;
			if (enableMipLevels)
			{
				DirectX::ScratchImage tempImage;
				VerifyD3DResult(DirectX::LoadFromTGAFile(filePath.c_str(), nullptr, tempImage));
				VerifyD3DResult(DirectX::GenerateMipMaps(*tempImage.GetImage(0, 0, 0), DirectX::TEX_FILTER_DEFAULT, 0, image, false));
			}
			else
			{
				VerifyD3DResult(DirectX::LoadFromTGAFile(filePath.c_str(), nullptr, image));
			}
		}
		else
		{
			// KolyaMipLevels
			bool enableMipLevels = false;
			if (enableMipLevels)
			{
				DirectX::ScratchImage tempImage;
				VerifyD3DResult(DirectX::LoadFromWICFile(filePath.c_str(), DirectX::WIC_FLAGS_NONE, nullptr, tempImage));
				VerifyD3DResult(DirectX::GenerateMipMaps(*tempImage.GetImage(0, 0, 0), DirectX::TEX_FILTER_DEFAULT, 0, image, false));
			}
			else
			{
				VerifyD3DResult(DirectX::LoadFromWICFile(filePath.c_str(), DirectX::WIC_FLAGS_NONE, nullptr, image));
			}
		}
	}

	void SetupImageDataForUpload(RenderEnv* pRenderEnv,
		const DirectX::ScratchImage& image,
		const std::wstring& debugImageName,
		bool forceSRGB,
		CommandList* pUploadCommandList,
		std::vector<Buffer*>& uploadHeapBuffers,
		std::vector<ColorTexture*>& defaultHeapTextures,
		std::vector<ResourceTransitionBarrier>& pendingTransitionBarriers)
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

		ColorTextureDesc textureDesc(dimension, format, UINT64(metaData.width), UINT(metaData.height),
			false, true, false, depthOrArraySize, UINT16(metaData.mipLevels));
		ColorTexture* pTexture = new ColorTexture(pRenderEnv, pRenderEnv->m_pDefaultHeapProps, &textureDesc,
			D3D12_RESOURCE_STATE_COPY_DEST, nullptr, debugImageName.c_str());

		defaultHeapTextures.emplace_back(pTexture);
		pendingTransitionBarriers.emplace_back(pTexture, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

		const UINT numSubresources = UINT(metaData.mipLevels * metaData.arraySize);
		assert(numSubresources <= maxNumSubresources);

		UINT64 uploadOffsetInBytes = 0;
		UINT64 textureSizeInBytes = 0;
		pRenderEnv->m_pDevice->GetCopyableFootprints(&textureDesc, 0, numSubresources, uploadOffsetInBytes, subresourceFootprints, numRows, rowSizeInBytes, &textureSizeInBytes);

		std::wstring debugBufferName = L"UploadBuffer: " + debugImageName;
		StructuredBufferDesc uploadBufferDesc(1, textureSizeInBytes, false, false);
		Buffer* pUploadBuffer = new Buffer(pRenderEnv, pRenderEnv->m_pUploadHeapProps, &uploadBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, debugBufferName.c_str());
		uploadHeapBuffers.emplace_back(pUploadBuffer);

		MemoryRange readRange(0, textureSizeInBytes);
		u8* pUploadMem = (u8*)pUploadBuffer->Map(0, &readRange);
		for (decltype(metaData.arraySize) arrayIndex = 0; arrayIndex < metaData.arraySize; ++arrayIndex)
		{
			for (decltype(metaData.mipLevels) mipIndex = 0; mipIndex < metaData.mipLevels; ++mipIndex)
			{
				const UINT subresourceIndex = UINT(mipIndex + (arrayIndex * metaData.mipLevels));

				const D3D12_PLACED_SUBRESOURCE_FOOTPRINT& subresourceFootprint = subresourceFootprints[subresourceIndex];
				const UINT subresourceNumRows = numRows[subresourceIndex];
				const UINT subresourceRowPitch = subresourceFootprint.Footprint.RowPitch;
				const UINT subresourceDepth = subresourceFootprint.Footprint.Depth;
				u8* pDestSubresourceMem = pUploadMem + subresourceFootprint.Offset;

				for (UINT slice = 0; slice < subresourceDepth; ++slice)
				{
					const DirectX::Image* pSubimage = image.GetImage(mipIndex, arrayIndex, slice);
					assert(pSubimage != nullptr);
					const u8* pSrcSubresourceMem = pSubimage->pixels;

					const UINT bytesToCopy = Min(subresourceRowPitch, (UINT)pSubimage->rowPitch);
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
