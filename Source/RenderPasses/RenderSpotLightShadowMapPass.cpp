#include "RenderPasses/RenderSpotLightShadowMapPass.h"
#include "RenderPasses/MeshRenderResources.h"
#include "D3DWrapper/GraphicsDevice.h"
#include "D3DWrapper/GraphicsUtils.h"
#include "D3DWrapper/CommandList.h"
#include "D3DWrapper/PipelineState.h"
#include "D3DWrapper/RootSignature.h"
#include "D3DWrapper/RenderEnv.h"
#include "Profiler/GPUProfiler.h"

namespace
{
	enum RootParams
	{
		kRoot32BitConstantsParamVS,
		kRootSRVTableParamVS,
		kNumRootParams
	};
}

RenderSpotLightShadowMapPass::RenderSpotLightShadowMapPass(InitParams* pParams)
	: m_Name(pParams->m_pName)
{
	InitResources(pParams);
	InitRootSignature(pParams);
	InitPipelineState(pParams);
	InitCommandSignature(pParams);
}

RenderSpotLightShadowMapPass::~RenderSpotLightShadowMapPass()
{
	SafeDelete(m_pCommandSignature);
	SafeDelete(m_pPipelineState);
	SafeDelete(m_pRootSignature);
}

void RenderSpotLightShadowMapPass::Record(RenderParams* pParams)
{		
	RenderEnv* pRenderEnv = pParams->m_pRenderEnv;
	CommandList* pCommandList = pParams->m_pCommandList;

	DepthTexture* pSpotLightShadowMaps = pParams->m_pSpotLightShadowMaps;
	MeshRenderResources* pMeshRenderResources = pParams->m_pMeshRenderResources;
	assert(pMeshRenderResources->GetNumMeshTypes() == 1);
	const u32 meshType = 0;

	GPUProfiler* pGPUProfiler = pRenderEnv->m_pGPUProfiler;
#ifdef ENABLE_PROFILING
	u32 profileIndex = pGPUProfiler->StartProfile(pCommandList, m_Name.c_str());
#endif // ENABLE_PROFILING

	const ResourceTransitionBarrier resourceBarriers[] =
	{
		ResourceTransitionBarrier(pParams->m_pSpotLightShadowMaps,
			D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
			D3D12_RESOURCE_STATE_DEPTH_WRITE,
			pParams->m_ShadowMapIndex)
	};

	pCommandList->SetPipelineState(m_pPipelineState);
	pCommandList->SetGraphicsRootSignature(m_pRootSignature);
	pCommandList->SetDescriptorHeaps(pRenderEnv->m_pShaderVisibleSRVHeap);
	pCommandList->ResourceBarrier(ARRAYSIZE(resourceBarriers), resourceBarriers);
	
	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = pSpotLightShadowMaps->GetDSVHandle(0/*mipSlice*/, pParams->m_ShadowMapIndex);
	pCommandList->OMSetRenderTargets(0, nullptr, TRUE, &dsvHandle);
	pCommandList->ClearDepthView(dsvHandle, 1.0f);
	
	pCommandList->SetGraphicsRoot32BitConstant(kRoot32BitConstantsParamVS, pParams->m_SpotLightIndex, 0);
	pCommandList->SetGraphicsRootDescriptorTable(kRootSRVTableParamVS, m_SRVHeapStartVS);
	
	pCommandList->IASetPrimitiveTopology(pMeshRenderResources->GetPrimitiveTopology(meshType));
	pCommandList->IASetVertexBuffers(0, 1, pMeshRenderResources->GetVertexBuffer(meshType)->GetVBView());
	pCommandList->IASetIndexBuffer(pMeshRenderResources->GetIndexBuffer(meshType)->GetIBView());
	
	Viewport viewport(0.0f/*topLeftX*/, 0.0f/*topLeftY*/, FLOAT(pSpotLightShadowMaps->GetWidth()), FLOAT(pSpotLightShadowMaps->GetHeight()));
	Rect scissorRect(ExtractRect(&viewport));
	
	pCommandList->RSSetViewports(1, &viewport);
	pCommandList->RSSetScissorRects(1, &scissorRect);
	
	pCommandList->ExecuteIndirect(m_pCommandSignature, pParams->m_NumRenderCommands,
		pParams->m_pRenderCommandBuffer, pParams->m_FirstRenderCommand * sizeof(ShadowMapCommand),
		nullptr/*pCountBuffer*/, 0/*countBufferOffset*/);

#ifdef ENABLE_PROFILING
	pGPUProfiler->EndProfile(pCommandList, profileIndex);
#endif // ENABLE_PROFILING
}

void RenderSpotLightShadowMapPass::InitResources(InitParams* pParams)
{
	RenderEnv* pRenderEnv = pParams->m_pRenderEnv;

	assert(pParams->m_InputResourceStates.m_RenderCommandBufferState == D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
	assert(pParams->m_InputResourceStates.m_MeshInstanceIndexBufferState == D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	assert(pParams->m_InputResourceStates.m_MeshInstanceWorldMatrixBufferState == D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	assert(pParams->m_InputResourceStates.m_SpotLightViewProjMatrixBufferState == D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	assert(pParams->m_InputResourceStates.m_SpotLightShadowMapsState == D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	
	m_OutputResourceStates.m_RenderCommandBufferState = D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT;
	m_OutputResourceStates.m_MeshInstanceIndexBufferState = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
	m_OutputResourceStates.m_MeshInstanceWorldMatrixBufferState = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
	m_OutputResourceStates.m_SpotLightViewProjMatrixBufferState = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
	m_OutputResourceStates.m_SpotLightShadowMapsState = D3D12_RESOURCE_STATE_DEPTH_WRITE;
	
	assert(!m_SRVHeapStartVS.IsValid());
	m_SRVHeapStartVS = pRenderEnv->m_pShaderVisibleSRVHeap->Allocate();
	
	pRenderEnv->m_pDevice->CopyDescriptor(m_SRVHeapStartVS,
		pParams->m_pMeshInstanceIndexBuffer->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	pRenderEnv->m_pDevice->CopyDescriptor(pRenderEnv->m_pShaderVisibleSRVHeap->Allocate(),
		pParams->m_pMeshInstanceWorldMatrixBuffer->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	pRenderEnv->m_pDevice->CopyDescriptor(pRenderEnv->m_pShaderVisibleSRVHeap->Allocate(),
		pParams->m_pSpotLightViewProjMatrixBuffer->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}

void RenderSpotLightShadowMapPass::InitRootSignature(InitParams* pParams)
{
	assert(m_pRootSignature == nullptr);

	D3D12_ROOT_PARAMETER rootParams[kNumRootParams];
	rootParams[kRoot32BitConstantsParamVS] = Root32BitConstantsParameter(0, D3D12_SHADER_VISIBILITY_VERTEX, 2);

	D3D12_DESCRIPTOR_RANGE descriptorRanges[] = {SRVDescriptorRange(3, 0)};
	rootParams[kRootSRVTableParamVS] = RootDescriptorTableParameter(ARRAYSIZE(descriptorRanges), descriptorRanges, D3D12_SHADER_VISIBILITY_VERTEX);

	RootSignatureDesc rootSignatureDesc(kNumRootParams, rootParams, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
	m_pRootSignature = new RootSignature(pParams->m_pRenderEnv->m_pDevice, &rootSignatureDesc, L"RenderSpotLightShadowMapPass::m_pRootSignature");
}

void RenderSpotLightShadowMapPass::InitPipelineState(InitParams* pParams)
{
	assert(m_pPipelineState == nullptr);
	assert(m_pRootSignature != nullptr);

	RenderEnv* pRenderEnv = pParams->m_pRenderEnv;
	
	const MeshRenderResources* pMeshRenderResources = pParams->m_pMeshRenderResources;
	assert(pMeshRenderResources->GetNumMeshTypes() == 1);
	const u32 meshType = 0;
	
	const InputLayoutDesc& inputLayout = pMeshRenderResources->GetInputLayout(meshType);
	assert(inputLayout.NumElements == 3);
	assert(HasVertexSemantic(inputLayout, "POSITION"));
	assert(HasVertexSemantic(inputLayout, "NORMAL"));
	assert(HasVertexSemantic(inputLayout, "TEXCOORD"));

	Shader vertexShader(L"Shaders//RenderSpotLightShadowMapVS.hlsl", "Main", "vs_4_0");
	
	GraphicsPipelineStateDesc pipelineStateDesc;
	pipelineStateDesc.SetRootSignature(m_pRootSignature);
	pipelineStateDesc.SetVertexShader(&vertexShader);
	pipelineStateDesc.InputLayout = inputLayout;
	pipelineStateDesc.PrimitiveTopologyType = pMeshRenderResources->GetPrimitiveTopologyType(meshType);
	pipelineStateDesc.DepthStencilState = DepthStencilDesc(DepthStencilDesc::Enabled);
	pipelineStateDesc.SetRenderTargetFormats(0, nullptr, GetDepthStencilViewFormat(pParams->m_pSpotLightShadowMaps->GetFormat()));
	
	m_pPipelineState = new PipelineState(pRenderEnv->m_pDevice, &pipelineStateDesc, L"RenderSpotLightShadowMapPass::m_pPipelineState");
}

void RenderSpotLightShadowMapPass::InitCommandSignature(InitParams* pParams)
{
	assert(m_pCommandSignature == nullptr);
	assert(m_pRootSignature != nullptr);

	RenderEnv* pRenderEnv = pParams->m_pRenderEnv;

	const D3D12_INDIRECT_ARGUMENT_DESC argumentDescs[] =
	{
		Constant32BitArgument(kRoot32BitConstantsParamVS, 1, 1),
		DrawIndexedArgument()
	};

	CommandSignatureDesc commandSignatureDesc(sizeof(ShadowMapCommand), ARRAYSIZE(argumentDescs), argumentDescs);
	m_pCommandSignature = new CommandSignature(pRenderEnv->m_pDevice, m_pRootSignature, &commandSignatureDesc, L"RenderSpotLightShadowMapPass::m_pCommandSignature");
}
