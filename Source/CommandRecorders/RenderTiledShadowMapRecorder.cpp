#include "CommandRecorders/RenderTiledShadowMapRecorder.h"
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
	k32BitConstantRootParamVS,
	kSRVRootParamGS,
	kNumRootParams
};

RenderTiledShadowMapRecorder::RenderTiledShadowMapRecorder(InitParams* pParams)
	: m_pPipelineState(nullptr)
	, m_pRootSignature(nullptr)
	, m_pCommandSignature(nullptr)
{
	assert(pParams->m_LightType == LightType_Spot);

	D3DRenderEnv* pRenderEnv = pParams->m_pRenderEnv;
	MeshBatch* pMeshBatch = pParams->m_pMeshBatch;
		
	std::string lightTypeStr = std::to_string(pParams->m_LightType);
	const D3DShaderMacro geometryShaderDefines[] =
	{
		D3DShaderMacro("LIGHT_TYPE", lightTypeStr.c_str()),
		D3DShaderMacro()
	};

	D3DShader vertexShader(L"Shaders//RenderTiledShadowMapVS.hlsl", "Main", "vs_4_0");
	D3DShader geometryShader(L"Shaders//RenderTiledShadowMapGS.hlsl", "Main", "gs_4_0", geometryShaderDefines);

	D3D12_DESCRIPTOR_RANGE descriptorRangesVS[] = {D3DCBVRange(1, 0)};
	D3D12_DESCRIPTOR_RANGE descriptorRangesGS[] = {D3DSRVRange(3, 0)};

	D3D12_ROOT_PARAMETER rootParams[kNumRootParams];
	rootParams[kCBVRootParamVS] = D3DRootDescriptorTableParameter(ARRAYSIZE(descriptorRangesVS), &descriptorRangesVS[0], D3D12_SHADER_VISIBILITY_VERTEX);
	rootParams[k32BitConstantRootParamVS] = D3DRoot32BitConstantsParameter(1, D3D12_SHADER_VISIBILITY_VERTEX, 1);
	rootParams[kSRVRootParamGS] = D3DRootDescriptorTableParameter(ARRAYSIZE(descriptorRangesGS), &descriptorRangesGS[0], D3D12_SHADER_VISIBILITY_GEOMETRY);

	D3DRootSignatureDesc rootSignatureDesc(kNumRootParams, &rootParams[0], D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
	m_pRootSignature = new D3DRootSignature(pRenderEnv->m_pDevice, &rootSignatureDesc, L"RenderTiledShadowMapRecorder::m_pRootSignature");

	D3DGraphicsPipelineStateDesc pipelineStateDesc;
	pipelineStateDesc.SetRootSignature(m_pRootSignature);
	pipelineStateDesc.SetVertexShader(&vertexShader);
	pipelineStateDesc.SetGeometryShader(&geometryShader);
	pipelineStateDesc.InputLayout = *pMeshBatch->GetInputLayout();
	pipelineStateDesc.PrimitiveTopologyType = pMeshBatch->GetPrimitiveTopologyType();
	pipelineStateDesc.DepthStencilState = D3DDepthStencilDesc(D3DDepthStencilDesc::Enabled);
	pipelineStateDesc.SetRenderTargetFormats(0, nullptr, pParams->m_DSVFormat);

	m_pPipelineState = new D3DPipelineState(pRenderEnv->m_pDevice, &pipelineStateDesc, L"RenderTiledShadowMapRecorder::m_pPipelineState");

	D3D12_INDIRECT_ARGUMENT_DESC argumentDescs[] =
	{
		D3D32BitConstantsArgument(k32BitConstantRootParamVS, 0, 1),
		D3DDrawIndexedArgument()
	};
	D3DCommandSignatureDesc commandSignatureDesc(sizeof(DrawMeshCommand), ARRAYSIZE(argumentDescs), &argumentDescs[0]);
	m_pCommandSignature = new D3DCommandSignature(pRenderEnv->m_pDevice, m_pRootSignature, &commandSignatureDesc, L"RenderTiledShadowMapRecorder::m_pCommandSignature");
}

RenderTiledShadowMapRecorder::~RenderTiledShadowMapRecorder()
{
	SafeDelete(m_pCommandSignature);
	SafeDelete(m_pRootSignature);
	SafeDelete(m_pPipelineState);
}

void RenderTiledShadowMapRecorder::Record(RenderPassParams* pParams)
{
	D3DRenderEnv* pRenderEnv = pParams->m_pRenderEnv;
	D3DCommandList* pCommandList = pParams->m_pCommandList;
	D3DResourceList* pResources = pParams->m_pResources;
	MeshBatch* pMeshBatch = pParams->m_pMeshBatch;

	pCommandList->Reset(pParams->m_pCommandAllocator, m_pPipelineState);
	pCommandList->SetGraphicsRootSignature(m_pRootSignature);
	
	pCommandList->SetResourceTransitions(&pResources->m_RequiredResourceStates);
	pCommandList->SetDescriptorHeaps(pRenderEnv->m_pShaderVisibleSRVHeap);
	pCommandList->SetGraphicsRootDescriptorTable(kCBVRootParamVS, pResources->m_SRVHeapStart);
	pCommandList->SetGraphicsRootDescriptorTable(kSRVRootParamGS, D3DDescriptorHandle(pResources->m_SRVHeapStart, 1));

	D3D12_CPU_DESCRIPTOR_HANDLE dsvHeapStart = pResources->m_DSVHeapStart;
	pCommandList->ClearDepthStencilView(dsvHeapStart, 1.0f);
	pCommandList->OMSetRenderTargets(0, nullptr, FALSE, &dsvHeapStart);

	pCommandList->IASetPrimitiveTopology(pMeshBatch->GetPrimitiveTopology());
	pCommandList->IASetVertexBuffers(0, 1, pMeshBatch->GetVertexBuffer()->GetVBView());
	pCommandList->IASetIndexBuffer(pMeshBatch->GetIndexBuffer()->GetIBView());

	D3DRect scissorRect(ExtractRect(pParams->m_pViewport));
	pCommandList->RSSetViewports(1, pParams->m_pViewport);
	pCommandList->RSSetScissorRects(1, &scissorRect);

	pCommandList->ExecuteIndirect(m_pCommandSignature, pMeshBatch->GetNumMeshes(), pParams->m_pDrawShadowCasterCommandBuffer, 0, pParams->m_pNumDrawShadowCastersBuffer, 0);
	pCommandList->Close();
}