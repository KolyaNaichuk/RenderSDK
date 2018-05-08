#include "RenderPasses/CreateShadowMapCommandsPass.h"
#include "RenderPasses/Common.h"
#include "D3DWrapper/GraphicsDevice.h"
#include "D3DWrapper/CommandList.h"
#include "D3DWrapper/CommandSignature.h"
#include "D3DWrapper/PipelineState.h"
#include "D3DWrapper/RenderEnv.h"
#include "D3DWrapper/RootSignature.h"
#include "Math/Math.h"
#include "Profiler/GPUProfiler.h"

namespace
{
	enum RootParams
	{
		kRoot32BitConstantsParam = 0,
		kRootSRVTableParam,
		kNumRootParams
	};
}

CreateShadowMapCommandsPass::CreateShadowMapCommandsPass(InitParams* pParams)
	: m_Name(pParams->m_pName)
{
	assert((pParams->m_MaxNumPointLights + pParams->m_MaxNumSpotLights > 0));

	InitResources(pParams);
	InitRootSignature(pParams);
	InitPipelineState(pParams);
	InitCommandSignature(pParams);
}

CreateShadowMapCommandsPass::~CreateShadowMapCommandsPass()
{
	SafeDelete(m_pRootSignature);
	SafeDelete(m_pPipelineState);
	SafeDelete(m_pCommandSignature);

	SafeDelete(m_pNumPointLightMeshInstancesBuffer);
	SafeDelete(m_pPointLightIndexForMeshInstanceBuffer);
	SafeDelete(m_pMeshInstanceIndexForPointLightBuffer);
	SafeDelete(m_pNumPointLightCommandsBuffer);
	SafeDelete(m_pPointLightCommandBuffer);
	
	SafeDelete(m_pNumSpotLightMeshInstancesBuffer);
	SafeDelete(m_pSpotLightIndexForMeshInstanceBuffer);
	SafeDelete(m_pMeshInstanceIndexForSpotLightBuffer);
	SafeDelete(m_pNumSpotLightCommandsBuffer);
	SafeDelete(m_pSpotLightCommandBuffer);

	SafeDelete(m_pArgumentBuffer);
}

void CreateShadowMapCommandsPass::Record(RenderParams* pParams)
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
	
	if (!m_ResourceBarriers.empty())
		pCommandList->ResourceBarrier((UINT)m_ResourceBarriers.size(), m_ResourceBarriers.data());

	pCommandList->CopyBufferRegion(m_pArgumentBuffer, 0, pParams->m_pNumMeshesBuffer, 0, sizeof(UINT));
	ResourceTransitionBarrier argumentTransitionBarrier(m_pArgumentBuffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
	pCommandList->ResourceBarrier(1, &argumentTransitionBarrier);

	const UINT constants[] = {pParams->m_NumPointLights, pParams->m_NumSpotLights};
	pCommandList->SetComputeRoot32BitConstants(kRoot32BitConstantsParam, ARRAYSIZE(constants), constants, 0);
	pCommandList->SetComputeRootDescriptorTable(kRootSRVTableParam, m_SRVHeapStart);
	
	const UINT clearValue[4] = {0, 0, 0, 0};
	
	if ((m_pNumPointLightMeshInstancesBuffer != nullptr) && (m_pNumPointLightCommandsBuffer != nullptr))
	{
		pCommandList->ClearUnorderedAccessView(m_NumPointLightMeshInstancesBufferUAV, m_pNumPointLightMeshInstancesBuffer->GetUAVHandle(),
			m_pNumPointLightMeshInstancesBuffer, clearValue);

		pCommandList->ClearUnorderedAccessView(m_NumPointLightCommandsBufferUAV, m_pNumPointLightCommandsBuffer->GetUAVHandle(),
			m_pNumPointLightCommandsBuffer, clearValue);
	}
	if ((m_pNumSpotLightMeshInstancesBuffer != nullptr) && (m_pNumSpotLightCommandsBuffer != nullptr))
	{
		pCommandList->ClearUnorderedAccessView(m_NumSpotLightMeshInstancesBufferUAV, m_pNumSpotLightMeshInstancesBuffer->GetUAVHandle(),
			m_pNumSpotLightMeshInstancesBuffer, clearValue);

		pCommandList->ClearUnorderedAccessView(m_NumSpotLightCommandsBufferUAV, m_pNumSpotLightCommandsBuffer->GetUAVHandle(),
			m_pNumSpotLightCommandsBuffer, clearValue);
	}

	pCommandList->ExecuteIndirect(m_pCommandSignature, 1, m_pArgumentBuffer, 0, nullptr, 0);

#ifdef ENABLE_PROFILING
	pGPUProfiler->EndProfile(pCommandList, profileIndex);
#endif // ENABLE_PROFILING
	pCommandList->End();
}

void CreateShadowMapCommandsPass::InitResources(InitParams* pParams)
{
	RenderEnv* pRenderEnv = pParams->m_pRenderEnv;

	m_OutputResourceStates.m_MeshInfoBufferState = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
	m_OutputResourceStates.m_MeshInstanceWorldAABBBufferState = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
	m_OutputResourceStates.m_MeshInstanceIndexBufferState = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
	m_OutputResourceStates.m_NumMeshesBufferState = D3D12_RESOURCE_STATE_COPY_SOURCE;

	m_OutputResourceStates.m_PointLightWorldBoundsBufferState = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
	m_OutputResourceStates.m_NumPointLightMeshInstancesBufferState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
	m_OutputResourceStates.m_PointLightIndexForMeshInstanceBufferState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
	m_OutputResourceStates.m_MeshInstanceIndexForPointLightBufferState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
	m_OutputResourceStates.m_NumPointLightCommandsBufferState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
	m_OutputResourceStates.m_PointLightCommandBufferState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;

	m_OutputResourceStates.m_SpotLightWorldBoundsBufferState = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
	m_OutputResourceStates.m_NumSpotLightMeshInstancesBufferState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
	m_OutputResourceStates.m_SpotLightIndexForMeshInstanceBufferState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
	m_OutputResourceStates.m_MeshInstanceIndexForSpotLightBufferState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
	m_OutputResourceStates.m_NumSpotLightCommandsBufferState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
	m_OutputResourceStates.m_SpotLightCommandBufferState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;

	assert(m_pArgumentBuffer == nullptr);
	DispatchArguments argumentBufferData(0, 1, 1);
	StructuredBufferDesc argumentBufferDesc(1, sizeof(argumentBufferData), false, false);
	m_pArgumentBuffer = new Buffer(pRenderEnv, pRenderEnv->m_pDefaultHeapProps, &argumentBufferDesc,
		D3D12_RESOURCE_STATE_COPY_DEST, L"CreateShadowMapCommandsPass::m_pArgumentBuffer");
	UploadData(pRenderEnv, m_pArgumentBuffer, argumentBufferDesc,
		D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT, &argumentBufferData, sizeof(argumentBufferData));

	if (pParams->m_MaxNumPointLights > 0)
	{
		assert(m_pNumPointLightMeshInstancesBuffer == nullptr);
		FormattedBufferDesc numPointLightMeshInstancesBufferDesc(1, DXGI_FORMAT_R32_UINT, false, true);
		m_pNumPointLightMeshInstancesBuffer = new Buffer(pRenderEnv, pRenderEnv->m_pDefaultHeapProps, &numPointLightMeshInstancesBufferDesc,
			pParams->m_InputResourceStates.m_NumPointLightMeshInstancesBufferState, L"CreateShadowMapCommandsPass::m_pNumPointLightMeshInstancesBuffer");

		assert(m_pPointLightIndexForMeshInstanceBuffer == nullptr);
		FormattedBufferDesc pointLightIndexForMeshInstanceBufferDesc(pParams->m_MaxNumInstances * pParams->m_MaxNumPointLights, DXGI_FORMAT_R32_UINT, true, true);
		m_pPointLightIndexForMeshInstanceBuffer = new Buffer(pRenderEnv, pRenderEnv->m_pDefaultHeapProps, &pointLightIndexForMeshInstanceBufferDesc,
			pParams->m_InputResourceStates.m_PointLightIndexForMeshInstanceBufferState, L"CreateShadowMapCommandsPass::m_pPointLightIndexForMeshInstanceBuffer");

		assert(m_pMeshInstanceIndexForPointLightBuffer == nullptr);
		FormattedBufferDesc meshInstanceIndexForPointLightBufferDesc(pParams->m_MaxNumInstances * pParams->m_MaxNumPointLights, DXGI_FORMAT_R32_UINT, true, true);
		m_pMeshInstanceIndexForPointLightBuffer = new Buffer(pRenderEnv, pRenderEnv->m_pDefaultHeapProps, &meshInstanceIndexForPointLightBufferDesc,
			pParams->m_InputResourceStates.m_MeshInstanceIndexForPointLightBufferState, L"CreateShadowMapCommandsPass::m_pMeshInstanceIndexForPointLightBuffer");

		assert(m_pNumPointLightCommandsBuffer == nullptr);
		FormattedBufferDesc numPointLightCommandsBufferDesc(1, DXGI_FORMAT_R32_UINT, false, true);
		m_pNumPointLightCommandsBuffer = new Buffer(pRenderEnv, pRenderEnv->m_pDefaultHeapProps, &numPointLightCommandsBufferDesc,
			pParams->m_InputResourceStates.m_NumPointLightCommandsBufferState, L"CreateShadowMapCommandsPass::m_pNumPointLightCommandsBuffer");

		assert(m_pPointLightCommandBuffer == nullptr);
		StructuredBufferDesc pointLightCommandBufferDesc(pParams->m_MaxNumMeshes, sizeof(ShadowMapCommand), false, true);
		m_pPointLightCommandBuffer = new Buffer(pRenderEnv, pRenderEnv->m_pDefaultHeapProps, &pointLightCommandBufferDesc,
			pParams->m_InputResourceStates.m_PointLightCommandBufferState, L"CreateShadowMapCommandsPass::m_pPointLightCommandBuffer");
	}

	if (pParams->m_MaxNumSpotLights > 0)
	{
		assert(m_pNumSpotLightMeshInstancesBuffer == nullptr);
		FormattedBufferDesc numSpotLightMeshInstancesBufferDesc(1, DXGI_FORMAT_R32_UINT, false, true);
		m_pNumSpotLightMeshInstancesBuffer = new Buffer(pRenderEnv, pRenderEnv->m_pDefaultHeapProps, &numSpotLightMeshInstancesBufferDesc,
			pParams->m_InputResourceStates.m_NumSpotLightMeshInstancesBufferState, L"CreateShadowMapCommandsPass::m_pNumSpotLightMeshInstancesBuffer");

		assert(m_pSpotLightIndexForMeshInstanceBuffer == nullptr);
		FormattedBufferDesc spotLightIndexForMeshInstanceBufferDesc(pParams->m_MaxNumInstances * pParams->m_MaxNumSpotLights, DXGI_FORMAT_R32_UINT, true, true);
		m_pSpotLightIndexForMeshInstanceBuffer = new Buffer(pRenderEnv, pRenderEnv->m_pDefaultHeapProps, &spotLightIndexForMeshInstanceBufferDesc,
			pParams->m_InputResourceStates.m_SpotLightIndexForMeshInstanceBufferState, L"CreateShadowMapCommandsPass::m_pSpotLightIndexForMeshInstanceBuffer");

		assert(m_pMeshInstanceIndexForSpotLightBuffer == nullptr);
		FormattedBufferDesc meshInstanceIndexForSpotLightBufferDesc(pParams->m_MaxNumInstances * pParams->m_MaxNumSpotLights, DXGI_FORMAT_R32_UINT, true, true);
		m_pMeshInstanceIndexForSpotLightBuffer = new Buffer(pRenderEnv, pRenderEnv->m_pDefaultHeapProps, &meshInstanceIndexForSpotLightBufferDesc,
			pParams->m_InputResourceStates.m_MeshInstanceIndexForSpotLightBufferState, L"CreateShadowMapCommandsPass::m_pMeshInstanceIndexForSpotLightBuffer");

		assert(m_pNumSpotLightCommandsBuffer == nullptr);
		FormattedBufferDesc numSpotLightCommandsBufferDesc(1, DXGI_FORMAT_R32_UINT, false, true);
		m_pNumSpotLightCommandsBuffer = new Buffer(pRenderEnv, pRenderEnv->m_pDefaultHeapProps, &numSpotLightCommandsBufferDesc,
			pParams->m_InputResourceStates.m_NumSpotLightCommandsBufferState, L"CreateShadowMapCommandsPass::m_pNumSpotLightCommandsBuffer");

		assert(m_pSpotLightCommandBuffer == nullptr);
		StructuredBufferDesc spotLightCommandBufferDesc(pParams->m_MaxNumMeshes, sizeof(ShadowMapCommand), false, true);
		m_pSpotLightCommandBuffer = new Buffer(pRenderEnv, pRenderEnv->m_pDefaultHeapProps, &spotLightCommandBufferDesc,
			pParams->m_InputResourceStates.m_SpotLightCommandBufferState, L"CreateShadowMapCommandsPass::m_pSpotLightCommandBuffer");
	}

	assert(m_ResourceBarriers.empty());
	AddResourceBarrierIfRequired(m_pArgumentBuffer,
		D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT,
		D3D12_RESOURCE_STATE_COPY_DEST);

	AddResourceBarrierIfRequired(pParams->m_pMeshInfoBuffer,
		pParams->m_InputResourceStates.m_MeshInfoBufferState,
		m_OutputResourceStates.m_MeshInfoBufferState);

	AddResourceBarrierIfRequired(pParams->m_pMeshInstanceWorldAABBBuffer,
		pParams->m_InputResourceStates.m_MeshInstanceWorldAABBBufferState,
		m_OutputResourceStates.m_MeshInstanceWorldAABBBufferState);

	AddResourceBarrierIfRequired(pParams->m_pMeshInstanceIndexBuffer,
		pParams->m_InputResourceStates.m_MeshInstanceIndexBufferState,
		m_OutputResourceStates.m_MeshInstanceIndexBufferState);

	AddResourceBarrierIfRequired(pParams->m_pNumMeshesBuffer,
		pParams->m_InputResourceStates.m_NumMeshesBufferState,
		m_OutputResourceStates.m_NumMeshesBufferState);

	if (pParams->m_MaxNumPointLights > 0)
	{
		AddResourceBarrierIfRequired(pParams->m_pPointLightWorldBoundsBuffer,
			pParams->m_InputResourceStates.m_PointLightWorldBoundsBufferState,
			m_OutputResourceStates.m_PointLightWorldBoundsBufferState);

		AddResourceBarrierIfRequired(m_pNumPointLightMeshInstancesBuffer,
			pParams->m_InputResourceStates.m_NumPointLightMeshInstancesBufferState,
			m_OutputResourceStates.m_NumPointLightMeshInstancesBufferState);

		AddResourceBarrierIfRequired(m_pPointLightIndexForMeshInstanceBuffer,
			pParams->m_InputResourceStates.m_PointLightIndexForMeshInstanceBufferState,
			m_OutputResourceStates.m_PointLightIndexForMeshInstanceBufferState);

		AddResourceBarrierIfRequired(m_pMeshInstanceIndexForPointLightBuffer,
			pParams->m_InputResourceStates.m_MeshInstanceIndexForPointLightBufferState,
			m_OutputResourceStates.m_MeshInstanceIndexForPointLightBufferState);

		AddResourceBarrierIfRequired(m_pNumPointLightCommandsBuffer,
			pParams->m_InputResourceStates.m_NumPointLightCommandsBufferState,
			m_OutputResourceStates.m_NumPointLightCommandsBufferState);
		
		AddResourceBarrierIfRequired(m_pPointLightCommandBuffer,
			pParams->m_InputResourceStates.m_PointLightCommandBufferState,
			m_OutputResourceStates.m_PointLightCommandBufferState);
	}

	if (pParams->m_MaxNumSpotLights > 0)
	{
		AddResourceBarrierIfRequired(pParams->m_pSpotLightWorldBoundsBuffer,
			pParams->m_InputResourceStates.m_SpotLightWorldBoundsBufferState,
			m_OutputResourceStates.m_SpotLightWorldBoundsBufferState);

		AddResourceBarrierIfRequired(m_pNumSpotLightMeshInstancesBuffer,
			pParams->m_InputResourceStates.m_NumSpotLightMeshInstancesBufferState,
			m_OutputResourceStates.m_NumSpotLightMeshInstancesBufferState);

		AddResourceBarrierIfRequired(m_pSpotLightIndexForMeshInstanceBuffer,
			pParams->m_InputResourceStates.m_SpotLightIndexForMeshInstanceBufferState,
			m_OutputResourceStates.m_SpotLightIndexForMeshInstanceBufferState);

		AddResourceBarrierIfRequired(m_pMeshInstanceIndexForSpotLightBuffer,
			pParams->m_InputResourceStates.m_MeshInstanceIndexForSpotLightBufferState,
			m_OutputResourceStates.m_MeshInstanceIndexForSpotLightBufferState);

		AddResourceBarrierIfRequired(m_pNumSpotLightCommandsBuffer,
			pParams->m_InputResourceStates.m_NumSpotLightCommandsBufferState,
			m_OutputResourceStates.m_NumSpotLightCommandsBufferState);

		AddResourceBarrierIfRequired(m_pSpotLightCommandBuffer,
			pParams->m_InputResourceStates.m_SpotLightCommandBufferState,
			m_OutputResourceStates.m_SpotLightCommandBufferState);
	}

	m_SRVHeapStart = pRenderEnv->m_pShaderVisibleSRVHeap->Allocate();
	pRenderEnv->m_pDevice->CopyDescriptor(m_SRVHeapStart,
		pParams->m_pMeshInfoBuffer->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	pRenderEnv->m_pDevice->CopyDescriptor(pRenderEnv->m_pShaderVisibleSRVHeap->Allocate(),
		pParams->m_pMeshInstanceWorldAABBBuffer->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	pRenderEnv->m_pDevice->CopyDescriptor(pRenderEnv->m_pShaderVisibleSRVHeap->Allocate(),
		pParams->m_pMeshInstanceIndexBuffer->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	
	if (pParams->m_MaxNumPointLights > 0)
	{
		pRenderEnv->m_pDevice->CopyDescriptor(pRenderEnv->m_pShaderVisibleSRVHeap->Allocate(),
			pParams->m_pPointLightWorldBoundsBuffer->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		m_NumPointLightMeshInstancesBufferUAV = pRenderEnv->m_pShaderVisibleSRVHeap->Allocate();
		pRenderEnv->m_pDevice->CopyDescriptor(m_NumPointLightMeshInstancesBufferUAV,
			m_pNumPointLightMeshInstancesBuffer->GetUAVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		pRenderEnv->m_pDevice->CopyDescriptor(pRenderEnv->m_pShaderVisibleSRVHeap->Allocate(),
			m_pPointLightIndexForMeshInstanceBuffer->GetUAVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		pRenderEnv->m_pDevice->CopyDescriptor(pRenderEnv->m_pShaderVisibleSRVHeap->Allocate(),
			m_pMeshInstanceIndexForPointLightBuffer->GetUAVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		m_NumPointLightCommandsBufferUAV = pRenderEnv->m_pShaderVisibleSRVHeap->Allocate();
		pRenderEnv->m_pDevice->CopyDescriptor(m_NumPointLightCommandsBufferUAV,
			m_pNumPointLightCommandsBuffer->GetUAVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		pRenderEnv->m_pDevice->CopyDescriptor(pRenderEnv->m_pShaderVisibleSRVHeap->Allocate(),
			m_pPointLightCommandBuffer->GetUAVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	}

	if (pParams->m_MaxNumSpotLights > 0)
	{
		pRenderEnv->m_pDevice->CopyDescriptor(pRenderEnv->m_pShaderVisibleSRVHeap->Allocate(),
			pParams->m_pSpotLightWorldBoundsBuffer->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		m_NumSpotLightMeshInstancesBufferUAV = pRenderEnv->m_pShaderVisibleSRVHeap->Allocate();
		pRenderEnv->m_pDevice->CopyDescriptor(m_NumSpotLightMeshInstancesBufferUAV,
			m_pNumSpotLightMeshInstancesBuffer->GetUAVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		pRenderEnv->m_pDevice->CopyDescriptor(pRenderEnv->m_pShaderVisibleSRVHeap->Allocate(),
			m_pSpotLightIndexForMeshInstanceBuffer->GetUAVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		pRenderEnv->m_pDevice->CopyDescriptor(pRenderEnv->m_pShaderVisibleSRVHeap->Allocate(),
			m_pMeshInstanceIndexForSpotLightBuffer->GetUAVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		m_NumSpotLightCommandsBufferUAV = pRenderEnv->m_pShaderVisibleSRVHeap->Allocate();
		pRenderEnv->m_pDevice->CopyDescriptor(m_NumSpotLightCommandsBufferUAV,
			m_pNumSpotLightCommandsBuffer->GetUAVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		pRenderEnv->m_pDevice->CopyDescriptor(pRenderEnv->m_pShaderVisibleSRVHeap->Allocate(),
			m_pSpotLightCommandBuffer->GetUAVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	}
}

void CreateShadowMapCommandsPass::InitRootSignature(InitParams* pParams)
{
	assert(m_pRootSignature == nullptr);

	D3D12_ROOT_PARAMETER rootParams[kNumRootParams];
	rootParams[kRoot32BitConstantsParam] = Root32BitConstantsParameter(0, D3D12_SHADER_VISIBILITY_ALL, 2);

	std::vector<D3D12_DESCRIPTOR_RANGE> descriptorRanges;
	descriptorRanges.push_back(SRVDescriptorRange(3, 0));

	if (pParams->m_MaxNumPointLights > 0)
	{
		descriptorRanges.push_back(SRVDescriptorRange(1, 3));
		descriptorRanges.push_back(UAVDescriptorRange(5, 0));
	}
	if (pParams->m_MaxNumSpotLights > 0)
	{
		descriptorRanges.push_back(SRVDescriptorRange(1, 4));
		descriptorRanges.push_back(UAVDescriptorRange(5, 5));
	}
	rootParams[kRootSRVTableParam] = RootDescriptorTableParameter((UINT)descriptorRanges.size(), descriptorRanges.data(), D3D12_SHADER_VISIBILITY_ALL);

	RootSignatureDesc rootSignatureDesc(kNumRootParams, rootParams);
	m_pRootSignature = new RootSignature(pParams->m_pRenderEnv->m_pDevice, &rootSignatureDesc, L"CreateShadowMapCommandsPass::m_pRootSignature");
}

void CreateShadowMapCommandsPass::InitPipelineState(InitParams* pParams)
{
	assert(m_pPipelineState == nullptr);
	assert(m_pRootSignature != nullptr);

	RenderEnv* pRenderEnv = pParams->m_pRenderEnv;

	const u16 numThreadsPerMesh = 64;
	std::string numThreadsPerMeshStr = std::to_string(numThreadsPerMesh);
	std::string enablePointLightsStr = std::to_string((pParams->m_MaxNumPointLights > 0) ? 1 : 0);
	std::string enableSpotLightsStr = std::to_string((pParams->m_MaxNumSpotLights > 0) ? 1 : 0);
	std::string maxNumInstancesPerMeshStr = std::to_string(pParams->m_MaxNumInstancesPerMesh);
	std::string maxNumLightsStr = std::to_string(Max(pParams->m_MaxNumPointLights, pParams->m_MaxNumSpotLights));
	
	const ShaderMacro shaderDefines[] =
	{
		ShaderMacro("NUM_THREADS_PER_MESH", numThreadsPerMeshStr.c_str()),
		ShaderMacro("ENABLE_POINT_LIGHTS", enablePointLightsStr.c_str()),
		ShaderMacro("ENABLE_SPOT_LIGHTS", enableSpotLightsStr.c_str()),
		ShaderMacro("MAX_NUM_INSTANCES_PER_MESH", maxNumInstancesPerMeshStr.c_str()),
		ShaderMacro("MAX_NUM_LIGHTS", maxNumLightsStr.c_str()),
		ShaderMacro()
	};
	Shader computeShader(L"Shaders//CreateShadowMapCommandsCS.hlsl", "Main", "cs_5_0", shaderDefines);

	ComputePipelineStateDesc pipelineStateDesc;
	pipelineStateDesc.SetRootSignature(m_pRootSignature);
	pipelineStateDesc.SetComputeShader(&computeShader);

	m_pPipelineState = new PipelineState(pRenderEnv->m_pDevice, &pipelineStateDesc, L"CreateShadowMapCommandsPass::m_pPipelineState");
}

void CreateShadowMapCommandsPass::InitCommandSignature(InitParams* pParams)
{
	assert(m_pCommandSignature == nullptr);
	RenderEnv* pRenderEnv = pParams->m_pRenderEnv;

	D3D12_INDIRECT_ARGUMENT_DESC argumentDescs[] =
	{
		DispatchArgument()
	};
	CommandSignatureDesc commandSignatureDesc(sizeof(DispatchArguments), ARRAYSIZE(argumentDescs), argumentDescs);
	m_pCommandSignature = new CommandSignature(pRenderEnv->m_pDevice, nullptr, &commandSignatureDesc, L"CreateShadowMapCommandsPass::m_pCommandSignature");
}

void CreateShadowMapCommandsPass::AddResourceBarrierIfRequired(GraphicsResource* pResource, D3D12_RESOURCE_STATES currState, D3D12_RESOURCE_STATES requiredState)
{
	if (currState != requiredState)
		m_ResourceBarriers.emplace_back(pResource, currState, requiredState);
}