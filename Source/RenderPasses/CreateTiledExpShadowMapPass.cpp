#include "RenderPasses/CreateTiledExpShadowMapPass.h"
#include "D3DWrapper/CommandList.h"
#include "D3DWrapper/GraphicsDevice.h"
#include "D3DWrapper/GraphicsUtils.h"
#include "D3DWrapper/PipelineState.h"
#include "D3DWrapper/RenderEnv.h"
#include "D3DWrapper/RootSignature.h"
#include "Profiler/GPUProfiler.h"

namespace
{
	enum RootParams
	{
		kRootSRVTableParamVS = 0,
		kRootSRVTableParamPS,
		kNumRootParams
	};
}

CreateTiledExpShadowMapPass::CreateTiledExpShadowMapPass(InitParams* pParams)
	: m_Name(pParams->m_pName)
{
	assert((pParams->m_LightType == LightType_Point) || (pParams->m_LightType == LightType_Spot));

	InitResources(pParams);
	InitRootSignature(pParams);
	InitPipelineState(pParams);
}

CreateTiledExpShadowMapPass::~CreateTiledExpShadowMapPass()
{
	SafeDelete(m_pRootSignature);
	SafeDelete(m_pPipelineState);
	SafeDelete(m_pTiledExpShadowMap);
	SafeDelete(m_pViewport);
}

void CreateTiledExpShadowMapPass::Record(RenderParams* pParams)
{
	RenderEnv* pRenderEnv = pParams->m_pRenderEnv;
	CommandList* pCommandList = pParams->m_pCommandList;
	GPUProfiler* pGPUProfiler = pRenderEnv->m_pGPUProfiler;

	pCommandList->Begin(m_pPipelineState);
#ifdef ENABLE_PROFILING
	u32 profileIndex = pGPUProfiler->StartProfile(pCommandList, m_Name.c_str());
#endif // ENABLE_PROFILING

	pCommandList->SetGraphicsRootSignature(m_pRootSignature);

	if (!m_ResourceBarriers.empty())
		pCommandList->ResourceBarrier((UINT)m_ResourceBarriers.size(), m_ResourceBarriers.data());

	pCommandList->SetDescriptorHeaps(pRenderEnv->m_pShaderVisibleSRVHeap);
	pCommandList->SetGraphicsRootDescriptorTable(kRootSRVTableParamVS, m_SRVHeapStartVS);
	pCommandList->SetGraphicsRootDescriptorTable(kRootSRVTableParamPS, m_SRVHeapStartPS);
	
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHeapStart = m_RTVHeapStart;
	pCommandList->OMSetRenderTargets(1, &rtvHeapStart);

	pCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	pCommandList->IASetVertexBuffers(0, 1, nullptr);
	pCommandList->IASetIndexBuffer(nullptr);

	Rect scissorRect(ExtractRect(m_pViewport));
	pCommandList->RSSetViewports(1, m_pViewport);
	pCommandList->RSSetScissorRects(1, &scissorRect);

	pCommandList->DrawInstanced(4, pParams->m_NumShadowMapTiles, 0, 0);

#ifdef ENABLE_PROFILING
	pGPUProfiler->EndProfile(pCommandList, profileIndex);
#endif // ENABLE_PROFILING
	pCommandList->End();
}

void CreateTiledExpShadowMapPass::InitResources(InitParams* pParams)
{
	RenderEnv* pRenderEnv = pParams->m_pRenderEnv;

	m_OutputResourceStates.m_TiledShadowMapState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	m_OutputResourceStates.m_ShadowMapTileBufferState = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
	m_OutputResourceStates.m_CreateExpShadowMapParamsBufferState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	m_OutputResourceStates.m_TiledExpShadowMapState = D3D12_RESOURCE_STATE_RENDER_TARGET;

	assert(m_pTiledExpShadowMap == nullptr);
	const FLOAT optimizedClearColor[] = {0.0f, 0.0f, 0.0f, 0.0f};
	ColorTexture2DDesc tiledExpShadowMapDesc(DXGI_FORMAT_R32G32_FLOAT, pParams->m_pTiledShadowMap->GetWidth(),
		pParams->m_pTiledShadowMap->GetHeight(), true, true, false);
	m_pTiledExpShadowMap = new ColorTexture(pRenderEnv, pRenderEnv->m_pDefaultHeapProps, &tiledExpShadowMapDesc,
		pParams->m_InputResourceStates.m_TiledExpShadowMapState, optimizedClearColor, L"CreateTiledExpShadowMapPass::m_pTiledExpShadowMap");

	assert(m_ResourceBarriers.empty());
	AddResourceBarrierIfRequired(pParams->m_pTiledShadowMap,
		pParams->m_InputResourceStates.m_TiledShadowMapState,
		m_OutputResourceStates.m_TiledShadowMapState);

	AddResourceBarrierIfRequired(pParams->m_pShadowMapTileBuffer,
		pParams->m_InputResourceStates.m_ShadowMapTileBufferState,
		m_OutputResourceStates.m_ShadowMapTileBufferState);

	AddResourceBarrierIfRequired(pParams->m_pCreateExpShadowMapParamsBuffer,
		pParams->m_InputResourceStates.m_CreateExpShadowMapParamsBufferState,
		m_OutputResourceStates.m_CreateExpShadowMapParamsBufferState);

	AddResourceBarrierIfRequired(m_pTiledExpShadowMap,
		pParams->m_InputResourceStates.m_TiledExpShadowMapState,
		m_OutputResourceStates.m_TiledExpShadowMapState);

	m_SRVHeapStartVS = pRenderEnv->m_pShaderVisibleSRVHeap->Allocate();
	pRenderEnv->m_pDevice->CopyDescriptor(m_SRVHeapStartVS,
		pParams->m_pShadowMapTileBuffer->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	m_SRVHeapStartPS = pRenderEnv->m_pShaderVisibleSRVHeap->Allocate();
	pRenderEnv->m_pDevice->CopyDescriptor(m_SRVHeapStartPS,
		pParams->m_pTiledShadowMap->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	pRenderEnv->m_pDevice->CopyDescriptor(pRenderEnv->m_pShaderVisibleSRVHeap->Allocate(),
		pParams->m_pCreateExpShadowMapParamsBuffer->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	m_RTVHeapStart = m_pTiledExpShadowMap->GetRTVHandle();
}

void CreateTiledExpShadowMapPass::InitRootSignature(InitParams* pParams)
{
	assert(m_pRootSignature == nullptr);
	RenderEnv* pRenderEnv = pParams->m_pRenderEnv;

	D3D12_ROOT_PARAMETER rootParams[kNumRootParams];
	
	D3D12_DESCRIPTOR_RANGE descriptorRangesVS[] = {SRVDescriptorRange(1, 0)};
	rootParams[kRootSRVTableParamVS] = RootDescriptorTableParameter(ARRAYSIZE(descriptorRangesVS), descriptorRangesVS, D3D12_SHADER_VISIBILITY_VERTEX);

	D3D12_DESCRIPTOR_RANGE descriptorRangesPS[] = {SRVDescriptorRange(2, 0)};
	rootParams[kRootSRVTableParamPS] = RootDescriptorTableParameter(ARRAYSIZE(descriptorRangesPS), descriptorRangesPS, D3D12_SHADER_VISIBILITY_PIXEL);

	RootSignatureDesc rootSignatureDesc(kNumRootParams, rootParams);
	m_pRootSignature = new RootSignature(pParams->m_pRenderEnv->m_pDevice, &rootSignatureDesc, L"CreateTiledExpShadowMapPass::m_pRootSignature");
}

void CreateTiledExpShadowMapPass::InitPipelineState(InitParams* pParams)
{
	assert(m_pRootSignature != nullptr);
	assert(m_pPipelineState == nullptr);

	assert(m_pViewport == nullptr);
	m_pViewport = new Viewport(0.0f, 0.0f, FLOAT(m_pTiledExpShadowMap->GetWidth()), FLOAT(m_pTiledExpShadowMap->GetHeight()));

	RenderEnv* pRenderEnv = pParams->m_pRenderEnv;
	std::string lightTypeStr = std::to_string(pParams->m_LightType);
	
	const ShaderMacro shaderDefines[] =
	{
		ShaderMacro("LIGHT_TYPE", lightTypeStr.c_str()),
		ShaderMacro()
	};

	Shader vertexShader(L"Shaders//CreateTiledExpShadowMapVS.hlsl", "Main", "vs_4_0");
	Shader pixelShader(L"Shaders//CreateTiledExpShadowMapPS.hlsl", "Main", "ps_4_0", shaderDefines);

	GraphicsPipelineStateDesc pipelineStateDesc;
	pipelineStateDesc.SetRootSignature(m_pRootSignature);
	pipelineStateDesc.SetVertexShader(&vertexShader);
	pipelineStateDesc.SetPixelShader(&pixelShader);
	pipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	pipelineStateDesc.DepthStencilState = DepthStencilDesc(DepthStencilDesc::Disabled);
	pipelineStateDesc.SetRenderTargetFormat(GetRenderTargetViewFormat(m_pTiledExpShadowMap->GetFormat()));

	m_pPipelineState = new PipelineState(pParams->m_pRenderEnv->m_pDevice, &pipelineStateDesc, L"CreateTiledExpShadowMapPass::m_pPipelineState");
}

void CreateTiledExpShadowMapPass::AddResourceBarrierIfRequired(GraphicsResource* pResource, D3D12_RESOURCE_STATES currState, D3D12_RESOURCE_STATES requiredState)
{
	if (currState != requiredState)
		m_ResourceBarriers.emplace_back(pResource, currState, requiredState);
}