#include "RenderPasses/RenderGBufferPass.h"
#include "RenderPasses/Common.h"
#include "Common/MeshRenderResources.h"
#include "D3DWrapper/PipelineState.h"
#include "D3DWrapper/RootSignature.h"
#include "D3DWrapper/CommandSignature.h"
#include "D3DWrapper/CommandList.h"
#include "D3DWrapper/GraphicsDevice.h"
#include "D3DWrapper/GraphicsResource.h"
#include "D3DWrapper/GraphicsUtils.h"
#include "D3DWrapper/RenderEnv.h"
#include "D3DWrapper/Profiler.h"

namespace
{
	enum RootParams
	{
		kRoot32BitConstantParamVS = 0,
		kRootCBVParamVS,
		kRootSRVTableParamVS,
		kRoot32BitConstantParamPS,
		kNumRootParams
	};
}

RenderGBufferPass::RenderGBufferPass(InitParams* pParams)
	: m_Name(pParams->m_pName)
{
	InitResources(pParams);
	InitRootSignature(pParams);
	InitPipelineState(pParams);
	InitCommandSignature(pParams);
}

RenderGBufferPass::~RenderGBufferPass()
{
	SafeDelete(m_pRootSignature);
	SafeDelete(m_pPipelineState);
	SafeDelete(m_pCommandSignature);
}

void RenderGBufferPass::Record(RenderParams* pParams)
{
	MeshRenderResources* pMeshRenderResources = pParams->m_pMeshRenderResources;
	assert(pMeshRenderResources->GetNumMeshTypes() == 1);
	const u32 meshType = 0;

	RenderEnv* pRenderEnv = pParams->m_pRenderEnv;
	CommandList* pCommandList = pParams->m_pCommandList;
	Profiler* pProfiler = pRenderEnv->m_pProfiler;
		
	pCommandList->Begin(m_pPipelineState);
#ifdef ENABLE_GPU_PROFILING
	u32 profileIndex = pProfiler->StartProfile(pCommandList, m_Name.c_str());
#endif // ENABLE_GPU_PROFILING

	pCommandList->SetGraphicsRootSignature(m_pRootSignature);
	pCommandList->SetDescriptorHeaps(pRenderEnv->m_pShaderVisibleSRVHeap);

	if (!m_ResourceBarriers.empty())
		pCommandList->ResourceBarrier((UINT)m_ResourceBarriers.size(), m_ResourceBarriers.data());
					
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHeapStart = m_RTVHeapStart;
	D3D12_CPU_DESCRIPTOR_HANDLE dsvHeapStart = m_DSVHeapStart;
	pCommandList->OMSetRenderTargets(3, &rtvHeapStart, TRUE, &dsvHeapStart);

	if (pParams->m_ClearGBufferBeforeRendering)
	{
		const FLOAT clearColor[] = {0.0f, 0.0f, 0.0f, 0.0f};

		pCommandList->ClearRenderTargetView(m_RTVHeapStart, clearColor);
		pCommandList->ClearRenderTargetView(DescriptorHandle(m_RTVHeapStart, 1), clearColor);
		pCommandList->ClearRenderTargetView(DescriptorHandle(m_RTVHeapStart, 2), clearColor);
		pCommandList->ClearDepthView(m_DSVHeapStart, 1.0f);
	}

	pCommandList->SetGraphicsRootConstantBufferView(kRootCBVParamVS, pParams->m_pAppDataBuffer);
	pCommandList->SetGraphicsRootDescriptorTable(kRootSRVTableParamVS, m_SRVHeapStartVS);
	
	pCommandList->IASetPrimitiveTopology(pMeshRenderResources->GetPrimitiveTopology(meshType));
	pCommandList->IASetVertexBuffers(0, 1, pMeshRenderResources->GetVertexBuffer(meshType)->GetVBView());
	pCommandList->IASetIndexBuffer(pMeshRenderResources->GetIndexBuffer(meshType)->GetIBView());
	
	Rect scissorRect(ExtractRect(pParams->m_pViewport));
	pCommandList->RSSetViewports(1, pParams->m_pViewport);
	pCommandList->RSSetScissorRects(1, &scissorRect);
	
	pCommandList->ExecuteIndirect(m_pCommandSignature, pMeshRenderResources->GetTotalNumMeshes(),
		pParams->m_pDrawCommandBuffer, pMeshRenderResources->GetMeshTypeOffset(meshType) * sizeof(DrawCommand),
		pParams->m_pNumVisibleMeshesPerTypeBuffer, meshType * sizeof(u32));
	
#ifdef ENABLE_GPU_PROFILING
	pProfiler->EndProfile(pCommandList, profileIndex);
#endif // ENABLE_GPU_PROFILING
	pCommandList->End();
}

void RenderGBufferPass::InitResources(InitParams* pParams)
{
	RenderEnv* pRenderEnv = pParams->m_pRenderEnv;

	const MeshRenderResources* pMeshRenderResources = pParams->m_pMeshRenderResources;
	assert(pMeshRenderResources->GetNumMeshTypes() == 1);
			
	m_OutputResourceStates.m_TexCoordTextureState = D3D12_RESOURCE_STATE_RENDER_TARGET;
	m_OutputResourceStates.m_NormalTextureState = D3D12_RESOURCE_STATE_RENDER_TARGET;
	m_OutputResourceStates.m_MaterialIDTextureState = D3D12_RESOURCE_STATE_RENDER_TARGET;
	m_OutputResourceStates.m_DepthTextureState = D3D12_RESOURCE_STATE_DEPTH_WRITE;
	m_OutputResourceStates.m_InstanceIndexBufferState = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
	m_OutputResourceStates.m_InstanceWorldMatrixBufferState = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
	m_OutputResourceStates.m_NumVisibleMeshesPerTypeBufferState = D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT;
	m_OutputResourceStates.m_DrawCommandBufferState = D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT;

	assert(m_ResourceBarriers.empty());
	AddResourceBarrierIfRequired(pParams->m_pTexCoordTexture,
		pParams->m_InputResourceStates.m_TexCoordTextureState,
		m_OutputResourceStates.m_TexCoordTextureState);

	AddResourceBarrierIfRequired(pParams->m_pNormalTexture,
		pParams->m_InputResourceStates.m_NormalTextureState,
		m_OutputResourceStates.m_NormalTextureState);

	AddResourceBarrierIfRequired(pParams->m_pMaterialIDTexture,
		pParams->m_InputResourceStates.m_MaterialIDTextureState,
		m_OutputResourceStates.m_MaterialIDTextureState);

	AddResourceBarrierIfRequired(pParams->m_pDepthTexture,
		pParams->m_InputResourceStates.m_DepthTextureState,
		m_OutputResourceStates.m_DepthTextureState);

	AddResourceBarrierIfRequired(pParams->m_pInstanceIndexBuffer,
		pParams->m_InputResourceStates.m_InstanceIndexBufferState,
		m_OutputResourceStates.m_InstanceIndexBufferState);

	AddResourceBarrierIfRequired(pParams->m_pInstanceWorldMatrixBuffer,
		pParams->m_InputResourceStates.m_InstanceWorldMatrixBufferState,
		m_OutputResourceStates.m_InstanceWorldMatrixBufferState);

	AddResourceBarrierIfRequired(pParams->m_NumVisibleMeshesPerTypeBuffer,
		pParams->m_InputResourceStates.m_NumVisibleMeshesPerTypeBufferState,
		m_OutputResourceStates.m_NumVisibleMeshesPerTypeBufferState);

	AddResourceBarrierIfRequired(pParams->m_DrawCommandBuffer,
		pParams->m_InputResourceStates.m_DrawCommandBufferState,
		m_OutputResourceStates.m_DrawCommandBufferState);
	
	m_SRVHeapStartVS = pRenderEnv->m_pShaderVisibleSRVHeap->Allocate();
	pRenderEnv->m_pDevice->CopyDescriptor(m_SRVHeapStartVS,
		pParams->m_pInstanceIndexBuffer->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	pRenderEnv->m_pDevice->CopyDescriptor(pRenderEnv->m_pShaderVisibleSRVHeap->Allocate(),
		pParams->m_pInstanceWorldMatrixBuffer->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	m_RTVHeapStart = pRenderEnv->m_pShaderInvisibleRTVHeap->Allocate();
	pRenderEnv->m_pDevice->CopyDescriptor(m_RTVHeapStart,
		pParams->m_pTexCoordTexture->GetRTVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	pRenderEnv->m_pDevice->CopyDescriptor(pRenderEnv->m_pShaderInvisibleRTVHeap->Allocate(),
		pParams->m_pNormalTexture->GetRTVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	pRenderEnv->m_pDevice->CopyDescriptor(pRenderEnv->m_pShaderInvisibleRTVHeap->Allocate(),
		pParams->m_pMaterialIDTexture->GetRTVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	m_DSVHeapStart = pParams->m_pDepthTexture->GetDSVHandle();
}

void RenderGBufferPass::InitRootSignature(InitParams* pParams)
{
	assert(m_pRootSignature == nullptr);

	D3D12_ROOT_PARAMETER rootParams[kNumRootParams];
	rootParams[kRoot32BitConstantParamVS] = Root32BitConstantsParameter(0, D3D12_SHADER_VISIBILITY_VERTEX, 1);
	rootParams[kRootCBVParamVS] = RootCBVParameter(1, D3D12_SHADER_VISIBILITY_VERTEX);

	D3D12_DESCRIPTOR_RANGE descriptorRangesVS[] = {SRVDescriptorRange(2, 0)};
	rootParams[kRootSRVTableParamVS] = RootDescriptorTableParameter(ARRAYSIZE(descriptorRangesVS), descriptorRangesVS, D3D12_SHADER_VISIBILITY_VERTEX);

	rootParams[kRoot32BitConstantParamPS] = Root32BitConstantsParameter(0, D3D12_SHADER_VISIBILITY_PIXEL, 1);

	RootSignatureDesc rootSignatureDesc(kNumRootParams, rootParams, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
	m_pRootSignature = new RootSignature(pParams->m_pRenderEnv->m_pDevice, &rootSignatureDesc, L"RenderGBufferPass::m_pRootSignature");
}

void RenderGBufferPass::InitPipelineState(InitParams* pParams)
{
	assert(m_pRootSignature != nullptr);
	assert(m_pPipelineState == nullptr);
	
	RenderEnv* pRenderEnv = pParams->m_pRenderEnv;
	
	const MeshRenderResources* pMeshRenderResources = pParams->m_pMeshRenderResources;
	assert(pMeshRenderResources->GetNumMeshTypes() == 1);
	const u32 meshType = 0;

	Shader vertexShader(L"Shaders//RenderGBufferVS.hlsl", "Main", "vs_4_0");
	Shader pixelShader(L"Shaders//RenderGBufferPS.hlsl", "Main", "ps_4_0");

	const InputLayoutDesc& inputLayout = pMeshRenderResources->GetInputLayout(meshType);
	assert(inputLayout.NumElements == 3);
	assert(HasVertexSemantic(inputLayout, "POSITION"));
	assert(HasVertexSemantic(inputLayout, "NORMAL"));
	assert(HasVertexSemantic(inputLayout, "TEXCOORD"));

	GraphicsPipelineStateDesc pipelineStateDesc;
	pipelineStateDesc.SetRootSignature(m_pRootSignature);
	pipelineStateDesc.SetVertexShader(&vertexShader);
	pipelineStateDesc.SetPixelShader(&pixelShader);
	pipelineStateDesc.InputLayout = inputLayout;
	pipelineStateDesc.PrimitiveTopologyType = pMeshRenderResources->GetPrimitiveTopologyType(meshType);
	pipelineStateDesc.DepthStencilState = DepthStencilDesc(DepthStencilDesc::Enabled);

	const DXGI_FORMAT rtvFormats[] =
	{
		GetRenderTargetViewFormat(pParams->m_pTexCoordTexture->GetFormat()),
		GetRenderTargetViewFormat(pParams->m_pNormalTexture->GetFormat()),
		GetRenderTargetViewFormat(pParams->m_pMaterialIDTexture->GetFormat())
	};
	const DXGI_FORMAT dsvFormat = GetDepthStencilViewFormat(pParams->m_pDepthTexture->GetFormat());
	pipelineStateDesc.SetRenderTargetFormats(ARRAYSIZE(rtvFormats), rtvFormats, dsvFormat);
	
	m_pPipelineState = new PipelineState(pRenderEnv->m_pDevice, &pipelineStateDesc, L"RenderGBufferPass::m_pPipelineState");
}

void RenderGBufferPass::InitCommandSignature(InitParams* pParams)
{
	assert(m_pRootSignature != nullptr);
	assert(m_pCommandSignature == nullptr);
	
	RenderEnv* pRenderEnv = pParams->m_pRenderEnv;

	D3D12_INDIRECT_ARGUMENT_DESC argumentDescs[] =
	{
		Constant32BitArgument(kRoot32BitConstantParamVS, 0, 1),
		Constant32BitArgument(kRoot32BitConstantParamPS, 0, 1),
		DrawIndexedArgument()
	};

	CommandSignatureDesc commandSignatureDesc(sizeof(DrawCommand), ARRAYSIZE(argumentDescs), argumentDescs);
	m_pCommandSignature = new CommandSignature(pRenderEnv->m_pDevice, m_pRootSignature, &commandSignatureDesc, L"RenderGBufferPass::m_pCommandSignature");
}

void RenderGBufferPass::AddResourceBarrierIfRequired(GraphicsResource* pResource, D3D12_RESOURCE_STATES currState, D3D12_RESOURCE_STATES requiredState)
{
	if (currState != requiredState)
		m_ResourceBarriers.emplace_back(pResource, currState, requiredState);
}
