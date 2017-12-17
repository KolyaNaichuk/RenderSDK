#include "RenderPasses/FrustumMeshCullingPass.h"
#include "D3DWrapper/CommandList.h"
#include "D3DWrapper/GraphicsDevice.h"
#include "D3DWrapper/PipelineState.h"
#include "D3DWrapper/RenderEnv.h"
#include "D3DWrapper/RootSignature.h"
#include "Common/MeshRenderResources.h"

namespace
{
	enum RootParams
	{
		kRootCBVParam = 0,
		kRootSRVTableParam,
		kNumRootParams
	};
}

FrustumMeshCullingPass::FrustumMeshCullingPass(InitParams* pParams)
	: m_pRootSignature(nullptr)
	, m_pPipelineState(nullptr)
	, m_pNumVisibleMeshesBuffer(nullptr)
	, m_pNumVisibleInstancesBuffer(nullptr)
	, m_pVisibleMeshInfoBuffer(nullptr)
	, m_pVisibleInstanceIndexBuffer(nullptr)
	, m_MaxNumMeshes(pParams->m_MaxNumMeshes)
{
	InitResources(pParams);
	InitRootSignature(pParams);
	InitPipelineState(pParams);
}

FrustumMeshCullingPass::~FrustumMeshCullingPass()
{
	SafeDelete(m_pNumVisibleMeshesBuffer);
	SafeDelete(m_pNumVisibleInstancesBuffer);
	SafeDelete(m_pVisibleMeshInfoBuffer);
	SafeDelete(m_pVisibleInstanceIndexBuffer);
	SafeDelete(m_pPipelineState);
	SafeDelete(m_pRootSignature);
}

void FrustumMeshCullingPass::InitResources(InitParams* pParams)
{
	RenderEnv* pRenderEnv = pParams->m_pRenderEnv;

	assert(m_pNumVisibleMeshesBuffer == nullptr);
	FormattedBufferDesc numVisibleMeshesBufferDesc(1, DXGI_FORMAT_R32_UINT, true, true);
	m_pNumVisibleMeshesBuffer = new Buffer(pRenderEnv, pRenderEnv->m_pDefaultHeapProps, &numVisibleMeshesBufferDesc,
		pParams->m_InputResourceStates.m_NumVisibleMeshesBufferState, L"FrustumMeshCullingPass::m_pNumVisibleMeshesBuffer");
	
	assert(m_pNumVisibleInstancesBuffer == nullptr);
	FormattedBufferDesc numVisibleInstancesBufferDesc(1, DXGI_FORMAT_R32_UINT, false, true);
	m_pNumVisibleInstancesBuffer = new Buffer(pRenderEnv, pRenderEnv->m_pDefaultHeapProps, &numVisibleInstancesBufferDesc,
		pParams->m_InputResourceStates.m_NumVisibleInstancesBufferState, L"FrustumMeshCullingPass::m_pNumVisibleInstancesBuffer");

	assert(m_pVisibleMeshInfoBuffer == nullptr);
	StructuredBufferDesc visibleMeshInfoBufferDesc(pParams->m_MaxNumMeshes, sizeof(MeshRenderInfo), true, true);
	m_pVisibleMeshInfoBuffer = new Buffer(pRenderEnv, pRenderEnv->m_pDefaultHeapProps, &visibleMeshInfoBufferDesc,
		pParams->m_InputResourceStates.m_VisibleMeshInfoBufferState, L"FrustumMeshCullingPass::m_pVisibleMeshInfoBuffer");

	assert(m_pVisibleInstanceIndexBuffer == nullptr);
	FormattedBufferDesc visibleInstanceIndexBufferDesc(pParams->m_MaxNumInstances, DXGI_FORMAT_R32_UINT, true, true);
	m_pVisibleInstanceIndexBuffer = new Buffer(pRenderEnv, pRenderEnv->m_pDefaultHeapProps, &visibleInstanceIndexBufferDesc,
		pParams->m_InputResourceStates.m_VisibleInstanceIndexBufferState, L"FrustumMeshCullingPass::m_pVisibleInstanceIndexBuffer");
	
	m_OutputResourceStates.m_MeshInfoBufferState = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
	m_OutputResourceStates.m_InstanceWorldAABBBufferState = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
	m_OutputResourceStates.m_NumVisibleMeshesBufferState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
	m_OutputResourceStates.m_VisibleMeshInfoBufferState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
	m_OutputResourceStates.m_NumVisibleInstancesBufferState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
	m_OutputResourceStates.m_VisibleInstanceIndexBufferState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;

	assert(m_ResourceTransitionBarriers.empty());
	AddResourceTransitionBarrierIfRequired(pParams->m_pMeshInfoBuffer,
		pParams->m_InputResourceStates.m_MeshInfoBufferState,
		m_OutputResourceStates.m_MeshInfoBufferState);

	AddResourceTransitionBarrierIfRequired(pParams->m_pInstanceWorldAABBBuffer,
		pParams->m_InputResourceStates.m_InstanceWorldAABBBufferState,
		m_OutputResourceStates.m_InstanceWorldAABBBufferState);

	AddResourceTransitionBarrierIfRequired(m_pNumVisibleMeshesBuffer,
		pParams->m_InputResourceStates.m_NumVisibleMeshesBufferState,
		m_OutputResourceStates.m_NumVisibleMeshesBufferState);

	AddResourceTransitionBarrierIfRequired(m_pVisibleMeshInfoBuffer,
		pParams->m_InputResourceStates.m_VisibleMeshInfoBufferState,
		m_OutputResourceStates.m_VisibleMeshInfoBufferState);

	AddResourceTransitionBarrierIfRequired(m_pNumVisibleInstancesBuffer,
		pParams->m_InputResourceStates.m_NumVisibleInstancesBufferState,
		m_OutputResourceStates.m_NumVisibleInstancesBufferState);

	AddResourceTransitionBarrierIfRequired(m_pVisibleInstanceIndexBuffer,
		pParams->m_InputResourceStates.m_VisibleInstanceIndexBufferState,
		m_OutputResourceStates.m_VisibleInstanceIndexBufferState);

	m_SRVHeapStart = pRenderEnv->m_pShaderVisibleSRVHeap->Allocate();
	pRenderEnv->m_pDevice->CopyDescriptor(m_SRVHeapStart,
		pParams->m_pMeshInfoBuffer->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
			
	pRenderEnv->m_pDevice->CopyDescriptor(pRenderEnv->m_pShaderVisibleSRVHeap->Allocate(),
		pParams->m_pInstanceWorldAABBBuffer->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
			
	pRenderEnv->m_pDevice->CopyDescriptor(pRenderEnv->m_pShaderVisibleSRVHeap->Allocate(),
		m_pNumVisibleMeshesBuffer->GetUAVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	pRenderEnv->m_pDevice->CopyDescriptor(pRenderEnv->m_pShaderVisibleSRVHeap->Allocate(),
		m_pNumVisibleInstancesBuffer->GetUAVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	
	pRenderEnv->m_pDevice->CopyDescriptor(pRenderEnv->m_pShaderVisibleSRVHeap->Allocate(),
		m_pVisibleMeshInfoBuffer->GetUAVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
			
	pRenderEnv->m_pDevice->CopyDescriptor(pRenderEnv->m_pShaderVisibleSRVHeap->Allocate(),
		m_pVisibleInstanceIndexBuffer->GetUAVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}

void FrustumMeshCullingPass::InitRootSignature(InitParams* pParams)
{
	assert(m_pRootSignature == nullptr);

	D3D12_ROOT_PARAMETER rootParams[kNumRootParams];
	rootParams[kRootCBVParam] = RootCBVParameter(0, D3D12_SHADER_VISIBILITY_ALL);

	D3D12_DESCRIPTOR_RANGE descriptorRanges[] = {SRVDescriptorRange(2, 0), UAVDescriptorRange(4, 0)};
	rootParams[kRootSRVTableParam] = RootDescriptorTableParameter(ARRAYSIZE(descriptorRanges), &descriptorRanges[0], D3D12_SHADER_VISIBILITY_ALL);

	RootSignatureDesc rootSignatureDesc(kNumRootParams, rootParams);
	m_pRootSignature = new RootSignature(pParams->m_pRenderEnv->m_pDevice, &rootSignatureDesc, L"FrustumMeshCullingPass::m_pRootSignature");
}

void FrustumMeshCullingPass::InitPipelineState(InitParams* pParams)
{
	assert(m_pRootSignature != nullptr);
	assert(m_pPipelineState == nullptr);

	RenderEnv* pRenderEnv = pParams->m_pRenderEnv;

	const u8 numThreadsPerMesh = 64;
	std::string numThreadsPerMeshStr = std::to_string(numThreadsPerMesh);
	std::string maxNumInstancesPerMeshStr = std::to_string(pParams->m_MaxNumInstancesPerMesh);

	const ShaderMacro shaderDefines[] =
	{
		ShaderMacro("NUM_THREADS_PER_MESH", numThreadsPerMeshStr.c_str()),
		ShaderMacro("MAX_NUM_INSTANCES_PER_MESH", maxNumInstancesPerMeshStr.c_str()),
		ShaderMacro()
	};
	Shader computeShader(L"Shaders//FrustumMeshCullingCS.hlsl", "Main", "cs_5_0", shaderDefines);

	ComputePipelineStateDesc pipelineStateDesc;
	pipelineStateDesc.SetRootSignature(m_pRootSignature);
	pipelineStateDesc.SetComputeShader(&computeShader);

	m_pPipelineState = new PipelineState(pRenderEnv->m_pDevice, &pipelineStateDesc, L"FrustumMeshCullingPass::m_pPipelineState");
}

void FrustumMeshCullingPass::AddResourceTransitionBarrierIfRequired(GraphicsResource* pResource, D3D12_RESOURCE_STATES currState, D3D12_RESOURCE_STATES requiredState)
{
	if (currState != requiredState)
		m_ResourceTransitionBarriers.emplace_back(pResource, currState, requiredState);
}

void FrustumMeshCullingPass::Record(RenderParams* pParams)
{
	RenderEnv* pRenderEnv = pParams->m_pRenderEnv;
	CommandList* pCommandList = pParams->m_pCommandList;

	pCommandList->Begin(m_pPipelineState);
	pCommandList->SetComputeRootSignature(m_pRootSignature);

	if (!m_ResourceTransitionBarriers.empty())
		pCommandList->ResourceBarrier((UINT)m_ResourceTransitionBarriers.size(), m_ResourceTransitionBarriers.data());
		
	const UINT clearValues[4] = {0, 0, 0, 0};
	pCommandList->ClearUnorderedAccessView(DescriptorHandle(m_SRVHeapStart, 2),
		m_pNumVisibleMeshesBuffer->GetUAVHandle(), m_pNumVisibleMeshesBuffer, clearValues);
	
	pCommandList->ClearUnorderedAccessView(DescriptorHandle(m_SRVHeapStart, 3),
		m_pNumVisibleInstancesBuffer->GetUAVHandle(), m_pNumVisibleInstancesBuffer, clearValues);

	pCommandList->SetDescriptorHeaps(pRenderEnv->m_pShaderVisibleSRVHeap);
	pCommandList->SetComputeRootConstantBufferView(kRootCBVParam, pParams->m_pAppDataBuffer);
	pCommandList->SetComputeRootDescriptorTable(kRootSRVTableParam, m_SRVHeapStart);
	pCommandList->Dispatch(m_MaxNumMeshes, 1, 1);
	pCommandList->End();
}
