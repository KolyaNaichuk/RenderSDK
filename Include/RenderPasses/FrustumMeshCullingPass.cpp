#include "RenderPasses/FrustumMeshCullingPass.h"
#include "D3DWrapper/CommandList.h"
#include "D3DWrapper/GraphicsDevice.h"
#include "D3DWrapper/PipelineState.h"
#include "D3DWrapper/RenderEnv.h"
#include "D3DWrapper/RootSignature.h"

namespace
{
	enum RootParams
	{
		kRootCBVParam = 0,
		kRootSRVTableParam,
		kNumRootParams
	};

	// Kolya. Duplicating structures. Could be moved to separate file
	struct MeshInstanceRange
	{
		MeshInstanceRange(u32 instanceOffset, u32 numInstances, u32 meshIndex)
			: m_InstanceOffset(instanceOffset)
			, m_NumInstances(numInstances)
			, m_MeshIndex(meshIndex)
		{
		}
		u32 m_InstanceOffset;
		u32 m_NumInstances;
		u32 m_MeshIndex;
	};

	struct DrawIndexedArgs
	{
		DrawIndexedArgs(u32 indexCountPerInstance, u32 instanceCount, u32 startIndexLocation, i32 baseVertexLocation, u32 startInstanceLocation)
			: m_IndexCountPerInstance(indexCountPerInstance)
			, m_InstanceCount(instanceCount)
			, m_StartIndexLocation(startIndexLocation)
			, m_BaseVertexLocation(baseVertexLocation)
			, m_StartInstanceLocation(startInstanceLocation)
		{}
		u32 m_IndexCountPerInstance;
		u32 m_InstanceCount;
		u32 m_StartIndexLocation;
		i32 m_BaseVertexLocation;
		u32 m_StartInstanceLocation;
	};
}

FrustumMeshCullingPass::FrustumMeshCullingPass(InitParams* pParams)
	: m_pRootSignature(nullptr)
	, m_pPipelineState(nullptr)
	, m_pNumVisibleMeshesBuffer(nullptr)
	, m_pNumVisibleInstancesBuffer(nullptr)
	, m_pVisibleInstanceRangeBuffer(nullptr)
	, m_pVisibleInstanceIndexBuffer(nullptr)
	, m_TotalNumMeshes(pParams->m_TotalNumMeshes)
{
	InitResources(pParams);
	InitRootSignature(pParams);
	InitPipelineState(pParams);
}

FrustumMeshCullingPass::~FrustumMeshCullingPass()
{
	SafeDelete(m_pNumVisibleMeshesBuffer);
	SafeDelete(m_pNumVisibleInstancesBuffer);
	SafeDelete(m_pVisibleInstanceRangeBuffer);
	SafeDelete(m_pVisibleInstanceIndexBuffer);
	SafeDelete(m_pPipelineState);
	SafeDelete(m_pRootSignature);
}

void FrustumMeshCullingPass::InitResources(InitParams* pParams)
{
	assert(m_ResourceBarriers.empty());
	RenderEnv* pRenderEnv = pParams->m_pRenderEnv;

	FormattedBufferDesc numVisibleMeshesBufferDesc(1, DXGI_FORMAT_R32_UINT, false, true);
	m_pNumVisibleMeshesBuffer = new Buffer(pRenderEnv, pRenderEnv->m_pDefaultHeapProps, &numVisibleMeshesBufferDesc,
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS, L"FrustumMeshCullingPass::m_pNumVisibleMeshesBuffer");
	
	FormattedBufferDesc numVisibleInstancesBufferDesc(1, DXGI_FORMAT_R32_UINT, false, true);
	m_pNumVisibleInstancesBuffer = new Buffer(pRenderEnv, pRenderEnv->m_pDefaultHeapProps, &numVisibleInstancesBufferDesc,
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS, L"FrustumMeshCullingPass::m_pNumVisibleInstancesBuffer");

	StructuredBufferDesc visibleInstanceRangeBufferDesc(pParams->m_TotalNumInstances, sizeof(MeshInstanceRange), true, true);
	m_pVisibleInstanceRangeBuffer = new Buffer(pRenderEnv, pRenderEnv->m_pDefaultHeapProps, &visibleInstanceRangeBufferDesc,
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS, L"FrustumMeshCullingPass::m_pVisibleInstanceRangeBuffer");

	FormattedBufferDesc visibleInstanceIndexBufferDesc(pParams->m_TotalNumInstances, DXGI_FORMAT_R32_UINT, true, true);
	m_pVisibleInstanceIndexBuffer = new Buffer(pRenderEnv, pRenderEnv->m_pDefaultHeapProps, &visibleInstanceIndexBufferDesc,
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS, L"FrustumMeshCullingPass::m_pVisibleInstanceIndexBuffer");
	
	m_OutputResourceStates.m_MeshInstanceRangeBufferState = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
	m_OutputResourceStates.m_InstanceWorldAABBBufferState = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
	m_OutputResourceStates.m_VisibleInstanceRangeBufferState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
	m_OutputResourceStates.m_VisibleInstanceIndexBufferState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
	m_OutputResourceStates.m_NumVisibleInstancesBufferState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
	
	CreateResourceBarrierIfRequired(pParams->m_pMeshInstanceRangeBuffer,
		pParams->m_InputResourceStates.m_MeshInstanceRangeBufferState,
		m_OutputResourceStates.m_MeshInstanceRangeBufferState);

	CreateResourceBarrierIfRequired(pParams->m_pInstanceWorldAABBBuffer,
		pParams->m_InputResourceStates.m_InstanceWorldAABBBufferState,
		m_OutputResourceStates.m_InstanceWorldAABBBufferState);

	CreateResourceBarrierIfRequired(m_pVisibleInstanceRangeBuffer,
		pParams->m_InputResourceStates.m_VisibleInstanceRangeBufferState,
		m_OutputResourceStates.m_VisibleInstanceRangeBufferState);

	CreateResourceBarrierIfRequired(m_pVisibleInstanceIndexBuffer,
		pParams->m_InputResourceStates.m_VisibleInstanceIndexBufferState,
		m_OutputResourceStates.m_VisibleInstanceIndexBufferState);

	CreateResourceBarrierIfRequired(m_pNumVisibleInstancesBuffer,
		pParams->m_InputResourceStates.m_NumVisibleInstancesBufferState,
		m_OutputResourceStates.m_NumVisibleInstancesBufferState);

	m_SRVHeapStart = pRenderEnv->m_pShaderVisibleSRVHeap->Allocate();
	pRenderEnv->m_pDevice->CopyDescriptor(m_SRVHeapStart,
		pParams->m_pMeshInstanceRangeBuffer->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
			
	pRenderEnv->m_pDevice->CopyDescriptor(pRenderEnv->m_pShaderVisibleSRVHeap->Allocate(),
		pParams->m_pInstanceWorldAABBBuffer->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
			
	pRenderEnv->m_pDevice->CopyDescriptor(pRenderEnv->m_pShaderVisibleSRVHeap->Allocate(),
		m_pNumVisibleMeshesBuffer->GetUAVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	pRenderEnv->m_pDevice->CopyDescriptor(pRenderEnv->m_pShaderVisibleSRVHeap->Allocate(),
		m_pNumVisibleInstancesBuffer->GetUAVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	
	pRenderEnv->m_pDevice->CopyDescriptor(pRenderEnv->m_pShaderVisibleSRVHeap->Allocate(),
		m_pVisibleInstanceRangeBuffer->GetUAVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
			
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

void FrustumMeshCullingPass::CreateResourceBarrierIfRequired(GraphicsResource* pResource, D3D12_RESOURCE_STATES currState, D3D12_RESOURCE_STATES requiredState)
{
	if (currState != requiredState)
		m_ResourceBarriers.emplace_back(pResource, currState, requiredState);
}

void FrustumMeshCullingPass::Record(RenderParams* pParams)
{
	RenderEnv* pRenderEnv = pParams->m_pRenderEnv;
	CommandList* pCommandList = pParams->m_pCommandList;

	pCommandList->Begin(m_pPipelineState);
	pCommandList->SetComputeRootSignature(m_pRootSignature);

	if (!m_ResourceBarriers.empty())
		pCommandList->ResourceBarrier(m_ResourceBarriers.size(), m_ResourceBarriers.data());
		
	const UINT clearValues[4] = {0, 0, 0, 0};
	DescriptorHandle numVisibleMeshesBufferHandle(m_SRVHeapStart, 2);
	pCommandList->ClearUnorderedAccessView(numVisibleMeshesBufferHandle, m_pNumVisibleMeshesBuffer->GetUAVHandle(), m_pNumVisibleMeshesBuffer, clearValues);
	
	DescriptorHandle numVisibleInstancesBufferHandle(m_SRVHeapStart, 3);
	pCommandList->ClearUnorderedAccessView(numVisibleInstancesBufferHandle, m_pNumVisibleInstancesBuffer->GetUAVHandle(), m_pNumVisibleInstancesBuffer, clearValues);

	pCommandList->SetDescriptorHeaps(pRenderEnv->m_pShaderVisibleSRVHeap);
	pCommandList->SetComputeRootConstantBufferView(kRootCBVParam, pParams->m_pCameraDataBuffer);
	pCommandList->SetComputeRootDescriptorTable(kRootSRVTableParam, m_SRVHeapStart);
	pCommandList->Dispatch(m_TotalNumMeshes, 1, 1);
	pCommandList->End();
}
