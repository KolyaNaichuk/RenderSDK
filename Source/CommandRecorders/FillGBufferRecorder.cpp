#include "CommandRecorders/FillGBufferRecorder.h"
#include "Common/MeshBatch.h"
#include "DX/DXPipelineState.h"
#include "DX/DXRootSignature.h"
#include "DX/DXCommandList.h"
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
}

FillGBufferRecorder::~FillGBufferRecorder()
{
	SafeDelete(m_pPipelineState);
	SafeDelete(m_pRootSignature);
}

void FillGBufferRecorder::Record(RenderPassParams* pParams)
{
	// Kolya: fix me
	assert(false);
	/*
	DXCommandList* pCommandList = pParams->m_pCommandList;
	
	pCommandList->Reset(pParams->m_pCommandAllocator, m_pPipelineState);
	// Kolya: Has to force clear state - otherwise VS Graphics Debugger will fail to make capture
	pCommandList->GetDXObject()->ClearState(m_pPipelineState->GetDXObject());
	pCommandList->SetGraphicsRootSignature(m_pRootSignature);
	
	GBuffer* pGBuffer = pParams->m_pGBuffer;
	if (pGBuffer->m_pDiffuseTexture->GetState() != D3D12_RESOURCE_STATE_RENDER_TARGET)
		pCommandList->TransitionBarrier(pGBuffer->m_pDiffuseTexture, D3D12_RESOURCE_STATE_RENDER_TARGET);
	
	if (pGBuffer->m_pNormalTexture->GetState() != D3D12_RESOURCE_STATE_RENDER_TARGET)
		pCommandList->TransitionBarrier(pGBuffer->m_pNormalTexture, D3D12_RESOURCE_STATE_RENDER_TARGET);

	if (pGBuffer->m_pSpecularTexture->GetState() != D3D12_RESOURCE_STATE_RENDER_TARGET)
		pCommandList->TransitionBarrier(pGBuffer->m_pSpecularTexture, D3D12_RESOURCE_STATE_RENDER_TARGET);
	
	if (pGBuffer->m_pDepthTexture->GetState() != D3D12_RESOURCE_STATE_DEPTH_WRITE)
		pCommandList->TransitionBarrier(pGBuffer->m_pDepthTexture, D3D12_RESOURCE_STATE_DEPTH_WRITE);

	pCommandList->SetDescriptorHeaps(pParams->m_pCBVDescriptorHeap, pParams->m_pShaderInvisibleSamplerHeap);
	pCommandList->SetGraphicsRootDescriptorTable(m_TransformCBVRootParam, pParams->m_TransformCBVHandle);
	pCommandList->SetGraphicsRootDescriptorTable(m_MaterialCBVRootParam, pParams->m_MaterialCBVHandle);

	if (m_MaterialElementFlags != 0)
	{
		Material* pMaterial = pParams->m_pMaterial;
		pCommandList->SetGraphicsRootDescriptorTable(m_AnisoSamplerRootParam, pParams->m_AnisoSamplerHandle);
		
		if (m_MaterialElementFlags & MaterialElementFlag_DiffuseMap)
		{
			if (pMaterial->m_pDiffuseMap->GetState() != D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE)
				pCommandList->TransitionBarrier(pMaterial->m_pDiffuseMap, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
			
			pCommandList->SetGraphicsRootDescriptorTable(m_DiffuseSRVRootParam, pMaterial->m_DiffuseSRVHandle);
		}
		if (m_MaterialElementFlags & MaterialElementFlag_NormalMap)
		{
			if (pMaterial->m_pNormalMap->GetState() != D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE)
				pCommandList->TransitionBarrier(pMaterial->m_pNormalMap, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

			pCommandList->SetGraphicsRootDescriptorTable(m_NormalSRVRootParam, pMaterial->m_NormalSRVHandle);
		}
		if (m_MaterialElementFlags & MaterialElementFlag_SpecularMap)
		{
			if (pMaterial->m_pSpecularMap->GetState() != D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE)
				pCommandList->TransitionBarrier(pMaterial->m_pSpecularMap, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

			pCommandList->SetGraphicsRootDescriptorTable(m_NormalSRVRootParam, pMaterial->m_SpecualSRVHandle);
		}
	}

	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandles[] =
	{
		pGBuffer->m_DiffuseRTVHandle,
		pGBuffer->m_NormalRTVHandle,
		pGBuffer->m_SpecularRTVHandle
	};
	pCommandList->OMSetRenderTargets(ARRAYSIZE(rtvHandles), rtvHandles, TRUE, &pGBuffer->m_DSVHandle);

	Mesh* pMesh = pParams->m_pMeshBatch;
	pCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	pCommandList->IASetVertexBuffers(0, 1, pMesh->GetVertexBufferView());
	pCommandList->IASetIndexBuffer(pMesh->GetIndexBufferView());

	DXViewport viewport(0.0f, 0.0f, FLOAT(pGBuffer->m_pDiffuseTexture->GetWidth()), FLOAT(pGBuffer->m_pDiffuseTexture->GetHeight()));
	pCommandList->RSSetViewports(1, &viewport);

	DXRect scissorRect(ExtractRect(viewport));
	pCommandList->RSSetScissorRects(1, &scissorRect);

	assert(pMesh->GetNumSubMeshes() == 1);
	const SubMeshData* pSubMeshData = pMesh->GetSubMeshes();
	pCommandList->DrawIndexedInstanced(pSubMeshData->m_NumIndices, 1, pSubMeshData->m_IndexStart, 0, 0);

	pCommandList->Close();
	*/
}
