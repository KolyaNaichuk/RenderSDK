#include "RenderPasses/CreateTiledShadowMapSATPass.h"
#include "RenderPasses/LightRenderData.h"
#include "D3DWrapper/CommandList.h"
#include "D3DWrapper/CommandSignature.h"
#include "D3DWrapper/GraphicsDevice.h"
#include "D3DWrapper/PipelineState.h"
#include "D3DWrapper/RenderEnv.h"
#include "D3DWrapper/RootSignature.h"
#include "Math/Vector2.h"
#include "Profiler/GPUProfiler.h"

namespace
{
	enum RootParams
	{
		kRoot32BitConstantsParam = 0,
		kRootSRVTableParam,
		kRootSRVParam,
		kNumRootParams
	};
	enum
	{
		kTileSizeInPixels = 32
	};
}

CreateTiledShadowMapSATPass::CreateTiledShadowMapSATPass(InitParams* pParams)
	: m_Name(pParams->m_pName)
	, m_LightType(pParams->m_LightType)
{
	InitResources(pParams);
	InitRootSignature(pParams);
	InitPipelineStates(pParams);
}

CreateTiledShadowMapSATPass::~CreateTiledShadowMapSATPass()
{	
	SafeDelete(m_pRootSignature);
	SafeDelete(m_pPipelineState);
	SafeDelete(m_pTiledShadowMapSAT);

	for (u32 index = 0; index < m_UploadTileOffsetBuffers.size(); ++index)
	{
		m_UploadTileOffsetBuffers[index]->Unmap(0, nullptr);
		SafeDelete(m_UploadTileOffsetBuffers[index]);
	}
}

void CreateTiledShadowMapSATPass::Record(RenderParams* pParams)
{
	Buffer* pTileOffsetsBuffer = m_UploadTileOffsetBuffers[pParams->m_CurrentFrameIndex];
	Vector2u* pTileOffsets = (Vector2u*)m_UploadTileOffsetBuffersMem[pParams->m_CurrentFrameIndex];
		
	u32 numTiles = 0;
	
	assert((m_LightType == LightType_Point) || (m_LightType == LightType_Spot));
	if (m_LightType == LightType_Point)
	{
		const PointLightRenderData** ppLightsData = (const PointLightRenderData**)pParams->m_ppLightsData;
		for (decltype(pParams->m_NumLights) lightIndex = 0; lightIndex < pParams->m_NumLights; ++lightIndex)
		{
			const PointLightRenderData* pLightData = ppLightsData[lightIndex];
			for (u8 faceIndex = 0; faceIndex < kNumCubeMapFaces; ++faceIndex)
			{
				const ShadowMapTile& shadowMapTile = pLightData->m_ShadowMapTiles[faceIndex];

				const u32 numSubTiles = shadowMapTile.m_SizeInPixels / kTileSizeInPixels;
				for (u32 x = 0; x < numSubTiles; ++x)
				{
					for (u32 y = 0; y < numSubTiles; ++y, ++numTiles)
					{
						pTileOffsets[numTiles].m_X = shadowMapTile.m_TopLeftInPixels.m_X + kTileSizeInPixels * x;
						pTileOffsets[numTiles].m_Y = shadowMapTile.m_TopLeftInPixels.m_Y + kTileSizeInPixels * y;
					}
				}
			}
		}
	}
	else
	{
		const SpotLightRenderData** ppLightsData = (const SpotLightRenderData**)pParams->m_ppLightsData;
		for (decltype(pParams->m_NumLights) lightIndex = 0; lightIndex < pParams->m_NumLights; ++lightIndex)
		{
			const SpotLightRenderData* pLightData = ppLightsData[lightIndex];
			const ShadowMapTile& shadowMapTile = pLightData->m_ShadowMapTile;
			
			const u32 numSubTiles = shadowMapTile.m_SizeInPixels / kTileSizeInPixels;
			for (u32 x = 0; x < numSubTiles; ++x)
			{
				for (u32 y = 0; y < numSubTiles; ++y, ++numTiles)
				{
					pTileOffsets[numTiles].m_X = shadowMapTile.m_TopLeftInPixels.m_X + kTileSizeInPixels * x;
					pTileOffsets[numTiles].m_Y = shadowMapTile.m_TopLeftInPixels.m_Y + kTileSizeInPixels * y;
				}
			}
		}
	}

	RenderEnv* pRenderEnv = pParams->m_pRenderEnv;
	CommandList* pCommandList = pParams->m_pCommandList;
	GPUProfiler* pGPUProfiler = pRenderEnv->m_pGPUProfiler;
		
	pCommandList->Begin();
#ifdef ENABLE_PROFILING
	u32 profileIndex = pGPUProfiler->StartProfile(pCommandList, m_Name.c_str());
#endif // ENABLE_PROFILING

	pCommandList->SetComputeRootSignature(m_pRootSignature);
	pCommandList->SetPipelineState(m_pPipelineState);
	pCommandList->SetDescriptorHeaps(pRenderEnv->m_pShaderVisibleSRVHeap);
	
	if (!m_ResourceBarriers.empty())
		pCommandList->ResourceBarrier((UINT)m_ResourceBarriers.size(), m_ResourceBarriers.data());
	
	pCommandList->SetComputeRootDescriptorTable(kRootSRVTableParam, m_SRVHeapStart);
	pCommandList->SetComputeRootShaderResourceView(kRootSRVParam, pTileOffsetsBuffer);
		
	pCommandList->Dispatch(numTiles, 1, 1);

#ifdef ENABLE_PROFILING
	pGPUProfiler->EndProfile(pCommandList, profileIndex);
#endif // ENABLE_PROFILING
	pCommandList->End();
}

void CreateTiledShadowMapSATPass::InitResources(InitParams* pParams)
{
	RenderEnv* pRenderEnv = pParams->m_pRenderEnv;

	m_OutputResourceStates.m_TiledVarianceShadowMapState = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
	m_OutputResourceStates.m_TiledVarianceShadowMapSATState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
		
	assert(pParams->m_pTiledShadowMap->GetWidth() == pParams->m_pTiledShadowMap->GetHeight());
	const UINT maxNumTiles = Sqr(pParams->m_pTiledShadowMap->GetWidth() / kTileSizeInPixels);
	
	assert(m_UploadTileOffsetBuffers.empty());
	assert(m_UploadTileOffsetBuffersMem.empty());
		
	m_UploadTileOffsetBuffers.resize(pParams->m_RenderLatency);
	m_UploadTileOffsetBuffersMem.resize(pParams->m_RenderLatency);

	StructuredBufferDesc tileOffsetBufferDesc(maxNumTiles, sizeof(Vector2u), false, false);
	const MemoryRange readRange(0, 0);

	for (u32 index = 0; index < pParams->m_RenderLatency; ++index)
	{
		m_UploadTileOffsetBuffers[index] = new Buffer(pRenderEnv, pRenderEnv->m_pUploadHeapProps, &tileOffsetBufferDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ, L"CreateTiledShadowMapSATPass::m_UploadTileOffsetBuffer");

		m_UploadTileOffsetBuffersMem[index] = m_UploadTileOffsetBuffers[index]->Map(0, &readRange);
	}
			
	assert(m_pTiledShadowMapSAT == nullptr);	
	const FLOAT optimizedClearColor[] = {0.0f, 0.0f, 0.0f, 0.0f};

	ColorTexture2DDesc tiledShadowMapSATDesc(DXGI_FORMAT_R32G32_FLOAT, pParams->m_pTiledShadowMap->GetWidth(),
		pParams->m_pTiledShadowMap->GetHeight(), false, true, true);
	
	 m_pTiledShadowMapSAT = new ColorTexture(pRenderEnv, pRenderEnv->m_pDefaultHeapProps, &tiledShadowMapSATDesc,
		pParams->m_InputResourceStates.m_TiledVarianceShadowMapSATState, optimizedClearColor, L"CreateTiledShadowMapSATPass::m_pTiledShadowMapSAT");
	
	assert(m_ResourceBarriers.empty());	
	
	if (pParams->m_InputResourceStates.m_TiledVarianceShadowMapState != m_OutputResourceStates.m_TiledVarianceShadowMapState)
		m_ResourceBarriers.emplace_back(pParams->m_pTiledShadowMap,
			pParams->m_InputResourceStates.m_TiledVarianceShadowMapState,
			m_OutputResourceStates.m_TiledVarianceShadowMapState);
	
	if (pParams->m_InputResourceStates.m_TiledVarianceShadowMapSATState != m_OutputResourceStates.m_TiledVarianceShadowMapSATState)
		m_ResourceBarriers.emplace_back(m_pTiledShadowMapSAT,
			pParams->m_InputResourceStates.m_TiledVarianceShadowMapSATState,
			m_OutputResourceStates.m_TiledVarianceShadowMapSATState);


	assert(!m_SRVHeapStart.IsValid());	
	m_SRVHeapStart = pRenderEnv->m_pShaderVisibleSRVHeap->Allocate();

	pRenderEnv->m_pDevice->CopyDescriptor(m_SRVHeapStart,
		pParams->m_pTiledShadowMap->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	pRenderEnv->m_pDevice->CopyDescriptor(pRenderEnv->m_pShaderVisibleSRVHeap->Allocate(),
		m_pTiledShadowMapSAT->GetUAVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}

void CreateTiledShadowMapSATPass::InitRootSignature(InitParams* pParams)
{	
	assert(m_pRootSignature == nullptr);
	D3D12_ROOT_PARAMETER rootParams[kNumRootParams];

	rootParams[kRoot32BitConstantsParam] = Root32BitConstantsParameter(0, D3D12_SHADER_VISIBILITY_ALL, 2);
	
	D3D12_DESCRIPTOR_RANGE descriptorRanges[] =
	{
		SRVDescriptorRange(1, 0),
		UAVDescriptorRange(1, 0)
	};
	rootParams[kRootSRVTableParam] = RootDescriptorTableParameter(ARRAYSIZE(descriptorRanges), descriptorRanges, D3D12_SHADER_VISIBILITY_ALL);
	rootParams[kRootSRVParam] = RootSRVParameter(1, D3D12_SHADER_VISIBILITY_ALL);
	
	RootSignatureDesc rootSignatureDesc(kNumRootParams, rootParams);
	m_pRootSignature = new RootSignature(pParams->m_pRenderEnv->m_pDevice, &rootSignatureDesc, L"CreateTiledShadowMapSATPass::m_pRootSignature");
}

void CreateTiledShadowMapSATPass::InitPipelineStates(InitParams* pParams)
{
	assert(m_pRootSignature != nullptr);
	assert(m_pPipelineState == nullptr);
		
	RenderEnv* pRenderEnv = pParams->m_pRenderEnv;
	
	std::string tileSizeStr = std::to_string(kTileSizeInPixels);
	const ShaderMacro shaderDefines[] =
	{
		ShaderMacro("TILE_SIZE", tileSizeStr.c_str()),
		ShaderMacro()
	};
	Shader computeShader(L"Shaders//CreateTiledShadowMapSATCS.hlsl", "Main", "cs_5_0", shaderDefines);

	ComputePipelineStateDesc pipelineStateDesc;
	pipelineStateDesc.SetRootSignature(m_pRootSignature);
	pipelineStateDesc.SetComputeShader(&computeShader);

	m_pPipelineState = new PipelineState(pRenderEnv->m_pDevice, &pipelineStateDesc, L"CreateTiledShadowMapSATPass::m_pPipelineState");
}
