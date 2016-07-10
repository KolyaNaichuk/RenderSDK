#include "CommandRecorders/RenderTiledShadowMapRecorder.h"
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

	DXRenderEnvironment* pRenderEnv = pParams->m_pRenderEnv;
	MeshBatch* pMeshBatch = pParams->m_pMeshBatch;
		
	std::string lightTypeStr = std::to_string(pParams->m_LightType);
	const DXShaderMacro geometryShaderDefines[] =
	{
		DXShaderMacro("LIGHT_TYPE", lightTypeStr.c_str()),
		DXShaderMacro()
	};

	DXShader vertexShader(L"Shaders//RenderTiledShadowMapVS.hlsl", "Main", "vs_4_0");
	DXShader geometryShader(L"Shaders//RenderTiledShadowMapGS.hlsl", "Main", "gs_4_0", geometryShaderDefines);

	D3D12_DESCRIPTOR_RANGE descriptorRangesVS[] = {DXCBVRange(1, 0)};
	D3D12_DESCRIPTOR_RANGE descriptorRangesGS[] = {DXSRVRange(3, 0)};

	D3D12_ROOT_PARAMETER rootParams[kNumRootParams];
	rootParams[kCBVRootParamVS] = DXRootDescriptorTableParameter(ARRAYSIZE(descriptorRangesVS), &descriptorRangesVS[0], D3D12_SHADER_VISIBILITY_VERTEX);
	rootParams[k32BitConstantRootParamVS] = DXRoot32BitConstantsParameter(1, D3D12_SHADER_VISIBILITY_VERTEX, 1);
	rootParams[kSRVRootParamGS] = DXRootDescriptorTableParameter(ARRAYSIZE(descriptorRangesGS), &descriptorRangesGS[0], D3D12_SHADER_VISIBILITY_GEOMETRY);

	DXRootSignatureDesc rootSignatureDesc(kNumRootParams, &rootParams[0], D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
	m_pRootSignature = new DXRootSignature(pRenderEnv->m_pDevice, &rootSignatureDesc, L"RenderTiledShadowMapRecorder::m_pRootSignature");

	DXGraphicsPipelineStateDesc pipelineStateDesc;
	pipelineStateDesc.SetRootSignature(m_pRootSignature);
	pipelineStateDesc.SetVertexShader(&vertexShader);
	pipelineStateDesc.SetGeometryShader(&geometryShader);
	pipelineStateDesc.InputLayout = *pMeshBatch->GetInputLayout();
	pipelineStateDesc.PrimitiveTopologyType = pMeshBatch->GetPrimitiveTopologyType();
	pipelineStateDesc.DepthStencilState = DXDepthStencilDesc(DXDepthStencilDesc::Enabled);
	pipelineStateDesc.SetRenderTargetFormats(0, nullptr, pParams->m_DSVFormat);

	m_pPipelineState = new DXPipelineState(pRenderEnv->m_pDevice, &pipelineStateDesc, L"RenderTiledShadowMapRecorder::m_pPipelineState");

	D3D12_INDIRECT_ARGUMENT_DESC argumentDescs[] =
	{
		DX32BitConstantsArgument(k32BitConstantRootParamVS, 0, 1),
		DXDrawIndexedArgument()
	};
	DXCommandSignatureDesc commandSignatureDesc(sizeof(DrawMeshCommand), ARRAYSIZE(argumentDescs), &argumentDescs[0]);
	m_pCommandSignature = new DXCommandSignature(pRenderEnv->m_pDevice, m_pRootSignature, &commandSignatureDesc, L"RenderTiledShadowMapRecorder::m_pCommandSignature");
}

RenderTiledShadowMapRecorder::~RenderTiledShadowMapRecorder()
{
	SafeDelete(m_pCommandSignature);
	SafeDelete(m_pRootSignature);
	SafeDelete(m_pPipelineState);
}

void RenderTiledShadowMapRecorder::Record(RenderPassParams* pParams)
{
	DXRenderEnvironment* pRenderEnv = pParams->m_pRenderEnv;
	DXCommandList* pCommandList = pParams->m_pCommandList;
	DXBindingResourceList* pResources = pParams->m_pResources;
	MeshBatch* pMeshBatch = pParams->m_pMeshBatch;

	pCommandList->Reset(pParams->m_pCommandAllocator, m_pPipelineState);
	pCommandList->SetGraphicsRootSignature(m_pRootSignature);
	
	pCommandList->SetResourceTransitions(&pResources->m_ResourceTransitions);
	pCommandList->SetDescriptorHeaps(pRenderEnv->m_pShaderVisibleSRVHeap);
	pCommandList->SetGraphicsRootDescriptorTable(kCBVRootParamVS, pResources->m_SRVHeapStart);
	pCommandList->SetGraphicsRootDescriptorTable(kSRVRootParamGS, DXDescriptorHandle(pResources->m_SRVHeapStart, 1));

	D3D12_CPU_DESCRIPTOR_HANDLE dsvHeapStart = pResources->m_DSVHeapStart;
	pCommandList->ClearDepthStencilView(dsvHeapStart, 1.0f);
	pCommandList->OMSetRenderTargets(0, nullptr, FALSE, &dsvHeapStart);

	pCommandList->IASetPrimitiveTopology(pMeshBatch->GetPrimitiveTopology());
	pCommandList->IASetVertexBuffers(0, 1, pMeshBatch->GetVertexBuffer()->GetVBView());
	pCommandList->IASetIndexBuffer(pMeshBatch->GetIndexBuffer()->GetIBView());

	DXRect scissorRect(ExtractRect(pParams->m_pViewport));
	pCommandList->RSSetViewports(1, pParams->m_pViewport);
	pCommandList->RSSetScissorRects(1, &scissorRect);

	pCommandList->ExecuteIndirect(m_pCommandSignature, pMeshBatch->GetNumMeshes(), pParams->m_pDrawShadowCasterCommandBuffer, 0, pParams->m_pNumDrawShadowCastersBuffer, 0);
	pCommandList->Close();
}