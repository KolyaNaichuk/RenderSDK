#include "RenderPasses/RenderTiledShadowMapPass.h"
#include "RenderPasses/Common.h"
#include "Common/MeshRenderResources.h"
#include "D3DWrapper/CommandSignature.h"
#include "D3DWrapper/CommandList.h"
#include "D3DWrapper/GraphicsDevice.h"
#include "D3DWrapper/GraphicsUtils.h"
#include "D3DWrapper/PipelineState.h"
#include "D3DWrapper/Profiler.h"
#include "D3DWrapper/RenderEnv.h"
#include "D3DWrapper/RootSignature.h"

namespace
{
	enum RootParams
	{
		kRoot32BitConstantParamVS = 0,
		kRootSRVTableParamVS,
		kRootSRVTableParamGS,
		kNumRootParams
	};

	const u32 INVALID_INDEX = ~0u;
}

RenderTiledShadowMapPass::RenderTiledShadowMapPass(InitParams* pParams)
	: m_Name(pParams->m_pName)
{
	assert((pParams->m_LightType == LightType_Point) || (pParams->m_LightType == LightType_Spot));
	
	InitResources(pParams);
	InitRootSignature(pParams);
	InitPipelineState(pParams);
	InitCommandSignature(pParams);
}

RenderTiledShadowMapPass::~RenderTiledShadowMapPass()
{
	SafeDelete(m_pCommandSignature);
	SafeDelete(m_pRootSignature);
	SafeDelete(m_pPipelineState);
	SafeDelete(m_pShadowMap);
	SafeDelete(m_pViewport);
}

void RenderTiledShadowMapPass::Record(RenderParams* pParams)
{
	MeshRenderResources* pMeshRenderResources = pParams->m_pMeshRenderResources;
	assert(pMeshRenderResources->GetNumMeshTypes() == 1);
	const u32 meshType = 0;

	RenderEnv* pRenderEnv = pParams->m_pRenderEnv;
	CommandList* pCommandList = pParams->m_pCommandList;
	Profiler* pProfiler = pRenderEnv->m_pProfiler;

	pCommandList->Begin(m_pPipelineState);
#ifdef ENABLE_GPU_PROFILING
	u32 profileIndex = pProfiler->StartProfile(pCommandList, m_Name.c_str());
#endif // ENABLE_GPU_PROFILING

	pCommandList->SetGraphicsRootSignature(m_pRootSignature);
	pCommandList->SetDescriptorHeaps(pRenderEnv->m_pShaderVisibleSRVHeap);

	if (!m_ResourceBarriers.empty())
		pCommandList->ResourceBarrier((UINT)m_ResourceBarriers.size(), m_ResourceBarriers.data());

	D3D12_CPU_DESCRIPTOR_HANDLE dsvHeapStart = m_DSVHeapStart;
	pCommandList->OMSetRenderTargets(0, nullptr, TRUE, &dsvHeapStart);
	pCommandList->ClearDepthView(dsvHeapStart, 1.0f);

	pCommandList->SetGraphicsRootDescriptorTable(kRootSRVTableParamVS, m_SRVHeapStartVS);
	pCommandList->SetGraphicsRootDescriptorTable(kRootSRVTableParamGS, m_SRVHeapStartGS);
	
	pCommandList->IASetPrimitiveTopology(pMeshRenderResources->GetPrimitiveTopology(meshType));
	pCommandList->IASetVertexBuffers(0, 1, pMeshRenderResources->GetVertexBuffer(meshType)->GetVBView());
	pCommandList->IASetIndexBuffer(pMeshRenderResources->GetIndexBuffer(meshType)->GetIBView());

	Rect scissorRect(ExtractRect(m_pViewport));
	pCommandList->RSSetViewports(1, m_pViewport);
	pCommandList->RSSetScissorRects(1, &scissorRect);

	pCommandList->ExecuteIndirect(m_pCommandSignature, pMeshRenderResources->GetTotalNumMeshes(),
		pParams->m_pShadowMapCommandBuffer, 0,
		pParams->m_pNumShadowMapCommandsBuffer, 0);
	
#ifdef ENABLE_GPU_PROFILING
	pProfiler->EndProfile(pCommandList, profileIndex);
#endif // ENABLE_GPU_PROFILING
	pCommandList->End();
}

void RenderTiledShadowMapPass::InitResources(InitParams* pParams)
{
	RenderEnv* pRenderEnv = pParams->m_pRenderEnv;

	assert(m_pShadowMap == nullptr);
	DepthStencilValue optimizedClearDepth(1.0f);
	DepthTexture2DDesc shadowMapDesc(DXGI_FORMAT_R32_TYPELESS, pParams->m_ShadowMapSize, pParams->m_ShadowMapSize, true, true);
	m_pShadowMap = new DepthTexture(pRenderEnv, pRenderEnv->m_pDefaultHeapProps, &shadowMapDesc,
		pParams->m_InputResourceStates.m_TiledShadowMapState, &optimizedClearDepth, L"RenderTiledShadowMapPass::m_pShadowMap");

	m_OutputResourceStates.m_MeshInstanceWorldMatrixBufferState = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
	m_OutputResourceStates.m_LightIndexForMeshInstanceBufferState = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
	m_OutputResourceStates.m_MeshInstanceIndexForLightBufferState = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
	m_OutputResourceStates.m_LightWorldBoundsOrPropsBufferState = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
	m_OutputResourceStates.m_LightWorldFrustumBufferState = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
	m_OutputResourceStates.m_LightViewProjMatrixBufferState = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
	m_OutputResourceStates.m_TiledShadowMapState = D3D12_RESOURCE_STATE_DEPTH_WRITE;
	m_OutputResourceStates.m_NumShadowMapCommandsBufferState = D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT;
	m_OutputResourceStates.m_ShadowMapCommandBufferState = D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT;

	assert(m_ResourceBarriers.empty());
	AddResourceBarrierIfRequired(pParams->m_pMeshInstanceWorldMatrixBuffer,
		pParams->m_InputResourceStates.m_MeshInstanceWorldMatrixBufferState,
		m_OutputResourceStates.m_MeshInstanceWorldMatrixBufferState);

	AddResourceBarrierIfRequired(pParams->m_pLightIndexForMeshInstanceBuffer,
		pParams->m_InputResourceStates.m_LightIndexForMeshInstanceBufferState,
		m_OutputResourceStates.m_LightIndexForMeshInstanceBufferState);

	AddResourceBarrierIfRequired(pParams->m_pMeshInstanceIndexForLightBuffer,
		pParams->m_InputResourceStates.m_MeshInstanceIndexForLightBufferState,
		m_OutputResourceStates.m_MeshInstanceIndexForLightBufferState);

	AddResourceBarrierIfRequired(pParams->m_pLightWorldBoundsOrPropsBuffer,
		pParams->m_InputResourceStates.m_LightWorldBoundsOrPropsBufferState,
		m_OutputResourceStates.m_LightWorldBoundsOrPropsBufferState);

	AddResourceBarrierIfRequired(pParams->m_pLightWorldFrustumBuffer,
		pParams->m_InputResourceStates.m_LightWorldFrustumBufferState,
		m_OutputResourceStates.m_LightWorldFrustumBufferState);

	AddResourceBarrierIfRequired(pParams->m_pLightViewProjMatrixBuffer,
		pParams->m_InputResourceStates.m_LightViewProjMatrixBufferState,
		m_OutputResourceStates.m_LightViewProjMatrixBufferState);

	AddResourceBarrierIfRequired(pParams->m_pNumShadowMapCommandsBuffer,
		pParams->m_InputResourceStates.m_NumShadowMapCommandsBufferState,
		m_OutputResourceStates.m_NumShadowMapCommandsBufferState);

	AddResourceBarrierIfRequired(pParams->m_pShadowMapCommandBuffer,
		pParams->m_InputResourceStates.m_ShadowMapCommandBufferState,
		m_OutputResourceStates.m_ShadowMapCommandBufferState);

	AddResourceBarrierIfRequired(m_pShadowMap,
		pParams->m_InputResourceStates.m_TiledShadowMapState,
		m_OutputResourceStates.m_TiledShadowMapState);

	m_SRVHeapStartVS = pRenderEnv->m_pShaderVisibleSRVHeap->Allocate();
	pRenderEnv->m_pDevice->CopyDescriptor(m_SRVHeapStartVS,
		pParams->m_pMeshInstanceWorldMatrixBuffer->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	pRenderEnv->m_pDevice->CopyDescriptor(pRenderEnv->m_pShaderVisibleSRVHeap->Allocate(),
		pParams->m_pLightIndexForMeshInstanceBuffer->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	pRenderEnv->m_pDevice->CopyDescriptor(pRenderEnv->m_pShaderVisibleSRVHeap->Allocate(),
		pParams->m_pMeshInstanceIndexForLightBuffer->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	m_SRVHeapStartGS = pRenderEnv->m_pShaderVisibleSRVHeap->Allocate();
	pRenderEnv->m_pDevice->CopyDescriptor(m_SRVHeapStartGS,
		pParams->m_pLightWorldBoundsOrPropsBuffer->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	pRenderEnv->m_pDevice->CopyDescriptor(pRenderEnv->m_pShaderVisibleSRVHeap->Allocate(),
		pParams->m_pLightWorldFrustumBuffer->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	
	pRenderEnv->m_pDevice->CopyDescriptor(pRenderEnv->m_pShaderVisibleSRVHeap->Allocate(),
		pParams->m_pLightViewProjMatrixBuffer->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	m_DSVHeapStart = m_pShadowMap->GetDSVHandle();
}

void RenderTiledShadowMapPass::InitRootSignature(InitParams* pParams)
{
	assert(m_pRootSignature == nullptr);

	D3D12_ROOT_PARAMETER rootParams[kNumRootParams];
	rootParams[kRoot32BitConstantParamVS] = Root32BitConstantsParameter(0, D3D12_SHADER_VISIBILITY_VERTEX, 1);

	D3D12_DESCRIPTOR_RANGE descriptorRangesVS[] = {SRVDescriptorRange(3, 0)};
	rootParams[kRootSRVTableParamVS] = RootDescriptorTableParameter(ARRAYSIZE(descriptorRangesVS), descriptorRangesVS, D3D12_SHADER_VISIBILITY_VERTEX);

	D3D12_DESCRIPTOR_RANGE descriptorRangesGS[] = {SRVDescriptorRange(3, 0)};
	rootParams[kRootSRVTableParamGS] = RootDescriptorTableParameter(ARRAYSIZE(descriptorRangesGS), descriptorRangesGS, D3D12_SHADER_VISIBILITY_GEOMETRY);

	RootSignatureDesc rootSignatureDesc(kNumRootParams, rootParams, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
	m_pRootSignature = new RootSignature(pParams->m_pRenderEnv->m_pDevice, &rootSignatureDesc, L"RenderTiledShadowMapPass::m_pRootSignature");
}

void RenderTiledShadowMapPass::InitPipelineState(InitParams* pParams)
{
	assert(m_pPipelineState == nullptr);
	assert(m_pRootSignature != nullptr);

	RenderEnv* pRenderEnv = pParams->m_pRenderEnv;

	assert(m_pViewport == nullptr);
	m_pViewport = new Viewport(0.0f, 0.0f, FLOAT(pParams->m_ShadowMapSize), FLOAT(pParams->m_ShadowMapSize));
	
	const MeshRenderResources* pMeshRenderResources = pParams->m_pMeshRenderResources;
	assert(pMeshRenderResources->GetNumMeshTypes() == 1);
	const u32 meshType = 0;

	const InputLayoutDesc& inputLayout = pMeshRenderResources->GetInputLayout(meshType);
	assert(inputLayout.NumElements == 3);
	assert(HasVertexSemantic(inputLayout, "POSITION"));
	assert(HasVertexSemantic(inputLayout, "NORMAL"));
	assert(HasVertexSemantic(inputLayout, "TEXCOORD"));

	std::string lightTypeStr = std::to_string(pParams->m_LightType);
	const ShaderMacro shaderDefinesGS[] =
	{
		ShaderMacro("LIGHT_TYPE", lightTypeStr.c_str()),
		ShaderMacro()
	};

	Shader vertexShader(L"Shaders//RenderTiledShadowMapVS.hlsl", "Main", "vs_4_0");
	Shader geometryShader(L"Shaders//RenderTiledShadowMapGS.hlsl", "Main", "gs_4_0", shaderDefinesGS);
	
	GraphicsPipelineStateDesc pipelineStateDesc;
	pipelineStateDesc.SetRootSignature(m_pRootSignature);
	pipelineStateDesc.SetVertexShader(&vertexShader);
	pipelineStateDesc.SetGeometryShader(&geometryShader);
	pipelineStateDesc.InputLayout = inputLayout;
	pipelineStateDesc.PrimitiveTopologyType = pMeshRenderResources->GetPrimitiveTopologyType(meshType);
	pipelineStateDesc.DepthStencilState = DepthStencilDesc(DepthStencilDesc::Enabled);
	pipelineStateDesc.SetRenderTargetFormats(0, nullptr, GetDepthStencilViewFormat(m_pShadowMap->GetFormat()));

	m_pPipelineState = new PipelineState(pRenderEnv->m_pDevice, &pipelineStateDesc, L"RenderTiledShadowMapPass::m_pPipelineState");
}

void RenderTiledShadowMapPass::InitCommandSignature(InitParams* pParams)
{
	assert(m_pCommandSignature == nullptr);
	assert(m_pRootSignature != nullptr);

	RenderEnv* pRenderEnv = pParams->m_pRenderEnv;

	D3D12_INDIRECT_ARGUMENT_DESC argumentDescs[] =
	{
		Constant32BitArgument(kRoot32BitConstantParamVS, 0, 1),
		DrawIndexedArgument()
	};

	CommandSignatureDesc commandSignatureDesc(sizeof(ShadowMapCommand), ARRAYSIZE(argumentDescs), argumentDescs);
	m_pCommandSignature = new CommandSignature(pRenderEnv->m_pDevice, m_pRootSignature, &commandSignatureDesc, L"RenderTiledShadowMapPass::m_pCommandSignature");
}

void RenderTiledShadowMapPass::AddResourceBarrierIfRequired(GraphicsResource* pResource, D3D12_RESOURCE_STATES currState, D3D12_RESOURCE_STATES requiredState)
{
	if (currState != requiredState)
		m_ResourceBarriers.emplace_back(pResource, currState, requiredState);
}

ShadowMapTileAllocator::ShadowMapTileAllocator(u32 shadowMapSize, u32 numLevels)
{
	assert(IsPowerOf2(shadowMapSize));
	assert(numLevels > 0);

	const f32 shadowMapSizeF = f32(shadowMapSize);
	
	m_MaxTileSize = shadowMapSize;
	m_RcpMaxTileSize = Rcp(shadowMapSizeF);
	m_Log2MaxTileSize = Log2(shadowMapSizeF);
	m_MinTileSize = m_MaxTileSize >> (numLevels - 1);
	assert(m_MinTileSize > 0);
	m_NumNodes = ((2 << ((numLevels << 1) - 1)) - 1) / 3;

	m_pFreeNodes = new u8[m_NumNodes];
	m_pTiles = new ShadowMapTile[m_NumNodes];
	
	struct TileNode
	{
		TileNode(const Vector2f& center, f32 size)
			: m_Center(center)
			, m_Size(size)
		{}
		Vector2f m_Center;
		f32 m_Size;
	};

	std::queue<TileNode> queue;
	queue.emplace(Vector2f(0.5f * shadowMapSizeF), shadowMapSizeF);

	for (u32 nodeIndex = 0; true; queue.pop())
	{
		const TileNode& parentNode = queue.front();
		
		const f32 parentHalfSize = 0.5f * parentNode.m_Size;
		m_pTiles[nodeIndex].m_TexSpaceTopLeft = m_RcpMaxTileSize * (parentNode.m_Center - parentHalfSize);
		m_pTiles[nodeIndex].m_TexSpaceSize = m_RcpMaxTileSize * parentNode.m_Size;
		++nodeIndex;

		if (nodeIndex == m_NumNodes)
			break;

		const f32 childHalfSize = 0.25f * parentNode.m_Size;
		queue.emplace(parentNode.m_Center + Vector2f(-childHalfSize, -childHalfSize), parentHalfSize);
		queue.emplace(parentNode.m_Center + Vector2f( childHalfSize, -childHalfSize), parentHalfSize);
		queue.emplace(parentNode.m_Center + Vector2f(-childHalfSize,  childHalfSize), parentHalfSize);
		queue.emplace(parentNode.m_Center + Vector2f( childHalfSize,  childHalfSize), parentHalfSize);
	}
	Reset();
}

ShadowMapTileAllocator::~ShadowMapTileAllocator()
{
	SafeArrayDelete(m_pFreeNodes);
	SafeArrayDelete(m_pTiles);
}

bool ShadowMapTileAllocator::Allocate(u32 tileSize, ShadowMapTile* pNewTile)
{
	assert(pNewTile != nullptr);

	tileSize = Clamp(m_MinTileSize, m_MaxTileSize, tileSize);
	const u32 requiredLevel = CalcTreeLevel(tileSize);

	u32 rootLevel = 0;
	u32 rootIndex = 0;

	u32 foundIndex = FindFreeNode(rootLevel, rootIndex, requiredLevel);
	if (foundIndex != INVALID_INDEX)
	{
		m_pFreeNodes[foundIndex] = 0;

		pNewTile->m_TexSpaceTopLeft = m_pTiles[foundIndex].m_TexSpaceTopLeft;
		pNewTile->m_TexSpaceSize = f32(tileSize) * m_RcpMaxTileSize;

		return true;
	}
	return false;
}

void ShadowMapTileAllocator::Reset()
{
	std::memset(m_pFreeNodes, 1, m_NumNodes * sizeof(u8));
}

u32 ShadowMapTileAllocator::CalcTreeLevel(u32 tileSize) const
{
	return u32(m_Log2MaxTileSize - Ceil(Log2(f32(tileSize))));
}

u32 ShadowMapTileAllocator::FindFreeNode(u32 currentLevel, u32 currentIndex, u32 requiredLevel)
{
	if (m_pFreeNodes[currentIndex] == 1)
	{
		if (currentLevel == requiredLevel)
			return currentIndex;

		const u32 firstChildIndex = ((currentIndex << 2) + 1);
		const u32 lastChildIndex = firstChildIndex + 4;

		for (u32 childIndex = firstChildIndex; childIndex < lastChildIndex; ++childIndex)
		{
			u32 foundIndex = FindFreeNode(currentLevel + 1, childIndex, requiredLevel);
			if (foundIndex != INVALID_INDEX)
				return foundIndex;
		}
	}
	return INVALID_INDEX;
}