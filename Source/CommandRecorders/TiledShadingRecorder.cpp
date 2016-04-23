#include "CommandRecorders/TiledShadingRecorder.h"
#include "CommandRecorders/FillGBufferRecorder.h"
#include "DX/DXPipelineState.h"
#include "DX/DXRootSignature.h"
#include "DX/DXCommandList.h"
#include "DX/DXCommandAllocator.h"
#include "DX/DXResource.h"

TiledShadingRecorder::TiledShadingRecorder(InitParams* pParams)
	: m_pRootSignature(nullptr)
	, m_pPipelineState(nullptr)
	, m_NumThreadGroupsX(pParams->m_NumTilesX)
	, m_NumThreadGroupsY(pParams->m_NumTilesY)
{
	// Kolya: fix me
	assert(false);
	/*
	DXCBVRange shadingDataCBVRange(1, 0);
	DXSRVRange depthSRVRange(1, 0);
	DXSRVRange normalSRVRange(1, 1);
	DXSRVRange diffuseSRVRange(1, 2);
	DXSRVRange specularSRVRange(1, 3);
	DXSRVRange pointLightGeometrySRVRange(1, 4);
	DXSRVRange pointLightPropsSRVRange(1, 5);
	DXSRVRange spotLightGeometrySRVRange(1, 6);
	DXSRVRange spotLightPropsSRVRange(1, 7);
	DXUAVRange accumLightUAVRange(1, 0);

	u8 numRootParams = 0;
	std::vector<D3D12_ROOT_PARAMETER> rootParams;
	
	rootParams.push_back(DXRootDescriptorTableParameter(1, &shadingDataCBVRange, D3D12_SHADER_VISIBILITY_ALL));
	m_ShadingDataCBVRootParam = numRootParams++;

	rootParams.push_back(DXRootDescriptorTableParameter(1, &accumLightUAVRange, D3D12_SHADER_VISIBILITY_ALL));
	m_AccumLightUAVRootParam = numRootParams++;

	rootParams.push_back(DXRootDescriptorTableParameter(1, &depthSRVRange, D3D12_SHADER_VISIBILITY_ALL));
	m_DepthSRVRootParam = numRootParams++;

	rootParams.push_back(DXRootDescriptorTableParameter(1, &normalSRVRange, D3D12_SHADER_VISIBILITY_ALL));
	m_NormalSRVRootParam = numRootParams++;

	rootParams.push_back(DXRootDescriptorTableParameter(1, &diffuseSRVRange, D3D12_SHADER_VISIBILITY_ALL));
	m_DiffuseSRVRootParam = numRootParams++;

	rootParams.push_back(DXRootDescriptorTableParameter(1, &specularSRVRange, D3D12_SHADER_VISIBILITY_ALL));
	m_SpecularSRVRootParam = numRootParams++;

	if (pParams->m_NumPointLights > 0)
	{
		rootParams.push_back(DXRootDescriptorTableParameter(1, &pointLightGeometrySRVRange, D3D12_SHADER_VISIBILITY_ALL));
		m_PointLightGeometrySRVRootParam = numRootParams++;

		rootParams.push_back(DXRootDescriptorTableParameter(1, &pointLightPropsSRVRange, D3D12_SHADER_VISIBILITY_ALL));
		m_PointLightPropsSRVRootParam = numRootParams++;
	}

	if (pParams->m_NumSpotLights > 0)
	{
		rootParams.push_back(DXRootDescriptorTableParameter(1, &spotLightGeometrySRVRange, D3D12_SHADER_VISIBILITY_ALL));
		m_SpotLightGeometrySRVRootParam = numRootParams++;

		rootParams.push_back(DXRootDescriptorTableParameter(1, &spotLightPropsSRVRange, D3D12_SHADER_VISIBILITY_ALL));
		m_SpotLightPropsSRVRootParam = numRootParams++;
	}

	DXRootSignatureDesc rootSignatureDesc(numRootParams, &rootParams[0]);
	m_pRootSignature = new DXRootSignature(pParams->m_pDevice, &rootSignatureDesc, L"TiledShadingRecorder::m_pRootSignature");
	
	const u16 tileSize = 16;

	std::string tileSizeStr = std::to_string(tileSize);
	std::string shadingModeStr = std::to_string(pParams->m_ShadingMode);
	std::string numPointLightsStr = std::to_string(pParams->m_NumPointLights);
	std::string numSpotLightsStr = std::to_string(pParams->m_NumSpotLights);
	std::string useDirectLightStr = std::to_string(pParams->m_UseDirectLight ? 1 : 0);
	
	const DXShaderMacro shaderDefines[] =
	{
		DXShaderMacro("TILE_SIZE", tileSizeStr.c_str()),
		DXShaderMacro("SHADING_MODE", shadingModeStr.c_str()),
		DXShaderMacro("NUM_POINT_LIGHTS", numPointLightsStr.c_str()),
		DXShaderMacro("NUM_SPOT_LIGHTS", numSpotLightsStr.c_str()),
		DXShaderMacro("USE_DIRECT_LIGHT", useDirectLightStr.c_str()),
		DXShaderMacro()
	};
	DXShader computeShader(L"Shaders//TiledShadingCS.hlsl", "Main", "cs_5_0", shaderDefines);

	DXComputePipelineStateDesc pipelineStateDesc;
	pipelineStateDesc.SetRootSignature(m_pRootSignature);
	pipelineStateDesc.SetComputeShader(&computeShader);

	m_pPipelineState = new DXPipelineState(pParams->m_pDevice, &pipelineStateDesc, L"TiledShadingRecorder::m_pPipelineState");
	*/
}

TiledShadingRecorder::~TiledShadingRecorder()
{
	SafeDelete(m_pPipelineState);
	SafeDelete(m_pRootSignature);
}

void TiledShadingRecorder::Record(RenderPassParams* pParams)
{
	// Kolya: fix me
	assert(false);
	/*
	GBuffer* pGBuffer = pParams->m_pGBuffer;
	DXCommandList* pCommandList = pParams->m_pCommandList;

	pCommandList->Reset(pParams->m_pCommandAllocator, m_pPipelineState);
	pCommandList->SetGraphicsRootSignature(m_pRootSignature);
			
	if (pGBuffer->m_pAccumLightTexture->GetState() != D3D12_RESOURCE_STATE_UNORDERED_ACCESS)
		pCommandList->TransitionBarrier(pGBuffer->m_pAccumLightTexture, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

	if (pGBuffer->m_pDiffuseTexture->GetState() != D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE)
		pCommandList->TransitionBarrier(pGBuffer->m_pDiffuseTexture, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

	if (pGBuffer->m_pNormalTexture->GetState() != D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE)
		pCommandList->TransitionBarrier(pGBuffer->m_pNormalTexture, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

	if (pGBuffer->m_pSpecularTexture->GetState() != D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE)
		pCommandList->TransitionBarrier(pGBuffer->m_pSpecularTexture, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

	if (pGBuffer->m_pDepthTexture->GetState() != D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE)
		pCommandList->TransitionBarrier(pGBuffer->m_pDepthTexture, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

	if ((pParams->m_pPointLightGeometryBuffer != nullptr) && (pParams->m_pPointLightPropsBuffer != nullptr))
	{
		if (pParams->m_pPointLightGeometryBuffer->GetState() != D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE)
			pCommandList->TransitionBarrier(pParams->m_pPointLightGeometryBuffer, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

		if (pParams->m_pPointLightPropsBuffer->GetState() != D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE)
			pCommandList->TransitionBarrier(pParams->m_pPointLightPropsBuffer, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	}
	if ((pParams->m_SpotLightGeometryBuffer != nullptr) && (pParams->m_SpotLightPropsBuffer != nullptr))
	{
		if (pParams->m_SpotLightGeometryBuffer->GetState() != D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE)
			pCommandList->TransitionBarrier(pParams->m_SpotLightGeometryBuffer, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		
		if (pParams->m_SpotLightPropsBuffer->GetState() != D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE)
			pCommandList->TransitionBarrier(pParams->m_SpotLightPropsBuffer, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	}	

	pCommandList->SetDescriptorHeaps(pParams->m_pCBVSRVUAVDescriptorHeap);
	
	pCommandList->SetGraphicsRootDescriptorTable(m_ShadingDataCBVRootParam, pParams->m_ShadingDataCBVHandle);
	pCommandList->SetGraphicsRootDescriptorTable(m_AccumLightUAVRootParam, pGBuffer->m_AccumLightUAVHandle);
	
	pCommandList->SetGraphicsRootDescriptorTable(m_DepthSRVRootParam, pGBuffer->m_DepthSRVHandle);
	pCommandList->SetGraphicsRootDescriptorTable(m_NormalSRVRootParam, pGBuffer->m_NormalSRVHandle);
	pCommandList->SetGraphicsRootDescriptorTable(m_DiffuseSRVRootParam, pGBuffer->m_DiffuseSRVHandle);
	pCommandList->SetGraphicsRootDescriptorTable(m_SpecularSRVRootParam, pGBuffer->m_SpecularSRVHandle);

	if ((pParams->m_pPointLightGeometryBuffer != nullptr) && (pParams->m_pPointLightPropsBuffer != nullptr))
	{
		pCommandList->SetGraphicsRootDescriptorTable(m_PointLightGeometrySRVRootParam, pParams->m_PointLightGeometrySRVHandle);
		pCommandList->SetGraphicsRootDescriptorTable(m_PointLightPropsSRVRootParam, pParams->m_PointLightPropsSRVHandle);
	}
	if ((pParams->m_SpotLightGeometryBuffer != nullptr) && (pParams->m_SpotLightPropsBuffer != nullptr))
	{
		pCommandList->SetGraphicsRootDescriptorTable(m_SpotLightGeometrySRVRootParam, pParams->m_SpotLightGeometrySRVHandle);
		pCommandList->SetGraphicsRootDescriptorTable(m_SpotLightPropsSRVRootParam, pParams->m_SpotLightPropsSRVHandle);
	}
		
	pCommandList->Dispatch(m_NumThreadGroupsX, m_NumThreadGroupsY, 1);
	pCommandList->Close();
	*/
}
