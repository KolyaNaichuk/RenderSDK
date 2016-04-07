#include "CommandRecorders/FillGBufferRecorder.h"
#include "DX/DXPipelineState.h"
#include "DX/DXRootSignature.h"
#include "DX/DXCommandList.h"
#include "DX/DXResource.h"
#include "DX/DXUtils.h"
#include "Common/MeshData.h"
#include "Common/Mesh.h"

FillGBufferRecorder::FillGBufferRecorder(InitParams* pParams)
	: m_MaterialElementFlags(pParams->m_MaterialElementFlags)
	, m_pRootSignature(nullptr)
	, m_pPipelineState(nullptr)
{
	DXCBVRange transformCBVRange(1, 0);
	DXCBVRange materialCBVRange(1, 0);
	DXSRVRange diffuseSRVRange(1, 0);
	DXSRVRange normalSRVRange(1, 1);
	DXSRVRange specularSRVRange(1, 2);
	DXSamplerRange anisoSamplerRange(1, 0);

	u8 numRootParams = 0;
	std::vector<D3D12_ROOT_PARAMETER> rootParams;
	
	rootParams.push_back(DXRootDescriptorTableParameter(1, &transformCBVRange, D3D12_SHADER_VISIBILITY_VERTEX));
	m_TransformCBVRootParam = numRootParams++;

	rootParams.push_back(DXRootDescriptorTableParameter(1, &materialCBVRange, D3D12_SHADER_VISIBILITY_PIXEL));
	m_MaterialCBVRootParam = numRootParams++;

	std::vector<DXShaderMacro> pixelShaderDefines;
	if (m_MaterialElementFlags != 0)
	{
		rootParams.push_back(DXRootDescriptorTableParameter(1, &anisoSamplerRange, D3D12_SHADER_VISIBILITY_PIXEL));
		m_AnisoSamplerRootParam = numRootParams++;

		if (m_MaterialElementFlags & MaterialElementFlag_DiffuseMap)
		{
			pixelShaderDefines.push_back(DXShaderMacro("DIFFUSE_MAP", "1"));
			rootParams.push_back(DXRootDescriptorTableParameter(1, &diffuseSRVRange, D3D12_SHADER_VISIBILITY_PIXEL));

			m_DiffuseSRVRootParam = numRootParams++;
		}
		if (m_MaterialElementFlags & MaterialElementFlag_NormalMap)
		{
			pixelShaderDefines.push_back(DXShaderMacro("NORMAL_MAP", "1"));
			rootParams.push_back(DXRootDescriptorTableParameter(1, &normalSRVRange, D3D12_SHADER_VISIBILITY_PIXEL));

			m_NormalSRVRootParam = numRootParams++;
		}
		if (m_MaterialElementFlags & MaterialElementFlag_SpecularMap)
		{
			pixelShaderDefines.push_back(DXShaderMacro("SPECULAR_MAP", "1"));
			rootParams.push_back(DXRootDescriptorTableParameter(1, &specularSRVRange, D3D12_SHADER_VISIBILITY_PIXEL));

			m_SpecularSRVRootParam = numRootParams++;
		}
	}
	pixelShaderDefines.push_back(DXShaderMacro());
	
	DXShader vertexShader(L"Shaders//FillGBufferVS.hlsl", "Main", "vs_4_0");
	DXShader pixelShader(L"Shaders//FillGBufferPS.hlsl", "Main", "ps_4_0", &pixelShaderDefines[0]);
	
	DXRootSignatureDesc rootSignatureDesc(numRootParams, &rootParams[0], D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
	m_pRootSignature = new DXRootSignature(pParams->m_pDevice, &rootSignatureDesc, L"FillGBufferRecorder::m_pRootSignature");

	std::vector<DXInputElementDesc> inputElementDescs;
	GenerateInputElements(inputElementDescs, pParams->m_VertexElementFlags, pParams->m_VertexElementFlags);

	DXGraphicsPipelineStateDesc pipelineStateDesc;
	pipelineStateDesc.SetRootSignature(m_pRootSignature);
	pipelineStateDesc.SetVertexShader(&vertexShader);
	pipelineStateDesc.SetPixelShader(&pixelShader);
	pipelineStateDesc.SetInputLayout(inputElementDescs.size(), &inputElementDescs[0]);
	pipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	pipelineStateDesc.DepthStencilState = DXDepthStencilDesc(DXDepthStencilDesc::Enabled);
	
	const DXGI_FORMAT rtvFormats[] = {pParams->m_DiffuseRTVFormat, pParams->m_NormalRTVFormat, pParams->m_SpecularRTVFormat};
	pipelineStateDesc.SetRenderTargetFormats(ARRAYSIZE(rtvFormats), rtvFormats, pParams->m_DSVFormat);

	m_pPipelineState = new DXPipelineState(pParams->m_pDevice, &pipelineStateDesc, L"FillGBufferRecorder::m_pPipelineState");
}

FillGBufferRecorder::~FillGBufferRecorder()
{
	SafeDelete(m_pPipelineState);
	SafeDelete(m_pRootSignature);
}

void FillGBufferRecorder::Record(RenderPassParams* pParams)
{
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

	pCommandList->SetDescriptorHeaps(pParams->m_pCBVDescriptorHeap, pParams->m_pSamplerDescriptorHeap);
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

	Mesh* pMesh = pParams->m_pMesh;
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
}
