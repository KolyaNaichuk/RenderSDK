#include "RenderPasses/CreateVoxelizeCommandsPass.h"
#include "D3DWrapper/CommandList.h"
#include "D3DWrapper/CommandSignature.h"
#include "D3DWrapper/GraphicsDevice.h"
#include "D3DWrapper/GraphicsResource.h"
#include "D3DWrapper/PipelineState.h"
#include "D3DWrapper/RenderEnv.h"
#include "D3DWrapper/RootSignature.h"

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
}

CreateVoxelizeCommandsPass::CreateVoxelizeCommandsPass(InitParams* pParams)
	: m_pSetArgumentsRootSignature(nullptr)
	, m_pSetArgumentsPipelineState(nullptr)
	, m_pArgumentBuffer(nullptr)
	, m_pCreateCommandsRootSignature(nullptr)
	, m_pCreateCommandsPipelineState(nullptr)
	, m_pCreateCommandsCommandSignature(nullptr)
	, m_pVoxelizeCommandBuffer(nullptr)
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
	SafeDelete(m_pVoxelizeCommandBuffer);
	
	SafeDelete(m_pCreateCommandsPipelineState);
	SafeDelete(m_pCreateCommandsRootSignature);
	SafeDelete(m_pCreateCommandsCommandSignature);

	SafeDelete(m_pSetArgumentsPipelineState);
	SafeDelete(m_pSetArgumentsRootSignature);
}

void CreateVoxelizeCommandsPass::Record(RenderParams* pParams)
{
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

	const ShaderMacro shaderDefines[] =
	{
		ShaderMacro("SET_ARGUMENTS", "1"),
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
	m_OutputResourceStates.m_NumMeshesBufferState = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
	m_OutputResourceStates.m_MeshInfoBufferState = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
	m_OutputResourceStates.m_NumMeshesPerTypeBufferState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
	m_OutputResourceStates.m_VoxelizeCommandBufferState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
}

void CreateVoxelizeCommandsPass::InitCreateCommandsRootSignature(InitParams* pParams)
{
}

void CreateVoxelizeCommandsPass::InitCreateCommandsPipelineState(InitParams* pParams)
{
}

void CreateVoxelizeCommandsPass::InitCreateCommandsCommandSignature(InitParams* pParams)
{
}

void CreateVoxelizeCommandsPass::AddCreateCommandsResourceBarrierIfRequired(GraphicsResource* pResource, D3D12_RESOURCE_STATES currState, D3D12_RESOURCE_STATES requiredState)
{
	if (currState != requiredState)
		m_CreateCommandsResourceBarriers.emplace_back(pResource, currState, requiredState);
}
