#include "RenderPasses/CreateFalseNegativeDrawCommandsPass.h"
#include "RenderPasses/Common.h"
#include "D3DWrapper/CommandList.h"
#include "D3DWrapper/CommandSignature.h"
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
		kRootSRVTableParam = 0,
		kNumRootParams
	};
}

CreateFalseNegativeDrawCommandsPass::CreateFalseNegativeDrawCommandsPass(InitParams* pParams)
	: m_Name(pParams->m_pName)
{
	InitResources(pParams);
	InitRootSignature(pParams);
	InitPipelineState(pParams);
	InitCommandSignature(pParams);
}

CreateFalseNegativeDrawCommandsPass::~CreateFalseNegativeDrawCommandsPass()
{
	SafeDelete(m_pVisibleInstanceIndexBuffer);
	SafeDelete(m_pNumVisibleMeshesPerTypeBuffer);
	SafeDelete(m_pDrawCommandBuffer);
	SafeDelete(m_pArgumentBuffer);
	SafeDelete(m_pCommandSignature);
	SafeDelete(m_pPipelineState);
	SafeDelete(m_pRootSignature);
}

void CreateFalseNegativeDrawCommandsPass::Record(RenderParams* pParams)
{
	RenderEnv* pRenderEnv = pParams->m_pRenderEnv;
	CommandList* pCommandList = pParams->m_pCommandList;
	GPUProfiler* pGPUProfiler = pRenderEnv->m_pGPUProfiler;

	pCommandList->Begin(m_pPipelineState);
#ifdef ENABLE_PROFILING
	u32 profileIndex = pGPUProfiler->StartProfile(pCommandList, m_Name.c_str());
#endif // ENABLE_PROFILING

	pCommandList->SetComputeRootSignature(m_pRootSignature);
	pCommandList->SetDescriptorHeaps(pRenderEnv->m_pShaderVisibleSRVHeap);
	pCommandList->SetComputeRootDescriptorTable(kRootSRVTableParam, m_SRVHeapStart);

	pCommandList->ResourceBarrier((UINT)m_ResourceBarriers.size(), m_ResourceBarriers.data());
	pCommandList->CopyBufferRegion(m_pArgumentBuffer, 0, pParams->m_pNumMeshesBuffer, 0, sizeof(UINT));

	ResourceTransitionBarrier argumentBarrier(m_pArgumentBuffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
	pCommandList->ResourceBarrier(1, &argumentBarrier);

	const UINT clearValues[] = {0, 0, 0, 0};
	pCommandList->ClearUnorderedAccessView(DescriptorHandle(m_SRVHeapStart, 1),
		m_pNumVisibleMeshesPerTypeBuffer->GetUAVHandle(), m_pNumVisibleMeshesPerTypeBuffer, clearValues);
	
	pCommandList->ExecuteIndirect(m_pCommandSignature, 1, m_pArgumentBuffer, 0, nullptr, 0);

#ifdef ENABLE_PROFILING
	pGPUProfiler->EndProfile(pCommandList, profileIndex);
#endif // ENABLE_PROFILING
	pCommandList->End();
}

void CreateFalseNegativeDrawCommandsPass::InitResources(InitParams* pParams)
{
	RenderEnv* pRenderEnv = pParams->m_pRenderEnv;

	assert(m_pVisibleInstanceIndexBuffer == nullptr);
	FormattedBufferDesc visibleInstanceIndexBufferDesc(pParams->m_MaxNumInstances, DXGI_FORMAT_R32_UINT, true, true);
	m_pVisibleInstanceIndexBuffer = new Buffer(pRenderEnv, pRenderEnv->m_pDefaultHeapProps, &visibleInstanceIndexBufferDesc,
		pParams->m_InputResourceStates.m_VisibleInstanceIndexBufferState, L"CreateFalseNegativeDrawCommandsPass::m_pVisibleInstanceIndexBuffer");

	assert(m_pNumVisibleMeshesPerTypeBuffer == nullptr);
	FormattedBufferDesc numVisibleMeshesPerTypeBufferDesc(pParams->m_NumMeshTypes, DXGI_FORMAT_R32_UINT, false, true);
	m_pNumVisibleMeshesPerTypeBuffer = new Buffer(pRenderEnv, pRenderEnv->m_pDefaultHeapProps, &numVisibleMeshesPerTypeBufferDesc,
		pParams->m_InputResourceStates.m_NumVisibleMeshesPerTypeBufferState, L"CreateFalseNegativeDrawCommandsPass::m_pNumVisibleMeshesPerTypeBuffer");

	assert(m_pDrawCommandBuffer == nullptr);
	StructuredBufferDesc drawCommandBuffer(pParams->m_MaxNumMeshes, sizeof(DrawCommand), false, true);
	m_pDrawCommandBuffer = new Buffer(pRenderEnv, pRenderEnv->m_pDefaultHeapProps, &drawCommandBuffer,
		pParams->m_InputResourceStates.m_DrawCommandBufferState, L"CreateFalseNegativeDrawCommandsPass::m_pDrawCommandBuffer");
		
	assert(m_pArgumentBuffer == nullptr);
	DispatchArguments argumentBufferData(0, 1, 1);
	StructuredBufferDesc argumentBufferDesc(1, sizeof(argumentBufferData), false, true);
	m_pArgumentBuffer = new Buffer(pRenderEnv, pRenderEnv->m_pDefaultHeapProps, &argumentBufferDesc,
		D3D12_RESOURCE_STATE_COPY_DEST, L"CreateFalseNegativeDrawCommandsPass::m_pArgumentBuffer");
	UploadData(pRenderEnv, m_pArgumentBuffer, argumentBufferDesc,
		D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT, &argumentBufferData, sizeof(argumentBufferData));

	m_OutputResourceStates.m_NumMeshesBufferState = D3D12_RESOURCE_STATE_COPY_SOURCE;
	m_OutputResourceStates.m_VisibilityBufferState = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
	m_OutputResourceStates.m_InstanceIndexBufferState = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
	m_OutputResourceStates.m_MeshInfoBufferState = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
	m_OutputResourceStates.m_VisibleInstanceIndexBufferState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
	m_OutputResourceStates.m_NumVisibleMeshesPerTypeBufferState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
	m_OutputResourceStates.m_DrawCommandBufferState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;

	assert(m_ResourceBarriers.empty());
	AddResourceBarrierIfRequired(pParams->m_pNumMeshesBuffer,
		pParams->m_InputResourceStates.m_NumMeshesBufferState,
		m_OutputResourceStates.m_NumMeshesBufferState);

	AddResourceBarrierIfRequired(pParams->m_pVisibilityBuffer,
		pParams->m_InputResourceStates.m_VisibilityBufferState,
		m_OutputResourceStates.m_VisibilityBufferState);

	AddResourceBarrierIfRequired(pParams->m_pInstanceIndexBuffer,
		pParams->m_InputResourceStates.m_InstanceIndexBufferState,
		m_OutputResourceStates.m_InstanceIndexBufferState);

	AddResourceBarrierIfRequired(pParams->m_pMeshInfoBuffer,
		pParams->m_InputResourceStates.m_MeshInfoBufferState,
		m_OutputResourceStates.m_MeshInfoBufferState);

	AddResourceBarrierIfRequired(m_pVisibleInstanceIndexBuffer,
		pParams->m_InputResourceStates.m_VisibleInstanceIndexBufferState,
		m_OutputResourceStates.m_VisibleInstanceIndexBufferState);

	AddResourceBarrierIfRequired(m_pNumVisibleMeshesPerTypeBuffer,
		pParams->m_InputResourceStates.m_NumVisibleMeshesPerTypeBufferState,
		m_OutputResourceStates.m_NumVisibleMeshesPerTypeBufferState);

	AddResourceBarrierIfRequired(m_pDrawCommandBuffer,
		pParams->m_InputResourceStates.m_DrawCommandBufferState,
		m_OutputResourceStates.m_DrawCommandBufferState);
		
	AddResourceBarrierIfRequired(m_pArgumentBuffer,
		D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT,
		D3D12_RESOURCE_STATE_COPY_DEST);

	m_SRVHeapStart = pRenderEnv->m_pShaderVisibleSRVHeap->Allocate();
	pRenderEnv->m_pDevice->CopyDescriptor(m_SRVHeapStart,
		m_pVisibleInstanceIndexBuffer->GetUAVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	pRenderEnv->m_pDevice->CopyDescriptor(pRenderEnv->m_pShaderVisibleSRVHeap->Allocate(),
		m_pNumVisibleMeshesPerTypeBuffer->GetUAVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	pRenderEnv->m_pDevice->CopyDescriptor(pRenderEnv->m_pShaderVisibleSRVHeap->Allocate(),
		m_pDrawCommandBuffer->GetUAVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	pRenderEnv->m_pDevice->CopyDescriptor(pRenderEnv->m_pShaderVisibleSRVHeap->Allocate(),
		pParams->m_pVisibilityBuffer->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	pRenderEnv->m_pDevice->CopyDescriptor(pRenderEnv->m_pShaderVisibleSRVHeap->Allocate(),
		pParams->m_pInstanceIndexBuffer->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	pRenderEnv->m_pDevice->CopyDescriptor(pRenderEnv->m_pShaderVisibleSRVHeap->Allocate(),
		pParams->m_pMeshInfoBuffer->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}

void CreateFalseNegativeDrawCommandsPass::InitRootSignature(InitParams* pParams)
{
	assert(m_pRootSignature == nullptr);
	D3D12_ROOT_PARAMETER rootParams[kNumRootParams];

	D3D12_DESCRIPTOR_RANGE descriptorRanges[] = {UAVDescriptorRange(3, 0), SRVDescriptorRange(3, 0)};
	rootParams[kRootSRVTableParam] = RootDescriptorTableParameter(ARRAYSIZE(descriptorRanges), descriptorRanges, D3D12_SHADER_VISIBILITY_ALL);

	RootSignatureDesc rootSignatureDesc(kNumRootParams, rootParams);
	m_pRootSignature = new RootSignature(pParams->m_pRenderEnv->m_pDevice, &rootSignatureDesc, L"CreateFalseNegativeDrawCommandsPass::m_pRootSignature");
}

void CreateFalseNegativeDrawCommandsPass::InitPipelineState(InitParams* pParams)
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
	Shader computeShader(L"Shaders//CreateFalseNegativeDrawCommandsCS.hlsl", "Main", "cs_5_0", shaderDefines);

	ComputePipelineStateDesc pipelineStateDesc;
	pipelineStateDesc.SetRootSignature(m_pRootSignature);
	pipelineStateDesc.SetComputeShader(&computeShader);

	m_pPipelineState = new PipelineState(pRenderEnv->m_pDevice, &pipelineStateDesc, L"CreateFalseNegativeDrawCommandsPass::m_pPipelineState");
}

void CreateFalseNegativeDrawCommandsPass::InitCommandSignature(InitParams* pParams)
{
	assert(m_pCommandSignature == nullptr);
	RenderEnv* pRenderEnv = pParams->m_pRenderEnv;

	D3D12_INDIRECT_ARGUMENT_DESC argumentDescs[] =
	{
		DispatchArgument()
	};
	CommandSignatureDesc commandSignatureDesc(sizeof(DispatchArguments), ARRAYSIZE(argumentDescs), argumentDescs);
	m_pCommandSignature = new CommandSignature(pRenderEnv->m_pDevice, nullptr, &commandSignatureDesc, L"CreateFalseNegativeDrawCommandsPass::m_pCommandSignature");
}

void CreateFalseNegativeDrawCommandsPass::AddResourceBarrierIfRequired(GraphicsResource * pResource, D3D12_RESOURCE_STATES currState, D3D12_RESOURCE_STATES requiredState)
{
	if (currState != requiredState)
		m_ResourceBarriers.emplace_back(pResource, currState, requiredState);
}