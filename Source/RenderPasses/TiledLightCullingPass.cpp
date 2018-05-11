#include "RenderPasses/TiledLightCullingPass.h"
#include "D3DWrapper/CommandList.h"
#include "D3DWrapper/GraphicsDevice.h"
#include "D3DWrapper/PipelineState.h"
#include "D3DWrapper/RenderEnv.h"
#include "D3DWrapper/RootSignature.h"
#include "Profiler/GPUProfiler.h"

namespace
{
	enum RootParams
	{
		kRootCBVParam = 0,
		kRoot32BitConstantsParam,
		kRootSRVTableParam,
		kNumRootParams
	};
	struct Range
	{
		u32 m_Start;
		u32 m_Length;
	};
}

TiledLightCullingPass::TiledLightCullingPass(InitParams* pParams)
	: m_Name(pParams->m_pName)
	, m_NumThreadGroupsX(pParams->m_NumTilesX)
	, m_NumThreadGroupsY(pParams->m_NumTilesY)
{
	assert(pParams->m_MaxNumSpotLights > 0);

	InitResources(pParams);
	InitRootSignature(pParams);
	InitPipelineState(pParams);
}

TiledLightCullingPass::~TiledLightCullingPass()
{
	SafeDelete(m_pRootSignature);
	SafeDelete(m_pPipelineState);
	SafeDelete(m_pSpotLightIndicesOffsetBuffer);
	SafeDelete(m_pSpotLightIndexPerTileBuffer);
	SafeDelete(m_pSpotLightRangePerTileBuffer);
}

void TiledLightCullingPass::Record(RenderParams* pParams)
{
	RenderEnv* pRenderEnv = pParams->m_pRenderEnv;
	CommandList* pCommandList = pParams->m_pCommandList;
	GPUProfiler* pGPUProfiler = pRenderEnv->m_pGPUProfiler;
	
	pCommandList->Begin(m_pPipelineState);
#ifdef ENABLE_PROFILING
	u32 profileIndex = pGPUProfiler->StartProfile(pCommandList, m_Name.c_str());
#endif // ENABLE_PROFILING

	pCommandList->SetComputeRootSignature(m_pRootSignature);

	if (!m_ResourceBarriers.empty())
		pCommandList->ResourceBarrier((UINT)m_ResourceBarriers.size(), m_ResourceBarriers.data());

	const UINT clearValue[4] = {0, 0, 0, 0};
	const UINT constants32Bit[] = {pParams->m_NumSpotLights};

	pCommandList->SetDescriptorHeaps(pRenderEnv->m_pShaderVisibleSRVHeap);
	pCommandList->SetComputeRootConstantBufferView(kRootCBVParam, pParams->m_pAppDataBuffer);
	pCommandList->SetComputeRoot32BitConstants(kRoot32BitConstantsParam, ARRAYSIZE(constants32Bit), constants32Bit, 0);
	pCommandList->SetComputeRootDescriptorTable(kRootSRVTableParam, m_SRVHeapStart);
	pCommandList->ClearUnorderedAccessView(DescriptorHandle(m_SRVHeapStart, 2), m_pSpotLightIndicesOffsetBuffer->GetUAVHandle(), m_pSpotLightIndicesOffsetBuffer, clearValue);
	pCommandList->Dispatch(m_NumThreadGroupsX, m_NumThreadGroupsY, 1);

#ifdef ENABLE_PROFILING
	pGPUProfiler->EndProfile(pCommandList, profileIndex);
#endif // ENABLE_PROFILING
	pCommandList->End();
}

void TiledLightCullingPass::InitResources(InitParams* pParams)
{
	RenderEnv* pRenderEnv = pParams->m_pRenderEnv;
	const u32 numTiles = pParams->m_NumTilesX * pParams->m_NumTilesY;

	m_OutputResourceStates.m_DepthTextureState = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
	m_OutputResourceStates.m_SpotLightWorldBoundsBufferState = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
	m_OutputResourceStates.m_SpotLightIndexPerTileBufferState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
	m_OutputResourceStates.m_SpotLightRangePerTileBufferState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
		
	assert(m_pSpotLightIndicesOffsetBuffer == nullptr);
	FormattedBufferDesc lightIndicesOffsetBufferDesc(1, DXGI_FORMAT_R32_UINT, true, true);
	m_pSpotLightIndicesOffsetBuffer = new Buffer(pRenderEnv, pRenderEnv->m_pDefaultHeapProps, &lightIndicesOffsetBufferDesc,
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS, L"m_pSpotLightIndicesOffsetBuffer");

	assert(m_pSpotLightIndexPerTileBuffer == nullptr);
	FormattedBufferDesc lightIndexPerTileBufferDesc(numTiles * pParams->m_MaxNumSpotLights, DXGI_FORMAT_R32_UINT, true, true);
	m_pSpotLightIndexPerTileBuffer = new Buffer(pRenderEnv, pRenderEnv->m_pDefaultHeapProps, &lightIndexPerTileBufferDesc,
		pParams->m_InputResourceStates.m_SpotLightIndexPerTileBufferState, L"m_pSpotLightIndexPerTileBuffer");

	assert(m_pSpotLightRangePerTileBuffer == nullptr);
	StructuredBufferDesc lightRangePerTileBufferDesc(numTiles, sizeof(Range), true, true);
	m_pSpotLightRangePerTileBuffer = new Buffer(pRenderEnv, pRenderEnv->m_pDefaultHeapProps, &lightRangePerTileBufferDesc,
		pParams->m_InputResourceStates.m_SpotLightRangePerTileBufferState, L"m_pSpotLightRangePerTileBuffer");

	assert(m_ResourceBarriers.empty());
	AddResourceBarrierIfRequired(pParams->m_pDepthTexture,
		pParams->m_InputResourceStates.m_DepthTextureState,
		m_OutputResourceStates.m_DepthTextureState);

	AddResourceBarrierIfRequired(pParams->m_pSpotLightWorldBoundsBuffer,
		pParams->m_InputResourceStates.m_SpotLightWorldBoundsBufferState,
		m_OutputResourceStates.m_SpotLightWorldBoundsBufferState);

	AddResourceBarrierIfRequired(m_pSpotLightIndexPerTileBuffer,
		pParams->m_InputResourceStates.m_SpotLightIndexPerTileBufferState,
		m_OutputResourceStates.m_SpotLightIndexPerTileBufferState);

	AddResourceBarrierIfRequired(m_pSpotLightRangePerTileBuffer,
		pParams->m_InputResourceStates.m_SpotLightRangePerTileBufferState,
		m_OutputResourceStates.m_SpotLightRangePerTileBufferState);
	
	m_SRVHeapStart = pRenderEnv->m_pShaderVisibleSRVHeap->Allocate();
	pRenderEnv->m_pDevice->CopyDescriptor(m_SRVHeapStart,
		pParams->m_pDepthTexture->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	pRenderEnv->m_pDevice->CopyDescriptor(pRenderEnv->m_pShaderVisibleSRVHeap->Allocate(),
		pParams->m_pSpotLightWorldBoundsBuffer->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	pRenderEnv->m_pDevice->CopyDescriptor(pRenderEnv->m_pShaderVisibleSRVHeap->Allocate(),
		m_pSpotLightIndicesOffsetBuffer->GetUAVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	pRenderEnv->m_pDevice->CopyDescriptor(pRenderEnv->m_pShaderVisibleSRVHeap->Allocate(),
		m_pSpotLightIndexPerTileBuffer->GetUAVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	pRenderEnv->m_pDevice->CopyDescriptor(pRenderEnv->m_pShaderVisibleSRVHeap->Allocate(),
		m_pSpotLightRangePerTileBuffer->GetUAVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}

void TiledLightCullingPass::InitRootSignature(InitParams* pParams)
{
	assert(m_pRootSignature == nullptr);

	D3D12_ROOT_PARAMETER rootParams[kNumRootParams];
	rootParams[kRootCBVParam] = RootCBVParameter(0, D3D12_SHADER_VISIBILITY_ALL);
	rootParams[kRoot32BitConstantsParam] = Root32BitConstantsParameter(1, D3D12_SHADER_VISIBILITY_ALL, 1);

	std::vector<D3D12_DESCRIPTOR_RANGE> descriptorRanges;
	descriptorRanges.push_back(SRVDescriptorRange(2, 0));
	descriptorRanges.push_back(UAVDescriptorRange(3, 0));
	
	rootParams[kRootSRVTableParam] = RootDescriptorTableParameter((UINT)descriptorRanges.size(), descriptorRanges.data(), D3D12_SHADER_VISIBILITY_ALL);

	RootSignatureDesc rootSignatureDesc(kNumRootParams, rootParams);
	m_pRootSignature = new RootSignature(pParams->m_pRenderEnv->m_pDevice, &rootSignatureDesc, L"TiledLightCullingPass::m_pRootSignature");
}

void TiledLightCullingPass::InitPipelineState(InitParams* pParams)
{
	assert(m_pRootSignature != nullptr);
	assert(m_pPipelineState == nullptr);

	RenderEnv* pRenderEnv = pParams->m_pRenderEnv;

	std::string tileSizeStr = std::to_string(pParams->m_TileSize);
	std::string numTilesXStr = std::to_string(pParams->m_NumTilesX);
	std::string maxNumSpotLightsStr = std::to_string(pParams->m_MaxNumSpotLights);

	const ShaderMacro shaderDefines[] =
	{
		ShaderMacro("TILE_SIZE", tileSizeStr.c_str()),
		ShaderMacro("NUM_TILES_X", numTilesXStr.c_str()),
		ShaderMacro("MAX_NUM_SPOT_LIGHTS", maxNumSpotLightsStr.c_str()),
		ShaderMacro()
	};
	Shader computeShader(L"Shaders//TiledLightCullingCS.hlsl", "Main", "cs_5_0", shaderDefines);

	ComputePipelineStateDesc pipelineStateDesc;
	pipelineStateDesc.SetRootSignature(m_pRootSignature);
	pipelineStateDesc.SetComputeShader(&computeShader);

	m_pPipelineState = new PipelineState(pRenderEnv->m_pDevice, &pipelineStateDesc, L"TiledLightCullingPass::m_pPipelineState");
}

void TiledLightCullingPass::AddResourceBarrierIfRequired(GraphicsResource* pResource, D3D12_RESOURCE_STATES currState, D3D12_RESOURCE_STATES requiredState)
{
	if (currState != requiredState)
		m_ResourceBarriers.emplace_back(pResource, currState, requiredState);
}
