#include "RenderPasses/TiledShadingPass.h"
#include "D3DWrapper/GraphicsDevice.h"
#include "D3DWrapper/GraphicsUtils.h"
#include "D3DWrapper/CommandList.h"
#include "D3DWrapper/PipelineState.h"
#include "D3DWrapper/RenderEnv.h"
#include "D3DWrapper/RootSignature.h"

namespace
{
	enum RootParams
	{
		kRootCBVParamVS = 0,
		kRoot32BitConstantsParamVS,
		kRootSRVTableParamVS,

		kRootCBVParamPS,
		kRootSRVTableParamPS,
		kRootSamplerTableParamPS,
		
		kNumRootParams
	};
}

TiledShadingPass::TiledShadingPass(InitParams* pParams)
	: m_pRootSignature(nullptr)
	, m_pPipelineState(nullptr)
{
	InitResources(pParams);
	InitRootSignature(pParams);
	InitPipelineState(pParams);
}

TiledShadingPass::~TiledShadingPass()
{
	SafeDelete(m_pPipelineState);
	SafeDelete(m_pRootSignature);
}

void TiledShadingPass::Record(RenderParams* pParams)
{
	const UINT meshType = 0;
	const UINT numMeshTypes = 1;
	assert(meshType == 0);

	RenderEnv* pRenderEnv = pParams->m_pRenderEnv;
	CommandList* pCommandList = pParams->m_pCommandList;

	pCommandList->Begin(m_pPipelineState);
	pCommandList->SetGraphicsRootSignature(m_pRootSignature);

	if (!m_ResourceBarriers.empty())
		pCommandList->ResourceBarrier((UINT)m_ResourceBarriers.size(), m_ResourceBarriers.data());

	pCommandList->SetDescriptorHeaps(pRenderEnv->m_pShaderVisibleSRVHeap, pRenderEnv->m_pShaderVisibleSamplerHeap);

	const UINT constantsVS[] = {meshType, numMeshTypes};
	pCommandList->SetGraphicsRootConstantBufferView(kRootCBVParamVS, pParams->m_pAppDataBuffer);
	pCommandList->SetGraphicsRoot32BitConstants(kRoot32BitConstantsParamVS, ARRAYSIZE(constantsVS), constantsVS, 0);
	pCommandList->SetGraphicsRootDescriptorTable(kRootSRVTableParamVS, m_SRVHeapStartVS);
	pCommandList->SetGraphicsRootConstantBufferView(kRootCBVParamPS, pParams->m_pAppDataBuffer);
	pCommandList->SetGraphicsRootDescriptorTable(kRootSRVTableParamPS, m_SRVHeapStartPS);
	pCommandList->SetGraphicsRootDescriptorTable(kRootSamplerTableParamPS, m_SamplerHeapStartPS);
		
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHeapStart = m_RTVHeapStart;
	D3D12_CPU_DESCRIPTOR_HANDLE dsvHeapStart = m_DSVHeapStart;
	pCommandList->OMSetRenderTargets(1, &rtvHeapStart, TRUE, &dsvHeapStart);

	pCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	pCommandList->IASetVertexBuffers(0, 1, nullptr);
	pCommandList->IASetIndexBuffer(nullptr);

	Rect scissorRect(ExtractRect(pParams->m_pViewport));
	pCommandList->RSSetViewports(1, pParams->m_pViewport);
	pCommandList->RSSetScissorRects(1, &scissorRect);

	pCommandList->DrawInstanced(4, 1, 0, 0);
	pCommandList->End();
}

void TiledShadingPass::InitResources(InitParams* pParams)
{
	RenderEnv* pRenderEnv = pParams->m_pRenderEnv;

	m_OutputResourceStates.m_AccumLightTextureState = D3D12_RESOURCE_STATE_RENDER_TARGET;
	m_OutputResourceStates.m_MeshTypeDepthTextureState = D3D12_RESOURCE_STATE_DEPTH_WRITE;
	m_OutputResourceStates.m_ShadingRectangleMinPointBufferState = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
	m_OutputResourceStates.m_ShadingRectangleMaxPointBufferState = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
	m_OutputResourceStates.m_DepthTextureState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	m_OutputResourceStates.m_TexCoordTextureState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	m_OutputResourceStates.m_NormalTextureState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	m_OutputResourceStates.m_MaterialIDTextureState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	m_OutputResourceStates.m_FirstResourceIndexPerMaterialIDBufferState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	
	m_OutputResourceStates.m_PointLightWorldBoundsBufferState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	m_OutputResourceStates.m_PointLightPropsBufferState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	m_OutputResourceStates.m_PointLightIndexPerTileBufferState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	m_OutputResourceStates.m_PointLightRangePerTileBufferState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	
	m_OutputResourceStates.m_SpotLightWorldBoundsBufferState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	m_OutputResourceStates.m_SpotLightPropsBufferState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	m_OutputResourceStates.m_SpotLightIndexPerTileBufferState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	m_OutputResourceStates.m_SpotLightRangePerTileBufferState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	
	m_OutputResourceStates.m_IntensityRCoeffsTextureState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	m_OutputResourceStates.m_IntensityGCoeffsTextureState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	m_OutputResourceStates.m_IntensityBCoeffsTextureState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;

	assert(m_ResourceBarriers.empty());
	CreateResourceBarrierIfRequired(pParams->m_pAccumLightTexture,
		pParams->m_InputResourceStates.m_AccumLightTextureState,
		m_OutputResourceStates.m_AccumLightTextureState);

	CreateResourceBarrierIfRequired(pParams->m_pMeshTypeDepthTexture,
		pParams->m_InputResourceStates.m_MeshTypeDepthTextureState,
		m_OutputResourceStates.m_MeshTypeDepthTextureState);

	CreateResourceBarrierIfRequired(pParams->m_pShadingRectangleMinPointBuffer,
		pParams->m_InputResourceStates.m_ShadingRectangleMinPointBufferState,
		m_OutputResourceStates.m_ShadingRectangleMinPointBufferState);

	CreateResourceBarrierIfRequired(pParams->m_pShadingRectangleMaxPointBuffer,
		pParams->m_InputResourceStates.m_ShadingRectangleMaxPointBufferState,
		m_OutputResourceStates.m_ShadingRectangleMaxPointBufferState);

	CreateResourceBarrierIfRequired(pParams->m_pDepthTexture,
		pParams->m_InputResourceStates.m_DepthTextureState,
		m_OutputResourceStates.m_DepthTextureState);

	CreateResourceBarrierIfRequired(pParams->m_pTexCoordTexture,
		pParams->m_InputResourceStates.m_TexCoordTextureState,
		m_OutputResourceStates.m_TexCoordTextureState);

	CreateResourceBarrierIfRequired(pParams->m_pNormalTexture,
		pParams->m_InputResourceStates.m_NormalTextureState,
		m_OutputResourceStates.m_NormalTextureState);

	CreateResourceBarrierIfRequired(pParams->m_pMaterialIDTexture,
		pParams->m_InputResourceStates.m_MaterialIDTextureState,
		m_OutputResourceStates.m_MaterialIDTextureState);

	CreateResourceBarrierIfRequired(pParams->m_pFirstResourceIndexPerMaterialIDBuffer,
		pParams->m_InputResourceStates.m_FirstResourceIndexPerMaterialIDBufferState,
		m_OutputResourceStates.m_FirstResourceIndexPerMaterialIDBufferState);

	if (pParams->m_EnablePointLights)
	{
		CreateResourceBarrierIfRequired(pParams->m_pPointLightWorldBoundsBuffer,
			pParams->m_InputResourceStates.m_PointLightWorldBoundsBufferState,
			m_OutputResourceStates.m_PointLightWorldBoundsBufferState);

		CreateResourceBarrierIfRequired(pParams->m_pPointLightPropsBuffer,
			pParams->m_InputResourceStates.m_PointLightPropsBufferState,
			m_OutputResourceStates.m_PointLightPropsBufferState);

		CreateResourceBarrierIfRequired(pParams->m_pPointLightIndexPerTileBuffer,
			pParams->m_InputResourceStates.m_PointLightIndexPerTileBufferState,
			m_OutputResourceStates.m_PointLightIndexPerTileBufferState);

		CreateResourceBarrierIfRequired(pParams->m_pPointLightRangePerTileBuffer,
			pParams->m_InputResourceStates.m_PointLightRangePerTileBufferState,
			m_OutputResourceStates.m_PointLightRangePerTileBufferState);
	}
	
	if (pParams->m_EnableSpotLights)
	{
		CreateResourceBarrierIfRequired(pParams->m_pSpotLightWorldBoundsBuffer,
			pParams->m_InputResourceStates.m_SpotLightWorldBoundsBufferState,
			m_OutputResourceStates.m_SpotLightWorldBoundsBufferState);

		CreateResourceBarrierIfRequired(pParams->m_pSpotLightPropsBuffer,
			pParams->m_InputResourceStates.m_SpotLightPropsBufferState,
			m_OutputResourceStates.m_SpotLightPropsBufferState);

		CreateResourceBarrierIfRequired(pParams->m_pSpotLightIndexPerTileBuffer,
			pParams->m_InputResourceStates.m_SpotLightIndexPerTileBufferState,
			m_OutputResourceStates.m_SpotLightIndexPerTileBufferState);

		CreateResourceBarrierIfRequired(pParams->m_pSpotLightRangePerTileBuffer,
			pParams->m_InputResourceStates.m_SpotLightRangePerTileBufferState,
			m_OutputResourceStates.m_SpotLightRangePerTileBufferState);
	}

	if (pParams->m_EnableIndirectLight)
	{
		CreateResourceBarrierIfRequired(pParams->m_pIntensityRCoeffsTexture,
			pParams->m_InputResourceStates.m_IntensityRCoeffsTextureState,
			m_OutputResourceStates.m_IntensityRCoeffsTextureState);

		CreateResourceBarrierIfRequired(pParams->m_pIntensityGCoeffsTexture,
			pParams->m_InputResourceStates.m_IntensityGCoeffsTextureState,
			m_OutputResourceStates.m_IntensityGCoeffsTextureState);

		CreateResourceBarrierIfRequired(pParams->m_pIntensityBCoeffsTexture,
			pParams->m_InputResourceStates.m_IntensityBCoeffsTextureState,
			m_OutputResourceStates.m_IntensityBCoeffsTextureState);
	}

	m_SRVHeapStartVS = pRenderEnv->m_pShaderVisibleSRVHeap->Allocate();
	pRenderEnv->m_pDevice->CopyDescriptor(m_SRVHeapStartVS,
		pParams->m_pShadingRectangleMinPointBuffer->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	pRenderEnv->m_pDevice->CopyDescriptor(pRenderEnv->m_pShaderVisibleSRVHeap->Allocate(),
		pParams->m_pShadingRectangleMaxPointBuffer->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	m_SRVHeapStartPS = pRenderEnv->m_pShaderVisibleSRVHeap->Allocate();
	pRenderEnv->m_pDevice->CopyDescriptor(m_SRVHeapStartPS,
		pParams->m_pDepthTexture->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	pRenderEnv->m_pDevice->CopyDescriptor(pRenderEnv->m_pShaderVisibleSRVHeap->Allocate(),
		pParams->m_pTexCoordTexture->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	pRenderEnv->m_pDevice->CopyDescriptor(pRenderEnv->m_pShaderVisibleSRVHeap->Allocate(),
		pParams->m_pNormalTexture->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	pRenderEnv->m_pDevice->CopyDescriptor(pRenderEnv->m_pShaderVisibleSRVHeap->Allocate(),
		pParams->m_pMaterialIDTexture->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	pRenderEnv->m_pDevice->CopyDescriptor(pRenderEnv->m_pShaderVisibleSRVHeap->Allocate(),
		pParams->m_pFirstResourceIndexPerMaterialIDBuffer->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	
	if (pParams->m_EnablePointLights)
	{
		pRenderEnv->m_pDevice->CopyDescriptor(pRenderEnv->m_pShaderVisibleSRVHeap->Allocate(),
			pParams->m_pPointLightWorldBoundsBuffer->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		pRenderEnv->m_pDevice->CopyDescriptor(pRenderEnv->m_pShaderVisibleSRVHeap->Allocate(),
			pParams->m_pPointLightPropsBuffer->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		pRenderEnv->m_pDevice->CopyDescriptor(pRenderEnv->m_pShaderVisibleSRVHeap->Allocate(),
			pParams->m_pPointLightIndexPerTileBuffer->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		pRenderEnv->m_pDevice->CopyDescriptor(pRenderEnv->m_pShaderVisibleSRVHeap->Allocate(),
			pParams->m_pPointLightRangePerTileBuffer->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	}

	if (pParams->m_EnableSpotLights)
	{
		pRenderEnv->m_pDevice->CopyDescriptor(pRenderEnv->m_pShaderVisibleSRVHeap->Allocate(),
			pParams->m_pSpotLightWorldBoundsBuffer->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		pRenderEnv->m_pDevice->CopyDescriptor(pRenderEnv->m_pShaderVisibleSRVHeap->Allocate(),
			pParams->m_pSpotLightPropsBuffer->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		pRenderEnv->m_pDevice->CopyDescriptor(pRenderEnv->m_pShaderVisibleSRVHeap->Allocate(),
			pParams->m_pSpotLightIndexPerTileBuffer->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		pRenderEnv->m_pDevice->CopyDescriptor(pRenderEnv->m_pShaderVisibleSRVHeap->Allocate(),
			pParams->m_pSpotLightRangePerTileBuffer->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	}

	if (pParams->m_EnableIndirectLight)
	{
		pRenderEnv->m_pDevice->CopyDescriptor(pRenderEnv->m_pShaderVisibleSRVHeap->Allocate(),
			pParams->m_pIntensityRCoeffsTexture->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		pRenderEnv->m_pDevice->CopyDescriptor(pRenderEnv->m_pShaderVisibleSRVHeap->Allocate(),
			pParams->m_pIntensityGCoeffsTexture->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		pRenderEnv->m_pDevice->CopyDescriptor(pRenderEnv->m_pShaderVisibleSRVHeap->Allocate(),
			pParams->m_pIntensityBCoeffsTexture->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	}
	
	for (decltype(pParams->m_NumMaterialTextures) index = 0; index < pParams->m_NumMaterialTextures; ++index)
	{
		ColorTexture* pMaterialTexture = pParams->m_ppMaterialTextures[index];

		pRenderEnv->m_pDevice->CopyDescriptor(pRenderEnv->m_pShaderVisibleSRVHeap->Allocate(),
			pMaterialTexture->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	}

	SamplerDesc anisoSamplerDesc(SamplerDesc::Anisotropic);
	Sampler anisoSampler(pRenderEnv, &anisoSamplerDesc);

	SamplerDesc linearSamplerDesc(SamplerDesc::Linear);
	Sampler linearSampler(pRenderEnv, &linearSamplerDesc);

	m_SamplerHeapStartPS = pRenderEnv->m_pShaderVisibleSamplerHeap->Allocate();
	pRenderEnv->m_pDevice->CopyDescriptor(m_SamplerHeapStartPS,
		anisoSampler.GetHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
	
	pRenderEnv->m_pDevice->CopyDescriptor(pRenderEnv->m_pShaderVisibleSamplerHeap->Allocate(),
		linearSampler.GetHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
	
	m_RTVHeapStart = pParams->m_pAccumLightTexture->GetRTVHandle();
	m_DSVHeapStart = pParams->m_pMeshTypeDepthTexture->GetDSVHandle();
}

void TiledShadingPass::InitRootSignature(InitParams* pParams)
{
	assert(m_pRootSignature == nullptr);
	RenderEnv* pRenderEnv = pParams->m_pRenderEnv;
		
	D3D12_ROOT_PARAMETER rootParams[kNumRootParams];
	rootParams[kRootCBVParamVS] = RootCBVParameter(0, D3D12_SHADER_VISIBILITY_VERTEX);
	rootParams[kRoot32BitConstantsParamVS] = Root32BitConstantsParameter(1, D3D12_SHADER_VISIBILITY_VERTEX, 2);
	
	std::vector<D3D12_DESCRIPTOR_RANGE> srvRangesVS = {SRVDescriptorRange(2, 0)};
	rootParams[kRootSRVTableParamVS] = RootDescriptorTableParameter((UINT)srvRangesVS.size(), srvRangesVS.data(), D3D12_SHADER_VISIBILITY_VERTEX);
	
	rootParams[kRootCBVParamPS] = RootCBVParameter(0, D3D12_SHADER_VISIBILITY_PIXEL);

	std::vector<D3D12_DESCRIPTOR_RANGE> srvRangesPS = {SRVDescriptorRange(5, 0)};
	if (pParams->m_EnablePointLights)
		srvRangesPS.push_back(SRVDescriptorRange(4, 5));
	
	if (pParams->m_EnableSpotLights)
		srvRangesPS.push_back(SRVDescriptorRange(4, 9));
	
	if (pParams->m_EnableIndirectLight)
		srvRangesPS.push_back(SRVDescriptorRange(3, 13));
	
	srvRangesPS.push_back(SRVDescriptorRange(pParams->m_NumMaterialTextures, 16));
	rootParams[kRootSRVTableParamPS] = RootDescriptorTableParameter((UINT)srvRangesPS.size(), srvRangesPS.data(), D3D12_SHADER_VISIBILITY_PIXEL);

	std::vector<D3D12_DESCRIPTOR_RANGE> samplerRangesPS = {SamplerRange(2, 0)};
	rootParams[kRootSamplerTableParamPS] = RootDescriptorTableParameter((UINT)samplerRangesPS.size(), samplerRangesPS.data(), D3D12_SHADER_VISIBILITY_PIXEL);

	RootSignatureDesc rootSignatureDesc(kNumRootParams, rootParams);
	m_pRootSignature = new RootSignature(pRenderEnv->m_pDevice, &rootSignatureDesc, L"TiledShadingPass::m_pRootSignature");
}

void TiledShadingPass::InitPipelineState(InitParams* pParams)
{
	assert(m_pRootSignature != nullptr);
	assert(m_pPipelineState == nullptr);

	RenderEnv* pRenderEnv = pParams->m_pRenderEnv;

	std::string shadingModeStr = std::to_string(pParams->m_ShadingMode);
	std::string numMaterialTexturesStr = std::to_string(pParams->m_NumMaterialTextures);
	std::string enablePointLightsStr = std::to_string(pParams->m_EnablePointLights ? 1 : 0);
	std::string enableSpotLightsStr = std::to_string(pParams->m_EnableSpotLights ? 1 : 0);
	std::string enableDirectionalLightStr = std::to_string(pParams->m_EnableDirectionalLight ? 1 : 0);
	std::string enableIndirectLightStr = std::to_string(pParams->m_EnableIndirectLight ? 1 : 0);

	const ShaderMacro shaderDefines[] =
	{
		ShaderMacro("SHADING_MODE", shadingModeStr.c_str()),
		ShaderMacro("NUM_MATERIAL_TEXTURES", numMaterialTexturesStr.c_str()),
		ShaderMacro("ENABLE_POINT_LIGHTS", enablePointLightsStr.c_str()),
		ShaderMacro("ENABLE_SPOT_LIGHTS", enableSpotLightsStr.c_str()),
		ShaderMacro("ENABLE_DIRECTIONAL_LIGHT", enableDirectionalLightStr.c_str()),
		ShaderMacro("ENABLE_INDIRECT_LIGHT", enableIndirectLightStr.c_str()),
		ShaderMacro()
	};
	
	Shader vertexShader(L"Shaders//TiledShadingVS.hlsl", "Main", "vs_4_0");
	Shader pixelShader(L"Shaders//TiledShadingPS.hlsl", "Main", "ps_5_1", shaderDefines);

	DXGI_FORMAT rtvFormat = GetRenderTargetViewFormat(pParams->m_pAccumLightTexture->GetFormat());
	DXGI_FORMAT dsvFormat = GetDepthStencilViewFormat(pParams->m_pMeshTypeDepthTexture->GetFormat());

	GraphicsPipelineStateDesc pipelineStateDesc;
	pipelineStateDesc.SetRootSignature(m_pRootSignature);
	pipelineStateDesc.SetVertexShader(&vertexShader);
	pipelineStateDesc.SetPixelShader(&pixelShader);
	pipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	pipelineStateDesc.DepthStencilState = DepthStencilDesc(DepthStencilDesc::EnabledEqualNoWrites);
	pipelineStateDesc.SetRenderTargetFormat(rtvFormat, dsvFormat);

	m_pPipelineState = new PipelineState(pParams->m_pRenderEnv->m_pDevice, &pipelineStateDesc, L"TiledShadingPass::m_pPipelineState");
}

void TiledShadingPass::CreateResourceBarrierIfRequired(GraphicsResource* pResource, D3D12_RESOURCE_STATES currState, D3D12_RESOURCE_STATES requiredState)
{
	if (currState != requiredState)
		m_ResourceBarriers.emplace_back(pResource, currState, requiredState);
}
