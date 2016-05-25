#include "CommandRecorders/FillGBufferRecorder.h"
#include "Common/MeshBatch.h"
#include "DX/DXPipelineState.h"
#include "DX/DXRootSignature.h"
#include "DX/DXCommandSignature.h"
#include "DX/DXCommandList.h"
#include "DX/DXResource.h"
#include "DX/DXUtils.h"
#include "DX/DXRenderEnvironment.h"

enum RootParams
{
	kCBVRootParamVS = 0,
	k32BitConstantRootParamPS,
	kSRVRootParamPS,
	kNumRootParams
};

FillGBufferRecorder::FillGBufferRecorder(InitParams* pParams)
	: m_pRootSignature(nullptr)
	, m_pPipelineState(nullptr)
	, m_pCommandSignature(nullptr)
{
	DXRenderEnvironment* pEnv = pParams->m_pEnv;
	MeshBatch* pMeshBatch = pParams->m_pMeshBatch;

	DXShader vertexShader(L"Shaders//FillGBufferVS.hlsl", "Main", "vs_4_0");
	DXShader pixelShader(L"Shaders//FillGBufferPS.hlsl", "Main", "ps_4_0");

	D3D12_DESCRIPTOR_RANGE descriptorRangesVS[] = {DXCBVRange(1, 0)};
	D3D12_DESCRIPTOR_RANGE descriptorRangesPS[] = {DXSRVRange(1, 0)};

	D3D12_ROOT_PARAMETER rootParams[kNumRootParams];
	rootParams[kCBVRootParamVS] = DXRootDescriptorTableParameter(ARRAYSIZE(descriptorRangesVS), &descriptorRangesVS[0], D3D12_SHADER_VISIBILITY_VERTEX);
	rootParams[k32BitConstantRootParamPS] = DXRoot32BitConstantsParameter(0, D3D12_SHADER_VISIBILITY_PIXEL, 1);
	rootParams[kSRVRootParamPS] = DXRootDescriptorTableParameter(ARRAYSIZE(descriptorRangesPS), &descriptorRangesPS[0], D3D12_SHADER_VISIBILITY_PIXEL);
	
	DXRootSignatureDesc rootSignatureDesc(kNumRootParams, &rootParams[0], D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
	m_pRootSignature = new DXRootSignature(pEnv->m_pDevice, &rootSignatureDesc, L"FillGBufferRecorder::m_pRootSignature");
		
	DXGraphicsPipelineStateDesc pipelineStateDesc;
	pipelineStateDesc.SetRootSignature(m_pRootSignature);
	pipelineStateDesc.SetVertexShader(&vertexShader);
	pipelineStateDesc.SetPixelShader(&pixelShader);
	pipelineStateDesc.InputLayout = *pMeshBatch->GetInputLayout();
	pipelineStateDesc.PrimitiveTopologyType = pMeshBatch->GetPrimitiveTopologyType();
	pipelineStateDesc.DepthStencilState = DXDepthStencilDesc(DXDepthStencilDesc::Enabled);
	
	const DXGI_FORMAT rtvFormats[] = {pParams->m_NormalRTVFormat, pParams->m_DiffuseRTVFormat, pParams->m_SpecularRTVFormat};
	pipelineStateDesc.SetRenderTargetFormats(ARRAYSIZE(rtvFormats), rtvFormats, pParams->m_DSVFormat);

	m_pPipelineState = new DXPipelineState(pEnv->m_pDevice, &pipelineStateDesc, L"FillGBufferRecorder::m_pPipelineState");

	D3D12_INDIRECT_ARGUMENT_DESC argumentDescs[] = 
	{
		DX32BitConstantsArgument(k32BitConstantRootParamPS, 0, 1),
		DXDrawIndexedArgument()
	};
	DXCommandSignatureDesc commandSignatureDesc(sizeof(FillGBufferCommand), ARRAYSIZE(argumentDescs), &argumentDescs[0]);
	m_pCommandSignature = new DXCommandSignature(pEnv->m_pDevice, m_pRootSignature, &commandSignatureDesc, L"FillGBufferRecorder::m_pCommandSignature");
}

FillGBufferRecorder::~FillGBufferRecorder()
{
	SafeDelete(m_pCommandSignature);
	SafeDelete(m_pPipelineState);
	SafeDelete(m_pRootSignature);
}

void FillGBufferRecorder::Record(RenderPassParams* pParams)
{
	DXRenderEnvironment* pEnv = pParams->m_pEnv;
	DXCommandList* pCommandList = pParams->m_pCommandList;
	DXBindingResourceList* pResources = pParams->m_pResources;
	MeshBatch* pMeshBatch = pParams->m_pMeshBatch;

	pCommandList->Reset(pParams->m_pCommandAllocator, m_pPipelineState);
	pCommandList->GetDXObject()->ClearState(m_pPipelineState->GetDXObject());
	pCommandList->SetGraphicsRootSignature(m_pRootSignature);

	pCommandList->SetResourceTransitions(&pResources->m_ResourceTransitions);
	pCommandList->SetDescriptorHeaps(pEnv->m_pShaderVisibleSRVHeap);

	const FLOAT clearColor[4] = {0.0f, 0.0f, 0.0f, 0.0f};	
	pCommandList->ClearRenderTargetView(pResources->m_RTVHeapStart, clearColor);
	pCommandList->ClearRenderTargetView(DXDescriptorHandle(pResources->m_RTVHeapStart, 1), clearColor);
	pCommandList->ClearRenderTargetView(DXDescriptorHandle(pResources->m_RTVHeapStart, 2), clearColor);
	pCommandList->ClearDepthStencilView(pResources->m_DSVHeapStart, 1.0f);

	pCommandList->SetGraphicsRootDescriptorTable(kCBVRootParamVS, pResources->m_SRVHeapStart);
	pCommandList->SetGraphicsRootDescriptorTable(kSRVRootParamPS, DXDescriptorHandle(pResources->m_SRVHeapStart, 1));

	D3D12_CPU_DESCRIPTOR_HANDLE rtvHeapStart = pResources->m_RTVHeapStart;
	D3D12_CPU_DESCRIPTOR_HANDLE dsvHeapStart = pResources->m_DSVHeapStart;
	pCommandList->OMSetRenderTargets(1, &rtvHeapStart, TRUE, &dsvHeapStart);
	
	pCommandList->IASetPrimitiveTopology(pMeshBatch->GetPrimitiveTopology());
	pCommandList->IASetVertexBuffers(0, 1, pMeshBatch->GetVertexBuffer()->GetVBView());
	pCommandList->IASetIndexBuffer(pMeshBatch->GetIndexBuffer()->GetIBView());

	pCommandList->RSSetViewports(1, pParams->m_pViewport);

	DXRect scissorRect(ExtractRect(pParams->m_pViewport));
	pCommandList->RSSetScissorRects(1, &scissorRect);

	pCommandList->ExecuteIndirect(m_pCommandSignature, pMeshBatch->GetNumMeshes(), pParams->m_pDrawCommandBuffer, 0, pParams->m_pNumDrawsBuffer, 0);

	pCommandList->Close();
}
