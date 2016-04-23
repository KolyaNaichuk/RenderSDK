#include "CommandRecorders/CreateVoxelGridRecorder.h"
#include "DX/DXPipelineState.h"
#include "DX/DXRootSignature.h"
#include "DX/DXCommandList.h"
#include "DX/DXResource.h"
#include "DX/DXUtils.h"
#include "DX/DXDescriptorHeap.h"
#include "Common/Mesh.h"
#include "Common/MeshData.h"

enum RootParams
{
	kObjectTransformCBVRootParam = 0,
	kCameraTransformCBVRootParam,
	kGridConfigCBVRootParam,
	kGridBufferUAVRootParam,

#ifdef HAS_TEXCOORD
	kColorSRVRootParam,
	kLinearSamplerRootParam,
#endif // HAS_TEXCOORD

	kNumRootParams
};

CreateVoxelGridRecorder::CreateVoxelGridRecorder(CreateVoxelGridInitParams* pParams)
	: m_pRootSignature(nullptr)
	, m_pPipelineState(nullptr)
{
	// Kolya: fix me
	assert(false);
	/*
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

	DXCBVRange objectTransformCBVRange(1, 0);
	DXCBVRange cameraTransformCBVRange(1, 0);
	DXCBVRange gridConfigCBVRange(1, 0);
	DXUAVRange gridBufferUAVRange(1, 0);

#ifdef HAS_TEXCOORD
	DXSRVRange colorSRVRange(1, 0);
	DXSamplerRange linearSamplerRange(1, 0);
#endif // HAS_TEXCOORD
	
	D3D12_ROOT_PARAMETER rootParams[kNumRootParams];
	rootParams[kObjectTransformCBVRootParam] = DXRootDescriptorTableParameter(1, &objectTransformCBVRange, D3D12_SHADER_VISIBILITY_VERTEX);
	rootParams[kCameraTransformCBVRootParam] = DXRootDescriptorTableParameter(1, &cameraTransformCBVRange, D3D12_SHADER_VISIBILITY_GEOMETRY);
	rootParams[kGridConfigCBVRootParam] = DXRootDescriptorTableParameter(1, &gridConfigCBVRange, D3D12_SHADER_VISIBILITY_PIXEL);
	rootParams[kGridBufferUAVRootParam] = DXRootDescriptorTableParameter(1, &gridBufferUAVRange, D3D12_SHADER_VISIBILITY_PIXEL);

#ifdef HAS_TEXCOORD
	rootParams[kColorSRVRootParam] = DXRootDescriptorTableParameter(1, &colorSRVRange, D3D12_SHADER_VISIBILITY_PIXEL);
	rootParams[kLinearSamplerRootParam] = DXRootDescriptorTableParameter(1, &linearSamplerRange, D3D12_SHADER_VISIBILITY_PIXEL);
#endif // HAS_TEXCOORD
	
	DXRootSignatureDesc rootSignatureDesc(kNumRootParams, rootParams, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
	m_pRootSignature = new DXRootSignature(pParams->m_pDevice, &rootSignatureDesc, L"CreateVoxelGridRecorder::m_pRootSignature");

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
	
	m_pPipelineState = new DXPipelineState(pParams->m_pDevice, &pipelineStateDesc, L"CreateVoxelGridRecorder::m_pPipelineState");
	*/
}

CreateVoxelGridRecorder::~CreateVoxelGridRecorder()
{
	SafeDelete(m_pPipelineState);
	SafeDelete(m_pRootSignature);
}

void CreateVoxelGridRecorder::Record(CreateVoxelGridRecordParams* pParams)
{
	// Kolya: fix me
	assert(false);
	/*
	pParams->m_pCommandList->Reset(pParams->m_pCommandAllocator, m_pPipelineState);
	
	// Kolya: Has to force clear state - otherwise VS Graphics Debugger will fail to make capture
	pParams->m_pCommandList->GetDXObject()->ClearState(m_pPipelineState->GetDXObject());

	pParams->m_pCommandList->SetGraphicsRootSignature(m_pRootSignature);
	
	if (pParams->m_pGridBuffer->GetState() != D3D12_RESOURCE_STATE_UNORDERED_ACCESS)
		pParams->m_pCommandList->TransitionBarrier(pParams->m_pGridBuffer, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

#ifdef HAS_TEXCOORD
	if (pParams->m_pColorTexture->GetState() != D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE)
		pParams->m_pCommandList->TransitionBarrier(pParams->m_pColorTexture, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
#endif // HAS_TEXCOORD
		
	pParams->m_pCommandList->OMSetRenderTargets(0, nullptr);

	pParams->m_pCommandList->SetDescriptorHeaps(pParams->m_pCBVSRVUAVDescriptorHeap);
	pParams->m_pCommandList->SetGraphicsRootDescriptorTable(kObjectTransformCBVRootParam, pParams->m_ObjectTransformCBVHandle);
	pParams->m_pCommandList->SetGraphicsRootDescriptorTable(kCameraTransformCBVRootParam, pParams->m_CameraTransformCBVHandle);
	pParams->m_pCommandList->SetGraphicsRootDescriptorTable(kGridConfigCBVRootParam, pParams->m_GridConfigCBVHandle);
	pParams->m_pCommandList->SetGraphicsRootDescriptorTable(kGridBufferUAVRootParam, pParams->m_GridBufferUAVHandle);

#ifdef HAS_TEXCOORD
	pParams->m_pCommandList->SetGraphicsRootDescriptorTable(kColorSRVRootParam, pParams->m_ColorSRVHandle);
	pParams->m_pCommandList->SetGraphicsRootDescriptorTable(kLinearSamplerRootParam, pParams->m_LinearSamplerHandle);
#endif // HAS_TEXCOORD

	pParams->m_pCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	pParams->m_pCommandList->IASetVertexBuffers(0, 1, pParams->m_pMesh->GetVertexBufferView());
	pParams->m_pCommandList->IASetIndexBuffer(pParams->m_pMesh->GetIndexBufferView());
	
	// Kolya: Hard-coding viewport size for now
	DXViewport viewport(0.0f, 0.0f, 924.0f, 668.0f);
	pParams->m_pCommandList->RSSetViewports(1, &viewport);
	
	DXRect scissorRect(ExtractRect(viewport));
	pParams->m_pCommandList->RSSetScissorRects(1, &scissorRect);
	
	assert(pParams->m_pMesh->GetNumSubMeshes() == 1);
	const SubMeshData* pSubMeshData = pParams->m_pMesh->GetSubMeshes();
	pParams->m_pCommandList->DrawIndexedInstanced(pSubMeshData->m_NumIndices, 1, pSubMeshData->m_IndexStart, 0, 0);

	D3D12_GPU_DESCRIPTOR_HANDLE nullHandle = pParams->m_pCBVSRVUAVDescriptorHeap->GetGPUDescriptor(0);
	pParams->m_pCommandList->SetGraphicsRootDescriptorTable(kGridBufferUAVRootParam, nullHandle);

	pParams->m_pCommandList->Close();
	*/
}
