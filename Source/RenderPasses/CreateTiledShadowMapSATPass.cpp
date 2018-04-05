#include "RenderPasses/CreateTiledShadowMapSATPass.h"
#include "D3DWrapper/CommandList.h"
#include "D3DWrapper/CommandSignature.h"
#include "D3DWrapper/GraphicsDevice.h"
#include "D3DWrapper/GPUProfiler.h"
#include "D3DWrapper/PipelineState.h"
#include "D3DWrapper/RenderEnv.h"
#include "D3DWrapper/RootSignature.h"
#include "Math/Vector2.h"

namespace
{
	struct CreateSATCommand
	{
		Vector2u m_TileTopLeftInPixels;
		DispatchArgument m_Args;
	};

	enum RootParams
	{
		kRoot32BitConstantsParam = 0,
		kRootSRVTableParam,
		kNumRootParams
	};
}

CreateTiledShadowMapSATPass::CreateTiledShadowMapSATPass(InitParams* pParams)
	: m_Name(pParams->m_pName)
{
	InitResources(pParams);
	InitRootSignature(pParams);
	InitPipelineStates(pParams);
	InitCommandSignature(pParams);
}

CreateTiledShadowMapSATPass::~CreateTiledShadowMapSATPass()
{	
	for (PipelineStatePermutation& permutation : m_PipelineStatePermutations)
		SafeDelete(permutation.m_pPipelineState);

	SafeDelete(m_pRootSignature);
	SafeDelete(m_pCommandSignature);

	SafeDelete(m_pTiledShadowMapSATRow);
	SafeDelete(m_pTiledShadowMapSATColumn);

	SafeDelete(m_pArgumentBuffer);

	m_pUploadArgumentBuffer->Unmap(0, nullptr);
	SafeDelete(m_pUploadArgumentBuffer);
}

void CreateTiledShadowMapSATPass::Record(RenderParams* pParams)
{
	struct ExecuteIndirectParams
	{
		UINT m_NumCommands;
		UINT64 m_FirstCommandOffset;
		PipelineState* m_pPipelineState;
	};
	
	CreateSATCommand* pCommands = (CreateSATCommand*)m_pUploadArgumentBufferMem;
	assert(false);

	RenderEnv* pRenderEnv = pParams->m_pRenderEnv;
	CommandList* pCommandList = pParams->m_pCommandList;
	GPUProfiler* pGPUProfiler = pRenderEnv->m_pGPUProfiler;
		
	pCommandList->Begin();
#ifdef ENABLE_PROFILING
	u32 profileIndex = pGPUProfiler->StartProfile(pCommandList, m_Name.c_str());
#endif // ENABLE_PROFILING

	pCommandList->SetComputeRootSignature(m_pRootSignature);
	pCommandList->SetDescriptorHeaps(pRenderEnv->m_pShaderVisibleSRVHeap);
	
	if (!m_ResourceBarriersRow.empty())
		pCommandList->ResourceBarrier((UINT)m_ResourceBarriersRow.size(), m_ResourceBarriersRow.data());
	pCommandList->SetComputeRootDescriptorTable(kRootSRVTableParam, m_SRVHeapStartRow);
	
	assert(false);
	//pCommandList->SetPipelineState();
	//pCommandList->ExecuteIndirect();
	
	if (!m_ResourceBarriersColumn.empty())
		pCommandList->ResourceBarrier((UINT)m_ResourceBarriersColumn.size(), m_ResourceBarriersColumn.data());
	pCommandList->SetComputeRootDescriptorTable(kRootSRVTableParam, m_SRVHeapStartColumn);

	assert(false);
	//pCommandList->SetPipelineState();
	//pCommandList->ExecuteIndirect();

#ifdef ENABLE_PROFILING
	pGPUProfiler->EndProfile(pCommandList, profileIndex);
#endif // ENABLE_PROFILING
	pCommandList->End();
}

void CreateTiledShadowMapSATPass::InitResources(InitParams* pParams)
{
	RenderEnv* pRenderEnv = pParams->m_pRenderEnv;

	m_OutputResourceStates.m_TiledShadowMapState = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
	m_OutputResourceStates.m_TiledShadowMapSATState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;

	assert(m_pTiledShadowMapSATRow == nullptr);
	assert(m_pTiledShadowMapSATColumn == nullptr);
	
	const FLOAT optimizedClearColor[] = {0.0f, 0.0f, 0.0f, 0.0f};

	ColorTexture2DDesc tiledShadowMapSATDesc(DXGI_FORMAT_R32G32_FLOAT, pParams->m_pTiledShadowMap->GetWidth(),
		pParams->m_pTiledShadowMap->GetHeight(), false, true, true);

	m_pTiledShadowMapSATRow = new ColorTexture(pRenderEnv, pRenderEnv->m_pDefaultHeapProps, &tiledShadowMapSATDesc,
		D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, optimizedClearColor, L"CreateTiledShadowMapSATPass::m_pTiledShadowMapSATRow");

	m_pTiledShadowMapSATColumn = new ColorTexture(pRenderEnv, pRenderEnv->m_pDefaultHeapProps, &tiledShadowMapSATDesc,
		pParams->m_InputResourceStates.m_TiledShadowMapSATState, optimizedClearColor, L"CreateTiledShadowMapSATPass::m_pTiledShadowMapSATColumn");
	
	assert(m_ResourceBarriersRow.empty());	
	
	if (pParams->m_InputResourceStates.m_TiledShadowMapState != m_OutputResourceStates.m_TiledShadowMapState)
		m_ResourceBarriersRow.emplace_back(pParams->m_pTiledShadowMap,
			pParams->m_InputResourceStates.m_TiledShadowMapState,
			m_OutputResourceStates.m_TiledShadowMapState);
	
	m_ResourceBarriersRow.emplace_back(m_pTiledShadowMapSATRow,
		D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

	assert(m_ResourceBarriersColumn.empty());

	if (pParams->m_InputResourceStates.m_TiledShadowMapSATState != m_OutputResourceStates.m_TiledShadowMapSATState)
		m_ResourceBarriersColumn.emplace_back(m_pTiledShadowMapSATColumn,
			pParams->m_InputResourceStates.m_TiledShadowMapSATState,
			m_OutputResourceStates.m_TiledShadowMapSATState);

	m_ResourceBarriersColumn.emplace_back(m_pTiledShadowMapSATRow,
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
		D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

	assert(!m_SRVHeapStartRow.IsValid());
	
	m_SRVHeapStartRow = pRenderEnv->m_pShaderVisibleSRVHeap->Allocate();
	pRenderEnv->m_pDevice->CopyDescriptor(m_SRVHeapStartRow,
		pParams->m_pTiledShadowMap->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	pRenderEnv->m_pDevice->CopyDescriptor(pRenderEnv->m_pShaderVisibleSRVHeap->Allocate(),
		m_pTiledShadowMapSATRow->GetUAVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	assert(!m_SRVHeapStartColumn.IsValid());

	m_SRVHeapStartColumn = pRenderEnv->m_pShaderVisibleSRVHeap->Allocate();
	pRenderEnv->m_pDevice->CopyDescriptor(m_SRVHeapStartColumn,
		m_pTiledShadowMapSATRow->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	pRenderEnv->m_pDevice->CopyDescriptor(pRenderEnv->m_pShaderVisibleSRVHeap->Allocate(),
		m_pTiledShadowMapSATColumn->GetUAVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
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
	
	RootSignatureDesc rootSignatureDesc(kNumRootParams, rootParams);
	m_pRootSignature = new RootSignature(pParams->m_pRenderEnv->m_pDevice, &rootSignatureDesc, L"CreateTiledShadowMapSATPass::m_pRootSignature");
}

void CreateTiledShadowMapSATPass::InitPipelineStates(InitParams* pParams)
{
	assert(m_pRootSignature != nullptr);
	assert(m_PipelineStatePermutations.empty());

	assert(IsPowerOf2(pParams->m_MinTileSize));
	assert(IsPowerOf2(pParams->m_MaxTileSize));
	assert(pParams->m_MinTileSize <= pParams->m_MaxTileSize);

	RenderEnv* pRenderEnv = pParams->m_pRenderEnv;

	for (u32 tileSize = pParams->m_MaxTileSize; pParams->m_MinTileSize <= tileSize; tileSize /= 2)
	{
		std::string tileSizeStr = std::to_string(tileSize);
		const ShaderMacro shaderDefines[] =
		{
			ShaderMacro("SHADOW_MAP_TILE_SIZE", tileSizeStr.c_str()),
			ShaderMacro()
		};
		Shader computeShader(L"Shaders//CreateTiledShadowMapSATCS.hlsl", "Main", "cs_5_0", shaderDefines);

		ComputePipelineStateDesc pipelineStateDesc;
		pipelineStateDesc.SetRootSignature(m_pRootSignature);
		pipelineStateDesc.SetComputeShader(&computeShader);

		m_PipelineStatePermutations.emplace_back(tileSize, new PipelineState(pRenderEnv->m_pDevice, &pipelineStateDesc, L"CreateTiledShadowMapSATPass::m_pPipelineState"));
	}
}

void CreateTiledShadowMapSATPass::InitCommandSignature(InitParams* pParams)
{
	assert(m_pCommandSignature == nullptr);
	assert(m_pRootSignature != nullptr);

	RenderEnv* pRenderEnv = pParams->m_pRenderEnv;

	D3D12_INDIRECT_ARGUMENT_DESC argumentDescs[] =
	{
		Constant32BitArgument(kRoot32BitConstantsParam, 0, 2),
		DispatchArgument()
	};
	CommandSignatureDesc commandSignatureDesc(sizeof(CreateSATCommand), ARRAYSIZE(argumentDescs), argumentDescs);
	m_pCommandSignature = new CommandSignature(pRenderEnv->m_pDevice, m_pRootSignature, &commandSignatureDesc, L"CreateTiledShadowMapSATPass::m_pCommandSignature");
}
