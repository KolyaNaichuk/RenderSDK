#include "RenderPasses/FrustumLightCullingPass.h"
#include "D3DWrapper/CommandList.h"
#include "D3DWrapper/GraphicsDevice.h"
#include "D3DWrapper/GraphicsResource.h"
#include "D3DWrapper/PipelineState.h"
#include "D3DWrapper/RenderEnv.h"
#include "D3DWrapper/RootSignature.h"
#include "Math/Math.h"

namespace
{
	enum RootParams
	{
		kRootCBVParam = 0,
		kRootSRVTableParam,
		kNumRootParams
	};
}

FrustumLightCullingPass::FrustumLightCullingPass(InitParams* pParams)
	: m_pRootSignature(nullptr)
	, m_pPipelineState(nullptr)
	, m_pNumVisibleLightsBuffer(nullptr)
	, m_pVisibleLightIndexBuffer(nullptr)
{
	InitResources(pParams);
	InitRootSignature(pParams);
	InitPipelineState(pParams);
}

FrustumLightCullingPass::~FrustumLightCullingPass()
{
	SafeDelete(m_pPipelineState);
	SafeDelete(m_pRootSignature);
	SafeDelete(m_pNumVisibleLightsBuffer);
	SafeDelete(m_pVisibleLightIndexBuffer);
}

void FrustumLightCullingPass::Record(RenderParams* pParams)
{
	RenderEnv* pRenderEnv = pParams->m_pRenderEnv;
	CommandList* pCommandList = pParams->m_pCommandList;
	
	pCommandList->Begin(m_pPipelineState);
	pCommandList->SetComputeRootSignature(m_pRootSignature);
	pCommandList->SetDescriptorHeaps(pRenderEnv->m_pShaderVisibleSRVHeap);

	if (!m_ResourceTransitionBarriers.empty())
		pCommandList->ResourceBarrier((UINT)m_ResourceTransitionBarriers.size(), m_ResourceTransitionBarriers.data());
		
	pCommandList->SetComputeRootConstantBufferView(kRootCBVParam, pParams->m_pAppDataBuffer);
	pCommandList->SetComputeRootDescriptorTable(kRootSRVTableParam, m_SRVHeapStart);

	const UINT clearValue[4] = {0, 0, 0, 0};
	pCommandList->ClearUnorderedAccessView(m_SRVHeapStart, m_pNumVisibleLightsBuffer->GetUAVHandle(), m_pNumVisibleLightsBuffer, clearValue);

	pCommandList->Dispatch(m_NumThreadGroupsX, 1, 1);
	pCommandList->End();
}

void FrustumLightCullingPass::InitResources(InitParams* pParams)
{
	RenderEnv* pRenderEnv = pParams->m_pRenderEnv;

	assert(m_pNumVisibleLightsBuffer == nullptr);
	FormattedBufferDesc numVisibleLightsBufferDesc(1, DXGI_FORMAT_R32_UINT, true, true);
	m_pNumVisibleLightsBuffer = new Buffer(pRenderEnv, pRenderEnv->m_pDefaultHeapProps, &numVisibleLightsBufferDesc,
		pParams->m_InputResourceStates.m_NumVisibleLightsBufferState, L"m_pNumVisibleLightsBuffer");

	assert(m_pVisibleLightIndexBuffer == nullptr);
	FormattedBufferDesc visibleLightIndexBufferDesc(pParams->m_NumTotalLights, DXGI_FORMAT_R32_UINT, true, true);
	m_pVisibleLightIndexBuffer = new Buffer(pRenderEnv, pRenderEnv->m_pDefaultHeapProps, &visibleLightIndexBufferDesc,
		pParams->m_InputResourceStates.m_VisibleLightIndexBufferState, L"m_pVisibleLightIndexBuffer");
	
	m_OutputResourceStates.m_LightWorldBoundsBufferState = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
	m_OutputResourceStates.m_NumVisibleLightsBufferState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
	m_OutputResourceStates.m_VisibleLightIndexBufferState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
		
	assert(m_ResourceTransitionBarriers.empty());
	AddResourceTransitionBarrierIfRequired(pParams->m_pLightWorldBoundsBuffer,
		pParams->m_InputResourceStates.m_LightWorldBoundsBufferState,
		m_OutputResourceStates.m_LightWorldBoundsBufferState);

	AddResourceTransitionBarrierIfRequired(m_pNumVisibleLightsBuffer,
		pParams->m_InputResourceStates.m_NumVisibleLightsBufferState,
		m_OutputResourceStates.m_NumVisibleLightsBufferState);

	AddResourceTransitionBarrierIfRequired(m_pVisibleLightIndexBuffer,
		pParams->m_InputResourceStates.m_VisibleLightIndexBufferState,
		m_OutputResourceStates.m_VisibleLightIndexBufferState);

	m_SRVHeapStart = pRenderEnv->m_pShaderVisibleSRVHeap->Allocate();
	pRenderEnv->m_pDevice->CopyDescriptor(m_SRVHeapStart,
		m_pNumVisibleLightsBuffer->GetUAVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	pRenderEnv->m_pDevice->CopyDescriptor(pRenderEnv->m_pShaderVisibleSRVHeap->Allocate(),
		m_pVisibleLightIndexBuffer->GetUAVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	pRenderEnv->m_pDevice->CopyDescriptor(pRenderEnv->m_pShaderVisibleSRVHeap->Allocate(),
		pParams->m_pLightWorldBoundsBuffer->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}

void FrustumLightCullingPass::InitRootSignature(InitParams* pParams)
{
	assert(m_pRootSignature == nullptr);

	D3D12_ROOT_PARAMETER rootParams[kNumRootParams];
	rootParams[kRootCBVParam] = RootCBVParameter(0, D3D12_SHADER_VISIBILITY_ALL);
			
	D3D12_DESCRIPTOR_RANGE descriptorRanges[] = {UAVDescriptorRange(2, 0), SRVDescriptorRange(1, 0)};
	rootParams[kRootSRVTableParam] = RootDescriptorTableParameter(ARRAYSIZE(descriptorRanges), descriptorRanges, D3D12_SHADER_VISIBILITY_ALL);

	RootSignatureDesc rootSignatureDesc(kNumRootParams, rootParams);
	m_pRootSignature = new RootSignature(pParams->m_pRenderEnv->m_pDevice, &rootSignatureDesc, L"FrustumLightCullingPass::m_pRootSignature");
}

void FrustumLightCullingPass::InitPipelineState(InitParams* pParams)
{
	assert(m_pRootSignature != nullptr);
	assert(m_pPipelineState == nullptr);

	RenderEnv* pRenderEnv = pParams->m_pRenderEnv;

	const u16 threadGroupSize = 64;
	m_NumThreadGroupsX = (u16)Ceil((f32)pParams->m_NumTotalLights / (f32)threadGroupSize);

	std::string threadGroupSizeStr = std::to_string(threadGroupSize);
	std::string numTotalLightsStr = std::to_string(pParams->m_NumTotalLights);

	const ShaderMacro shaderDefines[] =
	{
		ShaderMacro("THREAD_GROUP_SIZE", threadGroupSizeStr.c_str()),
		ShaderMacro("NUM_TOTAL_LIGHTS", numTotalLightsStr.c_str()),
		ShaderMacro()
	};
	Shader computeShader(L"Shaders//FrustumLightCullingCS.hlsl", "Main", "cs_5_0", shaderDefines);

	ComputePipelineStateDesc pipelineStateDesc;
	pipelineStateDesc.SetRootSignature(m_pRootSignature);
	pipelineStateDesc.SetComputeShader(&computeShader);

	m_pPipelineState = new PipelineState(pRenderEnv->m_pDevice, &pipelineStateDesc, L"FrustumLightCullingPass::m_pPipelineState");
}

void FrustumLightCullingPass::AddResourceTransitionBarrierIfRequired(GraphicsResource* pResource, D3D12_RESOURCE_STATES currState, D3D12_RESOURCE_STATES requiredState)
{
	if (currState != requiredState)
		m_ResourceTransitionBarriers.emplace_back(pResource, currState, requiredState);
}
