#include "RenderPasses/CreateVoxelizeCommandsPass.h"
#include "RenderPasses/Common.h"
#include "D3DWrapper/CommandList.h"
#include "D3DWrapper/CommandSignature.h"
#include "D3DWrapper/GraphicsDevice.h"
#include "D3DWrapper/GraphicsResource.h"
#include "D3DWrapper/PipelineState.h"
#include "D3DWrapper/RenderEnv.h"
#include "D3DWrapper/RootSignature.h"
#include "Profiler/GPUProfiler.h"

namespace
{
	enum SetArgumentsRootParams
	{
		kSetArgumentsRootSRVTableParam = 0,
		kSetArgumentsNumRootParams
	};
	enum CreateCommandsRootParams
	{
		kCreateCommandsRootSRVTableParam = 0,
		kCreateCommandsNumRootParams
	};

	const u8 NUM_THREADS_PER_GROUP = 64;
}

CreateVoxelizeCommandsPass::CreateVoxelizeCommandsPass(InitParams* pParams)
	: m_Name(pParams->m_pName)
{
	InitSetArgumentsResources(pParams);
	InitSetArgumentsRootSignature(pParams);
	InitSetArgumentsPipelineState(pParams);

	InitCreateCommandsResources(pParams);
	InitCreateCommandsRootSignature(pParams);
	InitCreateCommandsPipelineState(pParams);
	InitCreateCommandsCommandSignature(pParams);
}

CreateVoxelizeCommandsPass::~CreateVoxelizeCommandsPass()
{
	SafeDelete(m_pArgumentBuffer);
	SafeDelete(m_pNumCommandsPerMeshTypeBuffer);
	SafeDelete(m_pVoxelizeCommandBuffer);
	
	SafeDelete(m_pCreateCommandsPipelineState);
	SafeDelete(m_pCreateCommandsRootSignature);
	SafeDelete(m_pCreateCommandsCommandSignature);

	SafeDelete(m_pSetArgumentsPipelineState);
	SafeDelete(m_pSetArgumentsRootSignature);
}

void CreateVoxelizeCommandsPass::Record(RenderParams* pParams)
{
	RenderEnv* pRenderEnv = pParams->m_pRenderEnv;
	CommandList* pCommandList = pParams->m_pCommandList;
	GPUProfiler* pGPUProfiler = pRenderEnv->m_pGPUProfiler;

	pCommandList->Begin();
#ifdef ENABLE_PROFILING
	u32 profileIndex = pGPUProfiler->StartProfile(pCommandList, m_Name.c_str());
#endif // ENABLE_PROFILING

	pCommandList->SetDescriptorHeaps(pRenderEnv->m_pShaderVisibleSRVHeap);
	{
		pCommandList->SetPipelineState(m_pSetArgumentsPipelineState);
		pCommandList->SetComputeRootSignature(m_pSetArgumentsRootSignature);

		if (!m_SetArgumentsResourceBarriers.empty())
			pCommandList->ResourceBarrier((UINT)m_SetArgumentsResourceBarriers.size(), m_SetArgumentsResourceBarriers.data());

		pCommandList->SetComputeRootDescriptorTable(kSetArgumentsRootSRVTableParam, m_SetArgumentsSRVHeapStart);
		pCommandList->Dispatch(1, 1, 1);
	}
	{
		pCommandList->SetPipelineState(m_pCreateCommandsPipelineState);
		pCommandList->SetComputeRootSignature(m_pCreateCommandsRootSignature);

		if (!m_CreateCommandsResourceBarriers.empty())
			pCommandList->ResourceBarrier((UINT)m_CreateCommandsResourceBarriers.size(), m_CreateCommandsResourceBarriers.data());

		const UINT clearValues[4] = {0, 0, 0, 0};
		pCommandList->ClearUnorderedAccessView(DescriptorHandle(m_CreateCommandsSRVHeapStart, 0),
			m_pNumCommandsPerMeshTypeBuffer->GetUAVHandle(), m_pNumCommandsPerMeshTypeBuffer, clearValues);

		pCommandList->SetComputeRootDescriptorTable(kCreateCommandsRootSRVTableParam, m_CreateCommandsSRVHeapStart);
		pCommandList->ExecuteIndirect(m_pCreateCommandsCommandSignature, 1, m_pArgumentBuffer, 0, nullptr, 0);
	}

#ifdef ENABLE_PROFILING
	pGPUProfiler->EndProfile(pCommandList, profileIndex);
#endif // ENABLE_PROFILING
	pCommandList->End();
}

void CreateVoxelizeCommandsPass::InitSetArgumentsResources(InitParams* pParams)
{
	RenderEnv* pRenderEnv = pParams->m_pRenderEnv;

	assert(m_pArgumentBuffer == nullptr);
	StructuredBufferDesc argumentBufferDesc(1, sizeof(DispatchArguments), false, true);
	m_pArgumentBuffer = new Buffer(pRenderEnv, pRenderEnv->m_pDefaultHeapProps, &argumentBufferDesc,
		D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT, L"CreateVoxelizeCommandsPass::m_pArgumentBuffer");

	assert(m_SetArgumentsResourceBarriers.empty());
	AddSetArgumentsResourceBarrierIfRequired(m_pArgumentBuffer,
		D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT,
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

	AddSetArgumentsResourceBarrierIfRequired(pParams->m_pNumMeshesBuffer,
		pParams->m_InputResourceStates.m_NumMeshesBufferState,
		D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

	m_SetArgumentsSRVHeapStart = pRenderEnv->m_pShaderVisibleSRVHeap->Allocate();
	pRenderEnv->m_pDevice->CopyDescriptor(m_SetArgumentsSRVHeapStart,
		pParams->m_pNumMeshesBuffer->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	pRenderEnv->m_pDevice->CopyDescriptor(pRenderEnv->m_pShaderVisibleSRVHeap->Allocate(),
		m_pArgumentBuffer->GetUAVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}

void CreateVoxelizeCommandsPass::InitSetArgumentsRootSignature(InitParams* pParams)
{
	assert(m_pSetArgumentsRootSignature == nullptr);

	RenderEnv* pRenderEnv = pParams->m_pRenderEnv;
	D3D12_ROOT_PARAMETER rootParams[kSetArgumentsNumRootParams];

	D3D12_DESCRIPTOR_RANGE descriptorRanges[] = {SRVDescriptorRange(1, 0), UAVDescriptorRange(1, 0)};
	rootParams[kSetArgumentsRootSRVTableParam] = RootDescriptorTableParameter(ARRAYSIZE(descriptorRanges), descriptorRanges, D3D12_SHADER_VISIBILITY_ALL);

	RootSignatureDesc rootSignatureDesc(kSetArgumentsNumRootParams, rootParams);
	m_pSetArgumentsRootSignature = new RootSignature(pRenderEnv->m_pDevice, &rootSignatureDesc, L"CreateVoxelizeCommandsPass::m_pSetArgumentsRootSignature");
}

void CreateVoxelizeCommandsPass::InitSetArgumentsPipelineState(InitParams* pParams)
{
	assert(m_pSetArgumentsRootSignature != nullptr);
	assert(m_pSetArgumentsPipelineState == nullptr);

	RenderEnv* pRenderEnv = pParams->m_pRenderEnv;

	std::string numThreadsPerGroupStr = std::to_string(NUM_THREADS_PER_GROUP);
	const ShaderMacro shaderDefines[] =
	{
		ShaderMacro("SET_ARGUMENTS", "1"),
		ShaderMacro("NUM_THREADS_PER_GROUP", numThreadsPerGroupStr.c_str()),
		ShaderMacro()
	};
	Shader computeShader(L"Shaders//CreateVoxelizeCommandsCS.hlsl", "Main", "cs_5_0", shaderDefines);

	ComputePipelineStateDesc pipelineStateDesc;
	pipelineStateDesc.SetRootSignature(m_pSetArgumentsRootSignature);
	pipelineStateDesc.SetComputeShader(&computeShader);

	m_pSetArgumentsPipelineState = new PipelineState(pRenderEnv->m_pDevice, &pipelineStateDesc, L"CreateVoxelizeCommandsPass::m_pSetArgumentsPipelineState");
}

void CreateVoxelizeCommandsPass::AddSetArgumentsResourceBarrierIfRequired(GraphicsResource* pResource, D3D12_RESOURCE_STATES currState, D3D12_RESOURCE_STATES requiredState)
{
	if (currState != requiredState)
		m_SetArgumentsResourceBarriers.emplace_back(pResource, currState, requiredState);
}

void CreateVoxelizeCommandsPass::InitCreateCommandsResources(InitParams* pParams)
{
	RenderEnv* pRenderEnv = pParams->m_pRenderEnv;

	assert(m_pVoxelizeCommandBuffer == nullptr);
	StructuredBufferDesc voxelizeCommandBufferDesc(pParams->m_MaxNumMeshes, sizeof(DrawCommand), false, true);
	m_pVoxelizeCommandBuffer = new Buffer(pRenderEnv, pRenderEnv->m_pDefaultHeapProps, &voxelizeCommandBufferDesc,
		pParams->m_InputResourceStates.m_VoxelizeCommandBufferState, L"CreateVoxelizeCommandsPass::m_pVoxelizeCommandBuffer");

	assert(m_pNumCommandsPerMeshTypeBuffer == nullptr);
	FormattedBufferDesc numCommandsPerMeshTypeBufferDesc(pParams->m_NumMeshTypes, DXGI_FORMAT_R32_UINT, false, true);
	m_pNumCommandsPerMeshTypeBuffer = new Buffer(pRenderEnv, pRenderEnv->m_pDefaultHeapProps, &numCommandsPerMeshTypeBufferDesc,
		pParams->m_InputResourceStates.m_NumCommandsPerMeshTypeBufferState, L"CreateVoxelizeCommandsPass::m_pNumCommandsPerMeshTypeBuffer");

	m_OutputResourceStates.m_NumMeshesBufferState = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
	m_OutputResourceStates.m_MeshInfoBufferState = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
	m_OutputResourceStates.m_NumCommandsPerMeshTypeBufferState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
	m_OutputResourceStates.m_VoxelizeCommandBufferState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;

	assert(m_CreateCommandsResourceBarriers.empty());
	AddCreateCommandsResourceBarrierIfRequired(m_pArgumentBuffer,
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
		D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);

	AddCreateCommandsResourceBarrierIfRequired(pParams->m_pMeshInfoBuffer,
		pParams->m_InputResourceStates.m_MeshInfoBufferState,
		m_OutputResourceStates.m_MeshInfoBufferState);

	AddCreateCommandsResourceBarrierIfRequired(m_pNumCommandsPerMeshTypeBuffer,
		pParams->m_InputResourceStates.m_NumCommandsPerMeshTypeBufferState,
		m_OutputResourceStates.m_NumCommandsPerMeshTypeBufferState);

	AddCreateCommandsResourceBarrierIfRequired(m_pVoxelizeCommandBuffer,
		pParams->m_InputResourceStates.m_VoxelizeCommandBufferState,
		m_OutputResourceStates.m_VoxelizeCommandBufferState);

	m_CreateCommandsSRVHeapStart = pRenderEnv->m_pShaderVisibleSRVHeap->Allocate();
	pRenderEnv->m_pDevice->CopyDescriptor(m_CreateCommandsSRVHeapStart,
		m_pNumCommandsPerMeshTypeBuffer->GetUAVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	pRenderEnv->m_pDevice->CopyDescriptor(pRenderEnv->m_pShaderVisibleSRVHeap->Allocate(),
		m_pVoxelizeCommandBuffer->GetUAVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	
	pRenderEnv->m_pDevice->CopyDescriptor(pRenderEnv->m_pShaderVisibleSRVHeap->Allocate(),
		pParams->m_pNumMeshesBuffer->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	pRenderEnv->m_pDevice->CopyDescriptor(pRenderEnv->m_pShaderVisibleSRVHeap->Allocate(),
		pParams->m_pMeshInfoBuffer->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}

void CreateVoxelizeCommandsPass::InitCreateCommandsRootSignature(InitParams* pParams)
{
	assert(m_pCreateCommandsRootSignature == nullptr);

	RenderEnv* pRenderEnv = pParams->m_pRenderEnv;
	D3D12_ROOT_PARAMETER rootParams[kCreateCommandsNumRootParams];

	D3D12_DESCRIPTOR_RANGE descriptorRanges[] = {UAVDescriptorRange(2, 0), SRVDescriptorRange(2, 0)};
	rootParams[kCreateCommandsRootSRVTableParam] = RootDescriptorTableParameter(ARRAYSIZE(descriptorRanges), descriptorRanges, D3D12_SHADER_VISIBILITY_ALL);

	RootSignatureDesc rootSignatureDesc(kCreateCommandsNumRootParams, rootParams);
	m_pCreateCommandsRootSignature = new RootSignature(pRenderEnv->m_pDevice, &rootSignatureDesc, L"CreateVoxelizeCommandsPass::m_pCreateCommandsRootSignature");
}

void CreateVoxelizeCommandsPass::InitCreateCommandsPipelineState(InitParams* pParams)
{
	assert(m_pCreateCommandsRootSignature != nullptr);
	assert(m_pCreateCommandsPipelineState == nullptr);

	RenderEnv* pRenderEnv = pParams->m_pRenderEnv;

	std::string numThreadsPerGroupStr = std::to_string(NUM_THREADS_PER_GROUP);
	std::string numMeshTypesStr = std::to_string(pParams->m_NumMeshTypes);

	const ShaderMacro shaderDefines[] =
	{
		ShaderMacro("CREATE_COMMANDS", "1"),
		ShaderMacro("NUM_THREADS_PER_GROUP", numThreadsPerGroupStr.c_str()),
		ShaderMacro("NUM_MESH_TYPES", numMeshTypesStr.c_str()),
		ShaderMacro()
	};
	Shader computeShader(L"Shaders//CreateVoxelizeCommandsCS.hlsl", "Main", "cs_5_0", shaderDefines);

	ComputePipelineStateDesc pipelineStateDesc;
	pipelineStateDesc.SetRootSignature(m_pCreateCommandsRootSignature);
	pipelineStateDesc.SetComputeShader(&computeShader);

	m_pCreateCommandsPipelineState = new PipelineState(pRenderEnv->m_pDevice, &pipelineStateDesc, L"CreateVoxelizeCommandsPass::m_pCreateCommandsPipelineState");
}

void CreateVoxelizeCommandsPass::InitCreateCommandsCommandSignature(InitParams* pParams)
{
	assert(m_pCreateCommandsCommandSignature == nullptr);
	RenderEnv* pRenderEnv = pParams->m_pRenderEnv;

	D3D12_INDIRECT_ARGUMENT_DESC argumentDescs[] =
	{
		DispatchArgument()
	};
	CommandSignatureDesc commandSignatureDesc(sizeof(DispatchArguments), ARRAYSIZE(argumentDescs), argumentDescs);
	m_pCreateCommandsCommandSignature = new CommandSignature(pRenderEnv->m_pDevice, nullptr, &commandSignatureDesc, L"CreateVoxelizeCommandsPass::m_pCreateCommandsCommandSignature");
}

void CreateVoxelizeCommandsPass::AddCreateCommandsResourceBarrierIfRequired(GraphicsResource* pResource, D3D12_RESOURCE_STATES currState, D3D12_RESOURCE_STATES requiredState)
{
	if (currState != requiredState)
		m_CreateCommandsResourceBarriers.emplace_back(pResource, currState, requiredState);
}
