#include "RenderPasses/FrustumMeshCullingPass.h"
#include "D3DWrapper/CommandList.h"
#include "D3DWrapper/GraphicsDevice.h"
#include "D3DWrapper/GraphicsResource.h"
#include "D3DWrapper/PipelineState.h"
#include "D3DWrapper/RenderEnv.h"
#include "D3DWrapper/ResourceList.h"
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
	, m_pResourceList(new ResourceList())
	, m_pNumVisibleMeshesBuffer(nullptr)
	, m_pVisibleInstanceRangeBuffer(nullptr)
	, m_pVisibleInstanceIndexBuffer(nullptr)
	, m_pDrawVisibleInstanceCommandBuffer(nullptr)
	, m_TotalNumMeshes(pParams->m_TotalNumMeshes)
{
	InitResources(pParams);
	InitRootSignature(pParams);
	InitPipelineState(pParams);
}

FrustumMeshCullingPass::~FrustumMeshCullingPass()
{
	SafeDelete(m_pNumVisibleMeshesBuffer);
	SafeDelete(m_pVisibleInstanceRangeBuffer);
	SafeDelete(m_pVisibleInstanceIndexBuffer);
	SafeDelete(m_pDrawVisibleInstanceCommandBuffer);
	SafeDelete(m_pResourceList);
	SafeDelete(m_pPipelineState);
	SafeDelete(m_pRootSignature);
}

void FrustumMeshCullingPass::InitResources(InitParams* pParams)
{
	RenderEnv* pRenderEnv = pParams->m_pRenderEnv;

	FormattedBufferDesc numVisibleMeshesBufferDesc(1, DXGI_FORMAT_R32_UINT, false, true);
	m_pNumVisibleMeshesBuffer = new Buffer(pRenderEnv, pRenderEnv->m_pDefaultHeapProps, &numVisibleMeshesBufferDesc,
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS, L"FrustumMeshCullingPass::m_pNumVisibleMeshesBuffer");
	
	StructuredBufferDesc visibleInstanceRangeBufferDesc(pParams->m_TotalNumInstances, sizeof(MeshInstanceRange), true, true);
	m_pVisibleInstanceRangeBuffer = new Buffer(pRenderEnv, pRenderEnv->m_pDefaultHeapProps, &visibleInstanceRangeBufferDesc,
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS, L"FrustumMeshCullingPass::m_pVisibleInstanceRangeBuffer");

	FormattedBufferDesc visibleInstanceIndexBufferDesc(pParams->m_TotalNumInstances, DXGI_FORMAT_R32_UINT, true, true);
	m_pVisibleInstanceIndexBuffer = new Buffer(pRenderEnv, pRenderEnv->m_pDefaultHeapProps, &visibleInstanceIndexBufferDesc,
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS, L"FrustumMeshCullingPass::m_pVisibleInstanceIndexBuffer");

	StructuredBufferDesc drawVisibleInstanceCommandBufferDesc(1, sizeof(DrawIndexedArgs), true, true);
	m_pDrawVisibleInstanceCommandBuffer = new Buffer(pRenderEnv, pRenderEnv->m_pDefaultHeapProps, &drawVisibleInstanceCommandBufferDesc,
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS, L"FrustumMeshCullingPass::m_pDrawVisibleInstanceCommandBuffer");

	m_pResourceList->m_RequiredResourceStates.emplace_back(pParams->m_pMeshInstanceRangeBuffer, pParams->m_pMeshInstanceRangeBuffer->GetReadState());
	m_pResourceList->m_RequiredResourceStates.emplace_back(pParams->m_pInstanceAABBBuffer, pParams->m_pInstanceAABBBuffer->GetReadState());
	m_pResourceList->m_RequiredResourceStates.emplace_back(m_pNumVisibleMeshesBuffer, m_pNumVisibleMeshesBuffer->GetWriteState());
	m_pResourceList->m_RequiredResourceStates.emplace_back(m_pVisibleInstanceRangeBuffer, m_pVisibleInstanceRangeBuffer->GetWriteState());
	m_pResourceList->m_RequiredResourceStates.emplace_back(m_pVisibleInstanceIndexBuffer, m_pVisibleInstanceIndexBuffer->GetWriteState());
	m_pResourceList->m_RequiredResourceStates.emplace_back(m_pDrawVisibleInstanceCommandBuffer, m_pDrawVisibleInstanceCommandBuffer->GetWriteState());

	m_pResourceList->m_SRVHeapStart = pRenderEnv->m_pShaderVisibleSRVHeap->Allocate();	
	pRenderEnv->m_pDevice->CopyDescriptor(m_pResourceList->m_SRVHeapStart,
		pParams->m_pMeshInstanceRangeBuffer->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	
	pRenderEnv->m_pDevice->CopyDescriptor(pRenderEnv->m_pShaderVisibleSRVHeap->Allocate(),
		pParams->m_pInstanceAABBBuffer->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	
	pRenderEnv->m_pDevice->CopyDescriptor(pRenderEnv->m_pShaderVisibleSRVHeap->Allocate(),
		m_pNumVisibleMeshesBuffer->GetUAVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	
	pRenderEnv->m_pDevice->CopyDescriptor(pRenderEnv->m_pShaderVisibleSRVHeap->Allocate(),
		m_pVisibleInstanceRangeBuffer->GetUAVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	
	pRenderEnv->m_pDevice->CopyDescriptor(pRenderEnv->m_pShaderVisibleSRVHeap->Allocate(),
		m_pVisibleInstanceIndexBuffer->GetUAVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	
	pRenderEnv->m_pDevice->CopyDescriptor(pRenderEnv->m_pShaderVisibleSRVHeap->Allocate(),
		m_pDrawVisibleInstanceCommandBuffer->GetUAVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}

void FrustumMeshCullingPass::InitRootSignature(InitParams* pParams)
{
	D3D12_ROOT_PARAMETER rootParams[kNumRootParams];
	rootParams[kRootCBVParam] = RootCBVParameter(0, D3D12_SHADER_VISIBILITY_ALL);

	D3D12_DESCRIPTOR_RANGE descriptorRanges[] = {SRVDescriptorRange(2, 0), UAVDescriptorRange(4, 0)};
	rootParams[kRootSRVTableParam] = RootDescriptorTableParameter(ARRAYSIZE(descriptorRanges), &descriptorRanges[0], D3D12_SHADER_VISIBILITY_ALL);

	RootSignatureDesc rootSignatureDesc(kNumRootParams, rootParams);
	m_pRootSignature = new RootSignature(pParams->m_pRenderEnv->m_pDevice, &rootSignatureDesc, L"ClearVoxelGridPass::m_pRootSignature");
}

void FrustumMeshCullingPass::InitPipelineState(InitParams* pParams)
{
	assert(m_pRootSignature != nullptr);
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

void FrustumMeshCullingPass::Record(RenderParams* pParams)
{
	RenderEnv* pRenderEnv = pParams->m_pRenderEnv;
	CommandList* pCommandList = pParams->m_pCommandList;

	pCommandList->Begin(m_pPipelineState);
	pCommandList->SetComputeRootSignature(m_pRootSignature);

	pCommandList->SetRequiredResourceStates(&m_pResourceList->m_RequiredResourceStates);
	pCommandList->SetDescriptorHeaps(pRenderEnv->m_pShaderVisibleSRVHeap);
	pCommandList->SetComputeRootConstantBufferView(kRootCBVParam, pParams->m_pCullingDataBuffer);
	pCommandList->SetComputeRootDescriptorTable(kRootSRVTableParam, m_pResourceList->m_SRVHeapStart);

	pCommandList->Dispatch(m_TotalNumMeshes, 1, 1);
	pCommandList->End();
}
