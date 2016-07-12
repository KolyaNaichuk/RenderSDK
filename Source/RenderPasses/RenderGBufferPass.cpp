#include "RenderPasses/RenderGBufferPass.h"
#include "Common/MeshBatch.h"
#include "D3DWrapper/D3DPipelineState.h"
#include "D3DWrapper/D3DRootSignature.h"
#include "D3DWrapper/D3DCommandSignature.h"
#include "D3DWrapper/D3DCommandList.h"
#include "D3DWrapper/D3DResource.h"
#include "D3DWrapper/D3DUtils.h"
#include "D3DWrapper/D3DRenderEnv.h"

enum RootParams
{
	kCBVRootParamVS = 0,
	k32BitConstantRootParamPS,
	kSRVRootParamPS,
	kNumRootParams
};

RenderGBufferPass::RenderGBufferPass(InitParams* pParams)
	: m_pRootSignature(nullptr)
	, m_pPipelineState(nullptr)
	, m_pCommandSignature(nullptr)
{
	D3DRenderEnv* pRenderEnv = pParams->m_pRenderEnv;
	MeshBatch* pMeshBatch = pParams->m_pMeshBatch;

	D3DShader vertexShader(L"Shaders//FillGBufferVS.hlsl", "Main", "vs_4_0");
	D3DShader pixelShader(L"Shaders//FillGBufferPS.hlsl", "Main", "ps_4_0");

	D3D12_DESCRIPTOR_RANGE descriptorRangesVS[] = {D3DCBVRange(1, 0)};
	D3D12_DESCRIPTOR_RANGE descriptorRangesPS[] = {D3DSRVRange(1, 0)};

	D3D12_ROOT_PARAMETER rootParams[kNumRootParams];
	rootParams[kCBVRootParamVS] = D3DRootDescriptorTableParameter(ARRAYSIZE(descriptorRangesVS), &descriptorRangesVS[0], D3D12_SHADER_VISIBILITY_VERTEX);
	rootParams[k32BitConstantRootParamPS] = D3DRoot32BitConstantsParameter(0, D3D12_SHADER_VISIBILITY_PIXEL, 1);
	rootParams[kSRVRootParamPS] = D3DRootDescriptorTableParameter(ARRAYSIZE(descriptorRangesPS), &descriptorRangesPS[0], D3D12_SHADER_VISIBILITY_PIXEL);
	
	D3DRootSignatureDesc rootSignatureDesc(kNumRootParams, &rootParams[0], D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
	m_pRootSignature = new D3DRootSignature(pRenderEnv->m_pDevice, &rootSignatureDesc, L"RenderGBufferPass::m_pRootSignature");
		
	D3DGraphicsPipelineStateDesc pipelineStateDesc;
	pipelineStateDesc.SetRootSignature(m_pRootSignature);
	pipelineStateDesc.SetVertexShader(&vertexShader);
	pipelineStateDesc.SetPixelShader(&pixelShader);
	pipelineStateDesc.InputLayout = *pMeshBatch->GetInputLayout();
	pipelineStateDesc.PrimitiveTopologyType = pMeshBatch->GetPrimitiveTopologyType();
	pipelineStateDesc.DepthStencilState = D3DDepthStencilDesc(D3DDepthStencilDesc::Enabled);
	
	const DXGI_FORMAT rtvFormats[] = {pParams->m_NormalRTVFormat, pParams->m_DiffuseRTVFormat, pParams->m_SpecularRTVFormat};
	pipelineStateDesc.SetRenderTargetFormats(ARRAYSIZE(rtvFormats), rtvFormats, pParams->m_DSVFormat);

	m_pPipelineState = new D3DPipelineState(pRenderEnv->m_pDevice, &pipelineStateDesc, L"RenderGBufferPass::m_pPipelineState");

	D3D12_INDIRECT_ARGUMENT_DESC argumentDescs[] = 
	{
		D3D32BitConstantsArgument(k32BitConstantRootParamPS, 0, 1),
		D3DDrawIndexedArgument()
	};
	D3DCommandSignatureDesc commandSignatureDesc(sizeof(DrawMeshCommand), ARRAYSIZE(argumentDescs), &argumentDescs[0]);
	m_pCommandSignature = new D3DCommandSignature(pRenderEnv->m_pDevice, m_pRootSignature, &commandSignatureDesc, L"RenderGBufferPass::m_pCommandSignature");
}

RenderGBufferPass::~RenderGBufferPass()
{
	SafeDelete(m_pCommandSignature);
	SafeDelete(m_pPipelineState);
	SafeDelete(m_pRootSignature);
}

void RenderGBufferPass::Record(RenderParams* pParams)
{
	D3DRenderEnv* pRenderEnv = pParams->m_pRenderEnv;
	D3DCommandList* pCommandList = pParams->m_pCommandList;
	D3DResourceList* pResources = pParams->m_pResources;
	MeshBatch* pMeshBatch = pParams->m_pMeshBatch;

	pCommandList->Reset(pParams->m_pCommandAllocator, m_pPipelineState);
	pCommandList->SetGraphicsRootSignature(m_pRootSignature);

	pCommandList->SetResourceTransitions(&pResources->m_RequiredResourceStates);
	pCommandList->SetDescriptorHeaps(pRenderEnv->m_pShaderVisibleSRVHeap);

	const FLOAT clearColor[4] = {0.0f, 0.0f, 0.0f, 0.0f};	
	pCommandList->ClearRenderTargetView(pResources->m_RTVHeapStart, clearColor);
	pCommandList->ClearRenderTargetView(D3DDescriptorHandle(pResources->m_RTVHeapStart, 1), clearColor);
	pCommandList->ClearRenderTargetView(D3DDescriptorHandle(pResources->m_RTVHeapStart, 2), clearColor);
	pCommandList->ClearDepthStencilView(pResources->m_DSVHeapStart, 1.0f);

	pCommandList->SetGraphicsRootDescriptorTable(kCBVRootParamVS, pResources->m_SRVHeapStart);
	pCommandList->SetGraphicsRootDescriptorTable(kSRVRootParamPS, D3DDescriptorHandle(pResources->m_SRVHeapStart, 1));

	D3D12_CPU_DESCRIPTOR_HANDLE rtvHeapStart = pResources->m_RTVHeapStart;
	D3D12_CPU_DESCRIPTOR_HANDLE dsvHeapStart = pResources->m_DSVHeapStart;
	pCommandList->OMSetRenderTargets(3, &rtvHeapStart, TRUE, &dsvHeapStart);
	
	pCommandList->IASetPrimitiveTopology(pMeshBatch->GetPrimitiveTopology());
	pCommandList->IASetVertexBuffers(0, 1, pMeshBatch->GetVertexBuffer()->GetVBView());
	pCommandList->IASetIndexBuffer(pMeshBatch->GetIndexBuffer()->GetIBView());

	pCommandList->RSSetViewports(1, pParams->m_pViewport);

	D3DRect scissorRect(ExtractRect(pParams->m_pViewport));
	pCommandList->RSSetScissorRects(1, &scissorRect);

	pCommandList->ExecuteIndirect(m_pCommandSignature, pMeshBatch->GetNumMeshes(), pParams->m_pDrawMeshCommandBuffer, 0, pParams->m_pNumDrawMeshesBuffer, 0);
	pCommandList->Close();
}
