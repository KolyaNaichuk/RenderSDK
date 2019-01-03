#include "RenderPasses/MaterialRenderResources.h"
#include "Common/FileUtilities.h"
#include "D3DWrapper/CommandList.h"
#include "D3DWrapper/GraphicsDevice.h"
#include "D3DWrapper/GraphicsResource.h"
#include "D3DWrapper/GraphicsUtils.h"
#include "D3DWrapper/RenderEnv.h"
#include "DirectXTex/DirectXTex.h"
#include "Math/Vector4.h"
#include "Scene/Material.h"

namespace
{
	void GenerateImageData(UINT width, UINT height, DXGI_FORMAT format, u8* pPixelBytes, DirectX::ScratchImage& image);
	UINT CountMips(UINT width, UINT height);

	void LoadImageDataFromFile(const std::wstring& filePath, DirectX::ScratchImage& image, bool generateMips);
		
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
{
	assert(numMaterials > 0);

	// We are managing 1 + numMaterials. The material at index 0 is reserved for the unknown material ID.
	// Pixels in the GBuffer not affected by rendered geometry contain unknown material ID, that is 0.

	const bool generateMips = true;
	const u16 maxNumTextures = (1 + numMaterials) * Material::NumTextures;

	const u16 meshType = 0;
	const u16 numMeshTypes = 1;
	std::vector<u16> meshTypePerMaterialID;
	meshTypePerMaterialID.resize(1 + numMaterials, meshType);
	meshTypePerMaterialID[0] = numMeshTypes; // Unknown material ID

	assert(m_Textures.empty());
	m_Textures.reserve(maxNumTextures);

	std::vector<Buffer*> uploadBuffers;
	uploadBuffers.reserve(maxNumTextures);

	std::vector<ResourceTransitionBarrier> pendingTransitionBarriers;
	pendingTransitionBarriers.reserve(maxNumTextures);

	std::vector<u16> materialTextureIndices;
	materialTextureIndices.reserve(maxNumTextures);

	// Adding bogus material texture indices for the unknown material ID
	// to be able to calculate first texture index by formula materialID * Material::NumTextures
	materialTextureIndices.insert(materialTextureIndices.end(), Material::NumTextures, 0);

	std::unordered_map<std::wstring, u16> loadedTextures;

	CommandList* pUploadCommandList = pRenderEnv->m_pCommandListPool->Create(L"MaterialRenderResources::pUploadCommandList");
	pUploadCommandList->Begin();

	for (u16 materialIndex = 0; materialIndex < numMaterials; ++materialIndex)
	{
		const Material* pMaterial = ppMaterials[materialIndex];
		for (u16 textureIndex = 0; textureIndex < Material::NumTextures; ++textureIndex)
		{
			const bool forceSRGB = (textureIndex == Material::BaseColorTextureIndex);

			const std::wstring& textureFilePath = pMaterial->m_FilePaths[textureIndex];
			assert(!textureFilePath.empty());

			u16 globalTextureIndex = loadedTextures.size();

			auto it = loadedTextures.find(textureFilePath);
			if (it != loadedTextures.end())
			{
				globalTextureIndex = it->second;
			}
			else
			{
				DirectX::ScratchImage image;
				LoadImageDataFromFile(textureFilePath, image, generateMips);

				const std::wstring& debugTextureName = textureFilePath;
				SetupImageDataForUpload(pRenderEnv, image, debugTextureName, forceSRGB, pUploadCommandList, uploadBuffers, m_Textures, pendingTransitionBarriers);

				loadedTextures.emplace(textureFilePath, globalTextureIndex);
			}

			materialTextureIndices.emplace_back(globalTextureIndex);
		}
	}

	pUploadCommandList->ResourceBarrier(pendingTransitionBarriers.size(), pendingTransitionBarriers.data());
	pUploadCommandList->End();

	++pRenderEnv->m_LastSubmissionFenceValue;
	pRenderEnv->m_pCommandQueue->ExecuteCommandLists(1, &pUploadCommandList, pRenderEnv->m_pFence, pRenderEnv->m_LastSubmissionFenceValue);
	pRenderEnv->m_pFence->WaitForSignalOnCPU(pRenderEnv->m_LastSubmissionFenceValue);

	for (Buffer* pBuffer : uploadBuffers)
		SafeDelete(pBuffer);

	FormattedBufferDesc meshTypePerMaterialIDBufferDesc(meshTypePerMaterialID.size(), DXGI_FORMAT_R16_UINT, true, false);
	m_pMeshTypePerMaterialIDBuffer = new Buffer(pRenderEnv, pRenderEnv->m_pDefaultHeapProps, &meshTypePerMaterialIDBufferDesc,
		D3D12_RESOURCE_STATE_COPY_DEST, L"MaterialRenderResources::m_pMeshTypePerMaterialIDBuffer");

	UploadData(pRenderEnv, m_pMeshTypePerMaterialIDBuffer, meshTypePerMaterialIDBufferDesc, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		meshTypePerMaterialID.data(), meshTypePerMaterialID.size() * sizeof(meshTypePerMaterialID[0]));

	FormattedBufferDesc materialTextureIndicesBufferDesc(materialTextureIndices.size(), DXGI_FORMAT_R16_UINT, true, false);
	m_pMaterialTextureIndicesBuffer = new Buffer(pRenderEnv, pRenderEnv->m_pDefaultHeapProps, &materialTextureIndicesBufferDesc,
		D3D12_RESOURCE_STATE_COPY_DEST, L"MaterialRenderResources::m_pMaterialTextureIndicesBuffer");

	UploadData(pRenderEnv, m_pMaterialTextureIndicesBuffer, materialTextureIndicesBufferDesc, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		materialTextureIndices.data(), materialTextureIndices.size() * sizeof(materialTextureIndices[0]));
}

MaterialRenderResources::~MaterialRenderResources()
{
	SafeDelete(m_pMeshTypePerMaterialIDBuffer);
	SafeDelete(m_pMaterialTextureIndicesBuffer);
	
	for (ColorTexture* pTexture : m_Textures)
		SafeDelete(pTexture);
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

	UINT CountMips(UINT width, UINT height)
	{
		UINT mipLevels = 1;
		while (height > 1 || width > 1)
		{
			if (height > 1)
				height >>= 1;

			if (width > 1)
				width >>= 1;

			++mipLevels;
		}
		return mipLevels;
	}
	
	void LoadImageDataFromFile(const std::wstring& filePath, DirectX::ScratchImage& image, bool generateMips)
	{
		const std::wstring fileExtension = ExtractFileExtension(filePath);
		if ((fileExtension == L"DDS") || (fileExtension == L"dds"))
		{
			VerifyD3DResult(DirectX::LoadFromDDSFile(filePath.c_str(), DirectX::DDS_FLAGS_NONE, nullptr, image));
		}
		else if ((fileExtension == L"TGA") || (fileExtension == L"tga"))
		{
			VerifyD3DResult(DirectX::LoadFromTGAFile(filePath.c_str(), nullptr, image));
		}
		else
		{
			VerifyD3DResult(DirectX::LoadFromWICFile(filePath.c_str(), DirectX::WIC_FLAGS_NONE, nullptr, image));
		}

		if (generateMips)
		{
			const DirectX::TexMetadata& metaData = image.GetMetadata();
			if (CountMips(metaData.width, metaData.height) > 1)
			{
				DirectX::ScratchImage mipMappedImage;
				VerifyD3DResult(DirectX::GenerateMipMaps(*image.GetImage(0, 0, 0), DirectX::TEX_FILTER_DEFAULT, 0, mipMappedImage, false));
				
				image = std::move(mipMappedImage);
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
		
		assert(metaData.dimension == DirectX::TEX_DIMENSION_TEXTURE2D);
		assert(metaData.arraySize == 1);

		ColorTexture2DDesc texDesc(format, UINT64(metaData.width), UINT(metaData.height),
			false/*createRTV*/, true/*createSRV*/, false/*createUAV*/, UINT16(metaData.mipLevels));

		ColorTexture* pTexture = new ColorTexture(pRenderEnv, pRenderEnv->m_pDefaultHeapProps, &texDesc,
			D3D12_RESOURCE_STATE_COPY_DEST, nullptr, debugImageName.c_str());
						
		defaultHeapTextures.emplace_back(pTexture);
		pendingTransitionBarriers.emplace_back(pTexture, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

		const UINT numSubresources = UINT(metaData.arraySize * metaData.mipLevels);
		assert(numSubresources <= maxNumSubresources);

		UINT64 uploadOffsetInBytes = 0;
		UINT64 textureSizeInBytes = 0;
		pRenderEnv->m_pDevice->GetCopyableFootprints(&texDesc, 0, numSubresources, uploadOffsetInBytes, subresourceFootprints, numRows, rowSizeInBytes, &textureSizeInBytes);

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
