#include "RenderPasses/CalcShadingRectanglesPass.h"
#include "D3DWrapper/CommandList.h"
#include "D3DWrapper/GraphicsDevice.h"
#include "D3DWrapper/GraphicsResource.h"
#include "D3DWrapper/PipelineState.h"
#include "D3DWrapper/RenderEnv.h"
#include "D3DWrapper/RootSignature.h"
#include "Math/Vector2.h"

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
	, m_pShadingRectangleMinPointBuffer(nullptr)
	, m_pShadingRectangleMaxPointBuffer(nullptr)
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
	SafeDelete(m_pShadingRectangleMinPointBuffer);
	SafeDelete(m_pShadingRectangleMaxPointBuffer);
}

void CalcShadingRectanglesPass::Record(RenderParams* pParams)
{
	RenderEnv* pRenderEnv = pParams->m_pRenderEnv;
	CommandList* pCommandList = pParams->m_pCommandList;
	
	pCommandList->Begin(m_pPipelineState);
	pCommandList->SetComputeRootSignature(m_pRootSignature);

	if (!m_ResourceBarriers.empty())
		pCommandList->ResourceBarrier((UINT)m_ResourceBarriers.size(), m_ResourceBarriers.data());
	
	const UINT minValue[4] = {0xffffffff, 0, 0, 0};
	pCommandList->ClearUnorderedAccessView(m_SRVHeapStart, m_pShadingRectangleMinPointBuffer->GetUAVHandle(),
		m_pShadingRectangleMinPointBuffer, minValue);

	const UINT maxValue[4] = {0, 0, 0, 0};
	pCommandList->ClearUnorderedAccessView(DescriptorHandle(m_SRVHeapStart, 1), m_pShadingRectangleMaxPointBuffer->GetUAVHandle(),
		m_pShadingRectangleMaxPointBuffer, maxValue);

	pCommandList->SetDescriptorHeaps(pRenderEnv->m_pShaderVisibleSRVHeap);
	pCommandList->SetComputeRootConstantBufferView(kRootCBVParam, pParams->m_pAppDataBuffer);
	pCommandList->SetComputeRootDescriptorTable(kRootSRVTableParam, m_SRVHeapStart);

	pCommandList->Dispatch(m_NumThreadGroupsX, m_NumThreadGroupsY, 1);
	pCommandList->End();
}

void CalcShadingRectanglesPass::InitResources(InitParams* pParams)
{
	RenderEnv* pRenderEnv = pParams->m_pRenderEnv;
	StructuredBufferDesc shadingRectBufferDesc(pParams->m_NumMeshTypes, sizeof(Vector2u), true, true);

	assert(m_pShadingRectangleMinPointBuffer == nullptr);
	m_pShadingRectangleMinPointBuffer = new Buffer(pRenderEnv, pRenderEnv->m_pDefaultHeapProps, &shadingRectBufferDesc,
		pParams->m_InputResourceStates.m_ShadingRectangleMinPointBufferState, L"m_pShadingRectangleMinPointBuffer");

	assert(m_pShadingRectangleMaxPointBuffer == nullptr);
	m_pShadingRectangleMaxPointBuffer = new Buffer(pRenderEnv, pRenderEnv->m_pDefaultHeapProps, &shadingRectBufferDesc,
		pParams->m_InputResourceStates.m_ShadingRectangleMaxPointBufferState, L"m_pShadingRectangleMaxPointBuffer");

	m_OutputResourceStates.m_MaterialIDTextureState = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
	m_OutputResourceStates.m_MeshTypePerMaterialIDBufferState = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
	m_OutputResourceStates.m_ShadingRectangleMinPointBufferState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
	m_OutputResourceStates.m_ShadingRectangleMaxPointBufferState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;

	assert(m_ResourceBarriers.empty());
	CreateResourceBarrierIfRequired(m_pShadingRectangleMinPointBuffer,
		pParams->m_InputResourceStates.m_ShadingRectangleMinPointBufferState,
		m_OutputResourceStates.m_ShadingRectangleMinPointBufferState);

	CreateResourceBarrierIfRequired(m_pShadingRectangleMaxPointBuffer,
		pParams->m_InputResourceStates.m_ShadingRectangleMaxPointBufferState,
		m_OutputResourceStates.m_ShadingRectangleMaxPointBufferState);

	CreateResourceBarrierIfRequired(pParams->m_pMaterialIDTexture,
		pParams->m_InputResourceStates.m_MaterialIDTextureState,
		m_OutputResourceStates.m_MaterialIDTextureState);

	CreateResourceBarrierIfRequired(pParams->m_pMeshTypePerMaterialIDBuffer,
		pParams->m_InputResourceStates.m_MeshTypePerMaterialIDBufferState,
		m_OutputResourceStates.m_MeshTypePerMaterialIDBufferState);

	m_SRVHeapStart = pRenderEnv->m_pShaderVisibleSRVHeap->Allocate();
	pRenderEnv->m_pDevice->CopyDescriptor(m_SRVHeapStart,
		m_pShadingRectangleMinPointBuffer->GetUAVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	pRenderEnv->m_pDevice->CopyDescriptor(pRenderEnv->m_pShaderVisibleSRVHeap->Allocate(),
		m_pShadingRectangleMaxPointBuffer->GetUAVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	pRenderEnv->m_pDevice->CopyDescriptor(pRenderEnv->m_pShaderVisibleSRVHeap->Allocate(),
		pParams->m_pMaterialIDTexture->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	pRenderEnv->m_pDevice->CopyDescriptor(pRenderEnv->m_pShaderVisibleSRVHeap->Allocate(),
		pParams->m_pMeshTypePerMaterialIDBuffer->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}

void CalcShadingRectanglesPass::InitRootSignature(InitParams* pParams)
{
	assert(m_pRootSignature == nullptr);

	D3D12_ROOT_PARAMETER rootParams[kNumRootParams];
	rootParams[kRootCBVParam] = RootCBVParameter(0, D3D12_SHADER_VISIBILITY_ALL);

	D3D12_DESCRIPTOR_RANGE descriptorRanges[] = {UAVDescriptorRange(2, 0), SRVDescriptorRange(2, 0)};
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

	m_NumThreadGroupsX = (u32)Ceil((f32)pParams->m_pMaterialIDTexture->GetWidth() / (f32)numThreadsX);
	m_NumThreadGroupsY = (u32)Ceil((f32)pParams->m_pMaterialIDTexture->GetHeight() / (f32)numThreadsY);

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
