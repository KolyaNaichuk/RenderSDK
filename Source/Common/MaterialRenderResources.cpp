#include "Common/MaterialRenderResources.h"
#include "Common/Material.h"
#include "D3DWrapper/CommandList.h"
#include "D3DWrapper/GraphicsResource.h"
#include "D3DWrapper/RenderEnv.h"
#include "Math/Vector4.h"

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
		
		if (pMaterial->m_DiffuseMapName.empty())
		{
			Vector4f textureColor(pMaterial->m_DiffuseColor.m_X, pMaterial->m_DiffuseColor.m_Y, pMaterial->m_DiffuseColor.m_Z, 1.0f);
			std::wstring textureDebugName = pMaterial->m_Name + L" Diffuse Map";
									
			assert(false && "Fix format");
			ColorTexture2DDesc textureDesc(DXGI_FORMAT_R16G16B16A16_FLOAT, texturePlaceholderSize, texturePlaceholderSize, true, true, false);
			ColorTexture* pTexture = new ColorTexture(pRenderEnv, pRenderEnv->m_pDefaultHeapProps, &textureDesc,
				D3D12_RESOURCE_STATE_RENDER_TARGET, &textureColor.m_X, textureDebugName.c_str());

			m_Textures.emplace_back(pTexture);
			texturePlaceholders.emplace_back(pTexture, textureColor);
		}
		else
		{
			assert(false && "Missing impl");
		}
		
		if (pMaterial->m_SpecularMapName.empty())
		{
			Vector4f textureColor(pMaterial->m_SpecularColor.m_X, pMaterial->m_SpecularColor.m_Y, pMaterial->m_SpecularColor.m_Z, 1.0f);
			std::wstring textureDebugName = pMaterial->m_Name + L" Specular Map";
			
			assert(false && "Fix format");
			ColorTexture2DDesc textureDesc(DXGI_FORMAT_R16G16B16A16_FLOAT, texturePlaceholderSize, texturePlaceholderSize, true, true, false);
			ColorTexture* pTexture = new ColorTexture(pRenderEnv, pRenderEnv->m_pDefaultHeapProps, &textureDesc,
				D3D12_RESOURCE_STATE_RENDER_TARGET, &textureColor.m_X, textureDebugName.c_str());

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
			std::wstring textureDebugName = pMaterial->m_Name + L" Shininess Map";
			
			assert(false && "Fix format");
			ColorTexture2DDesc textureDesc(DXGI_FORMAT_R16_FLOAT, texturePlaceholderSize, texturePlaceholderSize, true, true, false);
			ColorTexture* pTexture = new ColorTexture(pRenderEnv, pRenderEnv->m_pDefaultHeapProps, &textureDesc,
				D3D12_RESOURCE_STATE_RENDER_TARGET, &textureColor.m_X, textureDebugName.c_str());

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
