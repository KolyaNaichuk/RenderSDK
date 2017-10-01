#include "RenderPasses/CalcShadingRectanglesPass.h"
#include "D3DWrapper/CommandList.h"
#include "D3DWrapper/GraphicsDevice.h"
#include "D3DWrapper/GraphicsResource.h"
#include "D3DWrapper/PipelineState.h"
#include "D3DWrapper/RenderEnv.h"
#include "D3DWrapper/RootSignature.h"
#include "Math/Vector4.h"

namespace
{
	enum RootParams
	{
		kRootCBVParam = 0,
		kRootSRVTableParam,
		kNumRootParams
	};
}

CalcShadingRectanglesPass::CalcShadingRectanglesPass(InitParams* pParams)
	: m_pRootSignature(nullptr)
	, m_pPipelineState(nullptr)
	, m_pShadngRectangleBuffer(nullptr)
	, m_NumThreadGroupsX(0)
	, m_NumThreadGroupsY(0)
{
	InitResources(pParams);
	InitRootSignature(pParams);
	InitPipelineState(pParams);
}

CalcShadingRectanglesPass::~CalcShadingRectanglesPass()
{
	SafeDelete(m_pRootSignature);
	SafeDelete(m_pPipelineState);
	SafeDelete(m_pShadngRectangleBuffer);
}

void CalcShadingRectanglesPass::Record(RenderParams* pParams)
{
	RenderEnv* pRenderEnv = pParams->m_pRenderEnv;
	CommandList* pCommandList = pParams->m_pCommandList;
	
	pCommandList->Begin(m_pPipelineState);
	pCommandList->SetComputeRootSignature(m_pRootSignature);

	if (!m_ResourceBarriers.empty())
		pCommandList->ResourceBarrier(m_ResourceBarriers.size(), m_ResourceBarriers.data());

	const UINT clearValue[4] = {0xffffffff, 0xffffffff, 0, 0};
	pCommandList->ClearUnorderedAccessView(m_SRVHeapStart, m_pShadngRectangleBuffer->GetUAVHandle(), m_pShadngRectangleBuffer, clearValue);

	pCommandList->SetDescriptorHeaps(pRenderEnv->m_pShaderVisibleSRVHeap);
	pCommandList->SetComputeRootConstantBufferView(kRootCBVParam, pParams->m_pAppDataBuffer);
	pCommandList->SetComputeRootDescriptorTable(kRootSRVTableParam, m_SRVHeapStart);

	pCommandList->Dispatch(m_NumThreadGroupsX, m_NumThreadGroupsY, 1);
	pCommandList->End();
}

void CalcShadingRectanglesPass::InitResources(InitParams* pParams)
{
	RenderEnv* pRenderEnv = pParams->m_pRenderEnv;
	
	assert(m_pShadngRectangleBuffer == nullptr);
	StructuredBufferDesc shadingRectBufferDesc(pParams->m_NumMeshTypes, sizeof(Vector4u), true, true);
	m_pShadngRectangleBuffer = new Buffer(pRenderEnv, pRenderEnv->m_pDefaultHeapProps, &shadingRectBufferDesc,
		pParams->m_InputResourceStates.m_ShadingRectangleBufferState, L"m_pShadngRectangleBuffer");

	m_OutputResourceStates.m_MeshTypeDepthTextureState = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
	m_OutputResourceStates.m_ShadingRectangleBufferState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;

	assert(m_ResourceBarriers.empty());
	CreateResourceBarrierIfRequired(pParams->m_pMeshTypeDepthTexture,
		pParams->m_InputResourceStates.m_MeshTypeDepthTextureState,
		m_OutputResourceStates.m_MeshTypeDepthTextureState);

	CreateResourceBarrierIfRequired(m_pShadngRectangleBuffer,
		pParams->m_InputResourceStates.m_ShadingRectangleBufferState,
		m_OutputResourceStates.m_ShadingRectangleBufferState);

	m_SRVHeapStart = pRenderEnv->m_pShaderVisibleSRVHeap->Allocate();
	pRenderEnv->m_pDevice->CopyDescriptor(m_SRVHeapStart,
		m_pShadngRectangleBuffer->GetUAVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	pRenderEnv->m_pDevice->CopyDescriptor(pRenderEnv->m_pShaderVisibleSRVHeap->Allocate(),
		pParams->m_pMeshTypeDepthTexture->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}

void CalcShadingRectanglesPass::InitRootSignature(InitParams* pParams)
{
	assert(m_pRootSignature == nullptr);

	D3D12_ROOT_PARAMETER rootParams[kNumRootParams];
	rootParams[kRootCBVParam] = RootCBVParameter(0, D3D12_SHADER_VISIBILITY_ALL);

	D3D12_DESCRIPTOR_RANGE descriptorRanges[] = {UAVDescriptorRange(1, 0), SRVDescriptorRange(1, 0)};
	rootParams[kRootSRVTableParam] = RootDescriptorTableParameter(ARRAYSIZE(descriptorRanges), descriptorRanges, D3D12_SHADER_VISIBILITY_ALL);

	RootSignatureDesc rootSignatureDesc(kNumRootParams, rootParams);
	m_pRootSignature = new RootSignature(pParams->m_pRenderEnv->m_pDevice, &rootSignatureDesc, L"CalcShadingRectanglesPass::m_pRootSignature");
}

void CalcShadingRectanglesPass::InitPipelineState(InitParams* pParams)
{
	assert(m_pRootSignature != nullptr);
	assert(m_pPipelineState == nullptr);
	
	RenderEnv* pRenderEnv = pParams->m_pRenderEnv;

	const u8 numThreadsX = 16;
	const u8 numThreadsY = 16;

	m_NumThreadGroupsX = (u32)Ceil((f32)pParams->m_pMeshTypeDepthTexture->GetWidth() / (f32)numThreadsX);
	m_NumThreadGroupsY = (u32)Ceil((f32)pParams->m_pMeshTypeDepthTexture->GetHeight() / (f32)numThreadsY);

	std::string numThreadsXStr = std::to_string(numThreadsX);
	std::string numThreadsYStr = std::to_string(numThreadsY);
	std::string numMeshTypesStr = std::to_string(pParams->m_NumMeshTypes);
	
	const ShaderMacro shaderDefines[] =
	{
		ShaderMacro("NUM_THREADS_X", numThreadsXStr.c_str()),
		ShaderMacro("NUM_THREADS_Y", numThreadsYStr.c_str()),
		ShaderMacro("NUM_MESH_TYPES", numMeshTypesStr.c_str()),
		ShaderMacro()
	};
	Shader computeShader(L"Shaders//CalcShadingRectanglesCS.hlsl", "Main", "cs_5_0", shaderDefines);

	ComputePipelineStateDesc pipelineStateDesc;
	pipelineStateDesc.SetRootSignature(m_pRootSignature);
	pipelineStateDesc.SetComputeShader(&computeShader);

	m_pPipelineState = new PipelineState(pRenderEnv->m_pDevice, &pipelineStateDesc, L"CalcShadingRectanglesPass::m_pPipelineState");
}

void CalcShadingRectanglesPass::CreateResourceBarrierIfRequired(GraphicsResource* pResource, D3D12_RESOURCE_STATES currState, D3D12_RESOURCE_STATES requiredState)
{
	if (currState != requiredState)
		m_ResourceBarriers.emplace_back(pResource, currState, requiredState);
}
