#include "CommandRecorders/CreateVoxelGridRecorder.h"
#include "DX/DXPipelineState.h"
#include "DX/DXRootSignature.h"
#include "DX/DXCommandList.h"
#include "DX/DXResource.h"
#include "DX/DXUtils.h"
#include "DX/DXDescriptorHeap.h"
#include "DX/DXRenderEnvironment.h"
#include "Common/Mesh.h"
#include "Common/MeshData.h"

enum RootParams
{
	kCBVRootParamVS = 0,
	kCBVRootParamGS,
	kSRVRootParamPS,

#ifdef HAS_TEXCOORD
	kColorSRVRootParam,
	kLinearSamplerRootParam,
#endif // HAS_TEXCOORD

	kNumRootParams
};

CreateVoxelGridRecorder::CreateVoxelGridRecorder(InitParams* pParams)
	: m_pRootSignature(nullptr)
	, m_pPipelineState(nullptr)
{
	DXRenderEnvironment* pEnv = pParams->m_pEnv;

	u8 inputElementFlags = 0;
	inputElementFlags |= VertexElementFlag_Position;
	inputElementFlags |= VertexElementFlag_Normal;

#ifdef HAS_TEXCOORD
	inputElementFlags |= VertexElementFlag_TexCoords;
	assert(false && "Needs further impl");
#else // HAS_TEXCOORD	
	inputElementFlags |= VertexElementFlag_Color;
#endif // HAS_TEXCOORD

	DXShader vertexShader(L"Shaders//CreateVoxelGridVS.hlsl", "Main", "vs_4_0");
	DXShader geometryShader(L"Shaders//CreateVoxelGridGS.hlsl", "Main", "gs_4_0");
	DXShader pixelShader(L"Shaders//CreateVoxelGridPS.hlsl", "Main", "ps_5_1");
	
	D3D12_DESCRIPTOR_RANGE descriptorRangesVS[] = {DXCBVRange(1, 0)};
	D3D12_DESCRIPTOR_RANGE descriptorRangesGS[] = {DXCBVRange(1, 0)};
	D3D12_DESCRIPTOR_RANGE descriptorRangesPS[] = {DXCBVRange(1, 0), DXUAVRange(1, 0)};

#ifdef HAS_TEXCOORD
	DXSRVRange colorSRVRange(1, 0);
	DXSamplerRange linearSamplerRange(1, 0);
#endif // HAS_TEXCOORD
	
	D3D12_ROOT_PARAMETER rootParams[kNumRootParams];
	rootParams[kCBVRootParamVS] = DXRootDescriptorTableParameter(ARRAYSIZE(descriptorRangesVS), &descriptorRangesVS[0], D3D12_SHADER_VISIBILITY_VERTEX);
	rootParams[kCBVRootParamGS] = DXRootDescriptorTableParameter(ARRAYSIZE(descriptorRangesGS), &descriptorRangesGS[0], D3D12_SHADER_VISIBILITY_GEOMETRY);
	rootParams[kSRVRootParamPS] = DXRootDescriptorTableParameter(ARRAYSIZE(descriptorRangesPS), &descriptorRangesPS[0], D3D12_SHADER_VISIBILITY_PIXEL);
		
#ifdef HAS_TEXCOORD
	rootParams[kColorSRVRootParam] = DXRootDescriptorTableParameter(1, &colorSRVRange, D3D12_SHADER_VISIBILITY_PIXEL);
	rootParams[kLinearSamplerRootParam] = DXRootDescriptorTableParameter(1, &linearSamplerRange, D3D12_SHADER_VISIBILITY_PIXEL);
#endif // HAS_TEXCOORD
	
	DXRootSignatureDesc rootSignatureDesc(kNumRootParams, rootParams, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
	m_pRootSignature = new DXRootSignature(pEnv->m_pDevice, &rootSignatureDesc, L"CreateVoxelGridRecorder::m_pRootSignature");

	std::vector<DXInputElementDesc> inputElementDescs;
	GenerateInputElements(inputElementDescs, inputElementFlags, pParams->m_VertexElementFlags);
	
	DXGraphicsPipelineStateDesc pipelineStateDesc;
	pipelineStateDesc.SetRootSignature(m_pRootSignature);
	pipelineStateDesc.SetVertexShader(&vertexShader);
	pipelineStateDesc.SetGeometryShader(&geometryShader);
	pipelineStateDesc.SetPixelShader(&pixelShader);
	pipelineStateDesc.RasterizerState = DXRasterizerDesc(DXRasterizerDesc::CullNoneConservative);
	pipelineStateDesc.SetInputLayout(inputElementDescs.size(), &inputElementDescs[0]);
	pipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	
	m_pPipelineState = new DXPipelineState(pEnv->m_pDevice, &pipelineStateDesc, L"CreateVoxelGridRecorder::m_pPipelineState");
}

CreateVoxelGridRecorder::~CreateVoxelGridRecorder()
{
	SafeDelete(m_pPipelineState);
	SafeDelete(m_pRootSignature);
}

void CreateVoxelGridRecorder::Record(RenderPassParams* pParams)
{
	DXRenderEnvironment* pEnv = pParams->m_pEnv;
	DXCommandList* pCommandList = pParams->m_pCommandList;
	DXBindingResourceList* pResources = pParams->m_pResources;
	Mesh* pMesh = pParams->m_pMesh;

	pCommandList->Reset(pParams->m_pCommandAllocator, m_pPipelineState);
	pCommandList->SetGraphicsRootSignature(m_pRootSignature);
			
	pCommandList->OMSetRenderTargets(0, nullptr);
	
	pCommandList->SetResourceTransitions(&pResources->m_ResourceTransitions);
	pCommandList->SetDescriptorHeaps(pEnv->m_pShaderVisibleSRVHeap);

	DXDescriptorHandle srvHeapStart = pResources->m_SRVHeapStart;
	pCommandList->SetGraphicsRootDescriptorTable(kCBVRootParamVS, srvHeapStart);

	srvHeapStart.Offset(1);
	pCommandList->SetGraphicsRootDescriptorTable(kCBVRootParamGS, srvHeapStart);

	srvHeapStart.Offset(1);
	pCommandList->SetGraphicsRootDescriptorTable(kSRVRootParamPS, srvHeapStart);	
	
	pCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	pCommandList->IASetVertexBuffers(0, 1, pMesh->GetVertexBuffer()->GetVBView());
	pCommandList->IASetIndexBuffer(pMesh->GetIndexBuffer()->GetIBView());
	
	pCommandList->RSSetViewports(1, pParams->m_pViewport);
	
	DXRect scissorRect(ExtractRect(pParams->m_pViewport));
	pCommandList->RSSetScissorRects(1, &scissorRect);
	
	assert(pMesh->GetNumSubMeshes() == 1);
	const SubMeshData* pSubMeshData = pMesh->GetSubMeshes();
	pCommandList->DrawIndexedInstanced(pSubMeshData->m_NumIndices, 1, pSubMeshData->m_IndexStart, 0, 0);

	pCommandList->SetGraphicsRootDescriptorTable(kSRVRootParamPS, pEnv->m_NullSRVHeapStart);
	pCommandList->Close();
}
