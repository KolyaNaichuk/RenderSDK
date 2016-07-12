#include "RenderPasses/CreateVoxelGridPass.h"
#include "D3DWrapper/D3DPipelineState.h"
#include "D3DWrapper/D3DRootSignature.h"
#include "D3DWrapper/D3DCommandSignature.h"
#include "D3DWrapper/D3DCommandList.h"
#include "D3DWrapper/D3DResource.h"
#include "D3DWrapper/D3DUtils.h"
#include "D3DWrapper/D3DDescriptorHeap.h"
#include "D3DWrapper/D3DRenderEnv.h"
#include "Common/MeshBatch.h"

//#define HAS_TEXCOORD

enum RootParams
{
	kCBVRootParamVS = 0,
	kCBVRootParamGS,
	kSRVRootParamPS,
	k32BitConstantRootParamPS,

#ifdef HAS_TEXCOORD
	kColorSRVRootParam,
	kLinearSamplerRootParam,
#endif // HAS_TEXCOORD

	kNumRootParams
};

CreateVoxelGridPass::CreateVoxelGridPass(InitParams* pParams)
	: m_pRootSignature(nullptr)
	, m_pPipelineState(nullptr)
	, m_pCommandSignature(nullptr)
{
	D3DRenderEnv* pRenderEnv = pParams->m_pRenderEnv;
	MeshBatch* pMeshBatch = pParams->m_pMeshBatch;

	D3DShader vertexShader(L"Shaders//CreateVoxelGridVS.hlsl", "Main", "vs_4_0");
	D3DShader geometryShader(L"Shaders//CreateVoxelGridGS.hlsl", "Main", "gs_4_0");
	D3DShader pixelShader(L"Shaders//CreateVoxelGridPS.hlsl", "Main", "ps_5_1");
	
	D3D12_DESCRIPTOR_RANGE descriptorRangesVS[] = {D3DCBVRange(1, 0)};
	D3D12_DESCRIPTOR_RANGE descriptorRangesGS[] = {D3DCBVRange(1, 0)};
	D3D12_DESCRIPTOR_RANGE descriptorRangesPS[] = {D3DCBVRange(1, 0), D3DSRVRange(1, 0), D3DUAVRange(1, 0)};

#ifdef HAS_TEXCOORD
	D3DSRVRange colorSRVRange(1, 0);
	D3DSamplerRange linearSamplerRange(1, 0);
#endif // HAS_TEXCOORD
	
	D3D12_ROOT_PARAMETER rootParams[kNumRootParams];
	rootParams[kCBVRootParamVS] = D3DRootDescriptorTableParameter(ARRAYSIZE(descriptorRangesVS), &descriptorRangesVS[0], D3D12_SHADER_VISIBILITY_VERTEX);
	rootParams[kCBVRootParamGS] = D3DRootDescriptorTableParameter(ARRAYSIZE(descriptorRangesGS), &descriptorRangesGS[0], D3D12_SHADER_VISIBILITY_GEOMETRY);
	rootParams[kSRVRootParamPS] = D3DRootDescriptorTableParameter(ARRAYSIZE(descriptorRangesPS), &descriptorRangesPS[0], D3D12_SHADER_VISIBILITY_PIXEL);
	rootParams[k32BitConstantRootParamPS] = D3DRoot32BitConstantsParameter(1, D3D12_SHADER_VISIBILITY_PIXEL, 1);
		
#ifdef HAS_TEXCOORD
	rootParams[kColorSRVRootParam] = D3DRootDescriptorTableParameter(1, &colorSRVRange, D3D12_SHADER_VISIBILITY_PIXEL);
	rootParams[kLinearSamplerRootParam] = D3DRootDescriptorTableParameter(1, &linearSamplerRange, D3D12_SHADER_VISIBILITY_PIXEL);
#endif // HAS_TEXCOORD
	
	D3DRootSignatureDesc rootSignatureDesc(kNumRootParams, rootParams, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
	m_pRootSignature = new D3DRootSignature(pRenderEnv->m_pDevice, &rootSignatureDesc, L"CreateVoxelGridPass::m_pRootSignature");
		
	D3DGraphicsPipelineStateDesc pipelineStateDesc;
	pipelineStateDesc.SetRootSignature(m_pRootSignature);
	pipelineStateDesc.SetVertexShader(&vertexShader);
	pipelineStateDesc.SetGeometryShader(&geometryShader);
	pipelineStateDesc.SetPixelShader(&pixelShader);
	pipelineStateDesc.RasterizerState = D3DRasterizerDesc(D3DRasterizerDesc::CullNoneConservative);
	pipelineStateDesc.InputLayout = *pMeshBatch->GetInputLayout();
	pipelineStateDesc.PrimitiveTopologyType = pMeshBatch->GetPrimitiveTopologyType();
	
	m_pPipelineState = new D3DPipelineState(pRenderEnv->m_pDevice, &pipelineStateDesc, L"CreateVoxelGridPass::m_pPipelineState");

	D3D12_INDIRECT_ARGUMENT_DESC argumentDescs[] =
	{
		D3D32BitConstantsArgument(k32BitConstantRootParamPS, 0, 1),
		D3DDrawIndexedArgument()
	};
	D3DCommandSignatureDesc commandSignatureDesc(sizeof(DrawMeshCommand), ARRAYSIZE(argumentDescs), &argumentDescs[0]);
	m_pCommandSignature = new D3DCommandSignature(pRenderEnv->m_pDevice, m_pRootSignature, &commandSignatureDesc, L"RenderGBufferPass::m_pCommandSignature");
}

CreateVoxelGridPass::~CreateVoxelGridPass()
{
	SafeDelete(m_pCommandSignature);
	SafeDelete(m_pPipelineState);
	SafeDelete(m_pRootSignature);
}

void CreateVoxelGridPass::Record(RenderParams* pParams)
{
	D3DRenderEnv* pRenderEnv = pParams->m_pRenderEnv;
	D3DCommandList* pCommandList = pParams->m_pCommandList;
	D3DResourceList* pResources = pParams->m_pResources;
	MeshBatch* pMeshBatch = pParams->m_pMeshBatch;

	pCommandList->Reset(pParams->m_pCommandAllocator, m_pPipelineState);
	pCommandList->SetGraphicsRootSignature(m_pRootSignature);
			
	pCommandList->OMSetRenderTargets(0, nullptr);
	
	pCommandList->SetResourceTransitions(&pResources->m_RequiredResourceStates);
	pCommandList->SetDescriptorHeaps(pRenderEnv->m_pShaderVisibleSRVHeap);

	pCommandList->SetGraphicsRootDescriptorTable(kCBVRootParamVS, pResources->m_SRVHeapStart);
	pCommandList->SetGraphicsRootDescriptorTable(kCBVRootParamGS, D3DDescriptorHandle(pResources->m_SRVHeapStart, 1));
	pCommandList->SetGraphicsRootDescriptorTable(kSRVRootParamPS, D3DDescriptorHandle(pResources->m_SRVHeapStart, 2));
	
	pCommandList->IASetPrimitiveTopology(pMeshBatch->GetPrimitiveTopology());
	pCommandList->IASetVertexBuffers(0, 1, pMeshBatch->GetVertexBuffer()->GetVBView());
	pCommandList->IASetIndexBuffer(pMeshBatch->GetIndexBuffer()->GetIBView());
	
	pCommandList->RSSetViewports(1, pParams->m_pViewport);
	
	D3DRect scissorRect(ExtractRect(pParams->m_pViewport));
	pCommandList->RSSetScissorRects(1, &scissorRect);
	
	pCommandList->ExecuteIndirect(m_pCommandSignature, pMeshBatch->GetNumMeshes(), pParams->m_pDrawMeshCommandBuffer, 0, pParams->m_pNumDrawMeshesBuffer, 0);
	pCommandList->Close();
}
