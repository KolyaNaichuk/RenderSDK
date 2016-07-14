#include "RenderPasses/RenderGBufferPass.h"
#include "Common/MeshBatch.h"
#include "D3DWrapper/PipelineState.h"
#include "D3DWrapper/RootSignature.h"
#include "D3DWrapper/CommandSignature.h"
#include "D3DWrapper/CommandList.h"
#include "D3DWrapper/GraphicsResource.h"
#include "D3DWrapper/GraphicsUtils.h"
#include "D3DWrapper/RenderEnv.h"

enum RootParams
{
	kCBVRootParamVS = 0,
	kConstant32BitRootParamPS,
	kSRVRootParamPS,
	kNumRootParams
};

RenderGBufferPass::RenderGBufferPass(InitParams* pParams)
	: m_pRootSignature(nullptr)
	, m_pPipelineState(nullptr)
	, m_pCommandSignature(nullptr)
{
	RenderEnv* pRenderEnv = pParams->m_pRenderEnv;
	MeshBatch* pMeshBatch = pParams->m_pMeshBatch;

	Shader vertexShader(L"Shaders//FillGBufferVS.hlsl", "Main", "vs_4_0");
	Shader pixelShader(L"Shaders//FillGBufferPS.hlsl", "Main", "ps_4_0");

	D3D12_DESCRIPTOR_RANGE descriptorRangesVS[] = {CBVDescriptorRange(1, 0)};
	D3D12_DESCRIPTOR_RANGE descriptorRangesPS[] = {SRVDescriptorRange(1, 0)};

	D3D12_ROOT_PARAMETER rootParams[kNumRootParams];
	rootParams[kCBVRootParamVS] = RootDescriptorTableParameter(ARRAYSIZE(descriptorRangesVS), &descriptorRangesVS[0], D3D12_SHADER_VISIBILITY_VERTEX);
	rootParams[kConstant32BitRootParamPS] = Root32BitConstantsParameter(0, D3D12_SHADER_VISIBILITY_PIXEL, 1);
	rootParams[kSRVRootParamPS] = RootDescriptorTableParameter(ARRAYSIZE(descriptorRangesPS), &descriptorRangesPS[0], D3D12_SHADER_VISIBILITY_PIXEL);
	
	RootSignatureDesc rootSignatureDesc(kNumRootParams, &rootParams[0], D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
	m_pRootSignature = new RootSignature(pRenderEnv->m_pDevice, &rootSignatureDesc, L"RenderGBufferPass::m_pRootSignature");
		
	GraphicsPipelineStateDesc pipelineStateDesc;
	pipelineStateDesc.SetRootSignature(m_pRootSignature);
	pipelineStateDesc.SetVertexShader(&vertexShader);
	pipelineStateDesc.SetPixelShader(&pixelShader);
	pipelineStateDesc.InputLayout = *pMeshBatch->GetInputLayout();
	pipelineStateDesc.PrimitiveTopologyType = pMeshBatch->GetPrimitiveTopologyType();
	pipelineStateDesc.DepthStencilState = DepthStencilDesc(DepthStencilDesc::Enabled);
	
	const DXGI_FORMAT rtvFormats[] = {pParams->m_NormalRTVFormat, pParams->m_DiffuseRTVFormat, pParams->m_SpecularRTVFormat};
	pipelineStateDesc.SetRenderTargetFormats(ARRAYSIZE(rtvFormats), rtvFormats, pParams->m_DSVFormat);

	m_pPipelineState = new PipelineState(pRenderEnv->m_pDevice, &pipelineStateDesc, L"RenderGBufferPass::m_pPipelineState");

	D3D12_INDIRECT_ARGUMENT_DESC argumentDescs[] = 
	{
		Constant32BitArgument(kConstant32BitRootParamPS, 0, 1),
		DrawIndexedArgument()
	};
	CommandSignatureDesc commandSignatureDesc(sizeof(DrawMeshCommand), ARRAYSIZE(argumentDescs), &argumentDescs[0]);
	m_pCommandSignature = new CommandSignature(pRenderEnv->m_pDevice, m_pRootSignature, &commandSignatureDesc, L"RenderGBufferPass::m_pCommandSignature");
}

RenderGBufferPass::~RenderGBufferPass()
{
	SafeDelete(m_pCommandSignature);
	SafeDelete(m_pPipelineState);
	SafeDelete(m_pRootSignature);
}

void RenderGBufferPass::Record(RenderParams* pParams)
{
	RenderEnv* pRenderEnv = pParams->m_pRenderEnv;
	CommandList* pCommandList = pParams->m_pCommandList;
	BindingResourceList* pResources = pParams->m_pResources;
	MeshBatch* pMeshBatch = pParams->m_pMeshBatch;

	pCommandList->Reset(pParams->m_pCommandAllocator, m_pPipelineState);
	pCommandList->SetGraphicsRootSignature(m_pRootSignature);

	pCommandList->SetRequiredResourceStates(&pResources->m_RequiredResourceStates);
	pCommandList->SetDescriptorHeaps(pRenderEnv->m_pShaderVisibleSRVHeap);

	const FLOAT clearColor[4] = {0.0f, 0.0f, 0.0f, 0.0f};	
	pCommandList->ClearRenderTargetView(pResources->m_RTVHeapStart, clearColor);
	pCommandList->ClearRenderTargetView(DescriptorHandle(pResources->m_RTVHeapStart, 1), clearColor);
	pCommandList->ClearRenderTargetView(DescriptorHandle(pResources->m_RTVHeapStart, 2), clearColor);
	pCommandList->ClearDepthStencilView(pResources->m_DSVHeapStart, 1.0f);

	pCommandList->SetGraphicsRootDescriptorTable(kCBVRootParamVS, pResources->m_SRVHeapStart);
	pCommandList->SetGraphicsRootDescriptorTable(kSRVRootParamPS, DescriptorHandle(pResources->m_SRVHeapStart, 1));

	D3D12_CPU_DESCRIPTOR_HANDLE rtvHeapStart = pResources->m_RTVHeapStart;
	D3D12_CPU_DESCRIPTOR_HANDLE dsvHeapStart = pResources->m_DSVHeapStart;
	pCommandList->OMSetRenderTargets(3, &rtvHeapStart, TRUE, &dsvHeapStart);
	
	pCommandList->IASetPrimitiveTopology(pMeshBatch->GetPrimitiveTopology());
	pCommandList->IASetVertexBuffers(0, 1, pMeshBatch->GetVertexBuffer()->GetVBView());
	pCommandList->IASetIndexBuffer(pMeshBatch->GetIndexBuffer()->GetIBView());

	pCommandList->RSSetViewports(1, pParams->m_pViewport);

	Rect scissorRect(ExtractRect(pParams->m_pViewport));
	pCommandList->RSSetScissorRects(1, &scissorRect);

	pCommandList->ExecuteIndirect(m_pCommandSignature, pMeshBatch->GetNumMeshes(), pParams->m_pDrawMeshCommandBuffer, 0, pParams->m_pNumDrawMeshesBuffer, 0);
	pCommandList->Close();
}
