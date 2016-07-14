#include "RenderPasses/RenderTiledShadowMapPass.h"
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
	kConstant32BitRootParamVS,
	kSRVRootParamGS,
	kNumRootParams
};

RenderTiledShadowMapPass::RenderTiledShadowMapPass(InitParams* pParams)
	: m_pPipelineState(nullptr)
	, m_pRootSignature(nullptr)
	, m_pCommandSignature(nullptr)
{
	assert(pParams->m_LightType == LightType_Spot);

	RenderEnv* pRenderEnv = pParams->m_pRenderEnv;
	MeshBatch* pMeshBatch = pParams->m_pMeshBatch;
		
	std::string lightTypeStr = std::to_string(pParams->m_LightType);
	const ShaderMacro geometryShaderDefines[] =
	{
		ShaderMacro("LIGHT_TYPE", lightTypeStr.c_str()),
		ShaderMacro()
	};

	Shader vertexShader(L"Shaders//RenderTiledShadowMapVS.hlsl", "Main", "vs_4_0");
	Shader geometryShader(L"Shaders//RenderTiledShadowMapGS.hlsl", "Main", "gs_4_0", geometryShaderDefines);

	D3D12_DESCRIPTOR_RANGE descriptorRangesVS[] = {CBVDescriptorRange(1, 0)};
	D3D12_DESCRIPTOR_RANGE descriptorRangesGS[] = {SRVDescriptorRange(3, 0)};

	D3D12_ROOT_PARAMETER rootParams[kNumRootParams];
	rootParams[kCBVRootParamVS] = RootDescriptorTableParameter(ARRAYSIZE(descriptorRangesVS), &descriptorRangesVS[0], D3D12_SHADER_VISIBILITY_VERTEX);
	rootParams[kConstant32BitRootParamVS] = Root32BitConstantsParameter(1, D3D12_SHADER_VISIBILITY_VERTEX, 1);
	rootParams[kSRVRootParamGS] = RootDescriptorTableParameter(ARRAYSIZE(descriptorRangesGS), &descriptorRangesGS[0], D3D12_SHADER_VISIBILITY_GEOMETRY);

	RootSignatureDesc rootSignatureDesc(kNumRootParams, &rootParams[0], D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
	m_pRootSignature = new RootSignature(pRenderEnv->m_pDevice, &rootSignatureDesc, L"RenderTiledShadowMapPass::m_pRootSignature");

	GraphicsPipelineStateDesc pipelineStateDesc;
	pipelineStateDesc.SetRootSignature(m_pRootSignature);
	pipelineStateDesc.SetVertexShader(&vertexShader);
	pipelineStateDesc.SetGeometryShader(&geometryShader);
	pipelineStateDesc.InputLayout = *pMeshBatch->GetInputLayout();
	pipelineStateDesc.PrimitiveTopologyType = pMeshBatch->GetPrimitiveTopologyType();
	pipelineStateDesc.DepthStencilState = DepthStencilDesc(DepthStencilDesc::Enabled);
	pipelineStateDesc.SetRenderTargetFormats(0, nullptr, pParams->m_DSVFormat);

	m_pPipelineState = new PipelineState(pRenderEnv->m_pDevice, &pipelineStateDesc, L"RenderTiledShadowMapPass::m_pPipelineState");

	D3D12_INDIRECT_ARGUMENT_DESC argumentDescs[] =
	{
		Constant32BitArgument(kConstant32BitRootParamVS, 0, 1),
		DrawIndexedArgument()
	};
	CommandSignatureDesc commandSignatureDesc(sizeof(DrawMeshCommand), ARRAYSIZE(argumentDescs), &argumentDescs[0]);
	m_pCommandSignature = new CommandSignature(pRenderEnv->m_pDevice, m_pRootSignature, &commandSignatureDesc, L"RenderTiledShadowMapPass::m_pCommandSignature");
}

RenderTiledShadowMapPass::~RenderTiledShadowMapPass()
{
	SafeDelete(m_pCommandSignature);
	SafeDelete(m_pRootSignature);
	SafeDelete(m_pPipelineState);
}

void RenderTiledShadowMapPass::Record(RenderParams* pParams)
{
	RenderEnv* pRenderEnv = pParams->m_pRenderEnv;
	CommandList* pCommandList = pParams->m_pCommandList;
	BindingResourceList* pResources = pParams->m_pResources;
	MeshBatch* pMeshBatch = pParams->m_pMeshBatch;

	pCommandList->Reset(pParams->m_pCommandAllocator, m_pPipelineState);
	pCommandList->SetGraphicsRootSignature(m_pRootSignature);
	
	pCommandList->SetRequiredResourceStates(&pResources->m_RequiredResourceStates);
	pCommandList->SetDescriptorHeaps(pRenderEnv->m_pShaderVisibleSRVHeap);
	pCommandList->SetGraphicsRootDescriptorTable(kCBVRootParamVS, pResources->m_SRVHeapStart);
	pCommandList->SetGraphicsRootDescriptorTable(kSRVRootParamGS, DescriptorHandle(pResources->m_SRVHeapStart, 1));

	D3D12_CPU_DESCRIPTOR_HANDLE dsvHeapStart = pResources->m_DSVHeapStart;
	pCommandList->ClearDepthStencilView(dsvHeapStart, 1.0f);
	pCommandList->OMSetRenderTargets(0, nullptr, FALSE, &dsvHeapStart);

	pCommandList->IASetPrimitiveTopology(pMeshBatch->GetPrimitiveTopology());
	pCommandList->IASetVertexBuffers(0, 1, pMeshBatch->GetVertexBuffer()->GetVBView());
	pCommandList->IASetIndexBuffer(pMeshBatch->GetIndexBuffer()->GetIBView());

	Rect scissorRect(ExtractRect(pParams->m_pViewport));
	pCommandList->RSSetViewports(1, pParams->m_pViewport);
	pCommandList->RSSetScissorRects(1, &scissorRect);

	pCommandList->ExecuteIndirect(m_pCommandSignature, pMeshBatch->GetNumMeshes(), pParams->m_pDrawShadowCasterCommandBuffer, 0, pParams->m_pNumDrawShadowCastersBuffer, 0);
	pCommandList->Close();
}