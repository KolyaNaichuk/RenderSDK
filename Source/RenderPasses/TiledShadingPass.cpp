#include "RenderPasses/TiledShadingPass.h"
#include "D3DWrapper/GraphicsDevice.h"
#include "D3DWrapper/GraphicsUtils.h"
#include "D3DWrapper/CommandList.h"
#include "D3DWrapper/PipelineState.h"
#include "D3DWrapper/RenderEnv.h"
#include "D3DWrapper/RootSignature.h"
#include "Profiler/GPUProfiler.h"

namespace
{
	enum RootParams
	{
		kRoot32BitConstantsParamVS = 0,
		kRootCBVParamPS,
		kRootSRVTableParamPS,
		kRootSamplerTableParamPS,
		kNumRootParams
	};
}

TiledShadingPass::TiledShadingPass(InitParams* pParams)
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
	GPUProfiler* pGPUProfiler = pRenderEnv->m_pGPUProfiler;

	pCommandList->Begin(m_pPipelineState);
#ifdef ENABLE_PROFILING
	u32 profileIndex = pGPUProfiler->StartProfile(pCommandList, "TiledShadingPass");
#endif // ENABLE_PROFILING

	pCommandList->SetGraphicsRootSignature(m_pRootSignature);

	if (!m_ResourceBarriers.empty())
		pCommandList->ResourceBarrier((UINT)m_ResourceBarriers.size(), m_ResourceBarriers.data());

	pCommandList->SetDescriptorHeaps(pRenderEnv->m_pShaderVisibleSRVHeap, pRenderEnv->m_pShaderVisibleSamplerHeap);
	pCommandList->SetGraphicsRoot32BitConstant(kRoot32BitConstantsParamVS, meshType, 0);
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

	pCommandList->DrawInstanced(3, 1, 0, 0);

#ifdef ENABLE_PROFILING
	pGPUProfiler->EndProfile(pCommandList, profileIndex);
#endif // ENABLE_PROFILING
	pCommandList->End();
}

void TiledShadingPass::InitResources(InitParams* pParams)
{
	RenderEnv* pRenderEnv = pParams->m_pRenderEnv;

	m_OutputResourceStates.m_AccumLightTextureState = D3D12_RESOURCE_STATE_RENDER_TARGET;
	m_OutputResourceStates.m_MeshTypeDepthTextureState = D3D12_RESOURCE_STATE_DEPTH_WRITE;
	m_OutputResourceStates.m_DepthTextureState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	m_OutputResourceStates.m_GBuffer1State = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	m_OutputResourceStates.m_GBuffer2State = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	m_OutputResourceStates.m_GBuffer3State = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	m_OutputResourceStates.m_GBuffer4State = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	m_OutputResourceStates.m_MaterialTextureIndicesBufferState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	m_OutputResourceStates.m_SpotLightPropsBufferState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	m_OutputResourceStates.m_SpotLightIndexPerTileBufferState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	m_OutputResourceStates.m_SpotLightRangePerTileBufferState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	m_OutputResourceStates.m_SpotLightShadowMapsState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	m_OutputResourceStates.m_SpotLightViewProjMatrixBufferState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	
	assert(m_ResourceBarriers.empty());
	AddResourceBarrierIfRequired(pParams->m_pAccumLightTexture,
		pParams->m_InputResourceStates.m_AccumLightTextureState,
		m_OutputResourceStates.m_AccumLightTextureState);

	AddResourceBarrierIfRequired(pParams->m_pMeshTypeDepthTexture,
		pParams->m_InputResourceStates.m_MeshTypeDepthTextureState,
		m_OutputResourceStates.m_MeshTypeDepthTextureState);

	AddResourceBarrierIfRequired(pParams->m_pDepthTexture,
		pParams->m_InputResourceStates.m_DepthTextureState,
		m_OutputResourceStates.m_DepthTextureState);

	AddResourceBarrierIfRequired(pParams->m_pGBuffer1,
		pParams->m_InputResourceStates.m_GBuffer1State,
		m_OutputResourceStates.m_GBuffer1State);

	AddResourceBarrierIfRequired(pParams->m_pGBuffer2,
		pParams->m_InputResourceStates.m_GBuffer2State,
		m_OutputResourceStates.m_GBuffer2State);

	AddResourceBarrierIfRequired(pParams->m_pGBuffer3,
		pParams->m_InputResourceStates.m_GBuffer3State,
		m_OutputResourceStates.m_GBuffer3State);

	AddResourceBarrierIfRequired(pParams->m_pGBuffer4,
		pParams->m_InputResourceStates.m_GBuffer4State,
		m_OutputResourceStates.m_GBuffer4State);

	AddResourceBarrierIfRequired(pParams->m_pMaterialTextureIndicesBuffer,
		pParams->m_InputResourceStates.m_MaterialTextureIndicesBufferState,
		m_OutputResourceStates.m_MaterialTextureIndicesBufferState);

	if (pParams->m_EnableSpotLights)
	{
		AddResourceBarrierIfRequired(pParams->m_pSpotLightPropsBuffer,
			pParams->m_InputResourceStates.m_SpotLightPropsBufferState,
			m_OutputResourceStates.m_SpotLightPropsBufferState);

		AddResourceBarrierIfRequired(pParams->m_pSpotLightIndexPerTileBuffer,
			pParams->m_InputResourceStates.m_SpotLightIndexPerTileBufferState,
			m_OutputResourceStates.m_SpotLightIndexPerTileBufferState);

		AddResourceBarrierIfRequired(pParams->m_pSpotLightRangePerTileBuffer,
			pParams->m_InputResourceStates.m_SpotLightRangePerTileBufferState,
			m_OutputResourceStates.m_SpotLightRangePerTileBufferState);

		AddResourceBarrierIfRequired(pParams->m_pSpotLightShadowMaps,
			pParams->m_InputResourceStates.m_SpotLightShadowMapsState,
			m_OutputResourceStates.m_SpotLightShadowMapsState);
	}

	m_SRVHeapStartPS = pRenderEnv->m_pShaderVisibleSRVHeap->Allocate();
	pRenderEnv->m_pDevice->CopyDescriptor(m_SRVHeapStartPS,
		pParams->m_pDepthTexture->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	pRenderEnv->m_pDevice->CopyDescriptor(pRenderEnv->m_pShaderVisibleSRVHeap->Allocate(),
		pParams->m_pGBuffer1->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	pRenderEnv->m_pDevice->CopyDescriptor(pRenderEnv->m_pShaderVisibleSRVHeap->Allocate(),
		pParams->m_pGBuffer2->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	pRenderEnv->m_pDevice->CopyDescriptor(pRenderEnv->m_pShaderVisibleSRVHeap->Allocate(),
		pParams->m_pGBuffer3->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	pRenderEnv->m_pDevice->CopyDescriptor(pRenderEnv->m_pShaderVisibleSRVHeap->Allocate(),
		pParams->m_pGBuffer4->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	if (pParams->m_EnableSpotLights)
	{
		pRenderEnv->m_pDevice->CopyDescriptor(pRenderEnv->m_pShaderVisibleSRVHeap->Allocate(),
			pParams->m_pSpotLightPropsBuffer->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		pRenderEnv->m_pDevice->CopyDescriptor(pRenderEnv->m_pShaderVisibleSRVHeap->Allocate(),
			pParams->m_pSpotLightIndexPerTileBuffer->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		pRenderEnv->m_pDevice->CopyDescriptor(pRenderEnv->m_pShaderVisibleSRVHeap->Allocate(),
			pParams->m_pSpotLightRangePerTileBuffer->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		pRenderEnv->m_pDevice->CopyDescriptor(pRenderEnv->m_pShaderVisibleSRVHeap->Allocate(),
			pParams->m_pSpotLightShadowMaps->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	}
	
	pRenderEnv->m_pDevice->CopyDescriptor(pRenderEnv->m_pShaderVisibleSRVHeap->Allocate(),
		pParams->m_pMaterialTextureIndicesBuffer->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	for (decltype(pParams->m_NumMaterialTextures) index = 0; index < pParams->m_NumMaterialTextures; ++index)
	{
		ColorTexture* pMaterialTexture = pParams->m_ppMaterialTextures[index];

		pRenderEnv->m_pDevice->CopyDescriptor(pRenderEnv->m_pShaderVisibleSRVHeap->Allocate(),
			pMaterialTexture->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	}

	SamplerDesc anisoSamplerDesc(SamplerDesc::Anisotropic);
	Sampler anisoSampler(pRenderEnv, &anisoSamplerDesc);
	
	SamplerDesc shadowMapSamplerDesc(SamplerDesc::ShadowMapSampler);
	Sampler shadowMapSampler(pRenderEnv, &shadowMapSamplerDesc);

	m_SamplerHeapStartPS = pRenderEnv->m_pShaderVisibleSamplerHeap->Allocate();
	pRenderEnv->m_pDevice->CopyDescriptor(m_SamplerHeapStartPS,
		anisoSampler.GetHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);

	pRenderEnv->m_pDevice->CopyDescriptor(pRenderEnv->m_pShaderVisibleSamplerHeap->Allocate(),
		shadowMapSampler.GetHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);

	m_RTVHeapStart = pParams->m_pAccumLightTexture->GetRTVHandle();
	m_DSVHeapStart = pParams->m_pMeshTypeDepthTexture->GetDSVHandle();
}

void TiledShadingPass::InitRootSignature(InitParams* pParams)
{
	assert(m_pRootSignature == nullptr);
	RenderEnv* pRenderEnv = pParams->m_pRenderEnv;
		
	D3D12_ROOT_PARAMETER rootParams[kNumRootParams];
	rootParams[kRoot32BitConstantsParamVS] = Root32BitConstantsParameter(0, D3D12_SHADER_VISIBILITY_VERTEX, 1);
	rootParams[kRootCBVParamPS] = RootCBVParameter(0, D3D12_SHADER_VISIBILITY_PIXEL);

	std::vector<D3D12_DESCRIPTOR_RANGE> srvRangesPS = {SRVDescriptorRange(5, 0)};
	if (pParams->m_EnableSpotLights)
		srvRangesPS.push_back(SRVDescriptorRange(4, 5));
		
	srvRangesPS.push_back(SRVDescriptorRange(1, 9));
	srvRangesPS.push_back(SRVDescriptorRange(pParams->m_NumMaterialTextures, 10));
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

	std::wstring numMaterialTexturesStr = std::to_wstring(pParams->m_NumMaterialTextures);
	std::wstring numTexturesPerMaterialStr = std::to_wstring(pParams->m_NumTexturesPerMaterial);
	std::wstring enableSpotLightsStr = std::to_wstring(pParams->m_EnableSpotLights ? 1 : 0);
	std::wstring enableDirectionalLightStr = std::to_wstring(pParams->m_EnableDirectionalLight ? 1 : 0);

	const ShaderDefine shaderDefines[] =
	{
		ShaderDefine(L"NUM_MATERIAL_TEXTURES", numMaterialTexturesStr.c_str()),
		ShaderDefine(L"NUM_TEXTURES_PER_MATERIAL", numTexturesPerMaterialStr.c_str()),
		ShaderDefine(L"ENABLE_SPOT_LIGHTS", enableSpotLightsStr.c_str()),
		ShaderDefine(L"ENABLE_DIRECTIONAL_LIGHT", enableDirectionalLightStr.c_str())
	};
	
	Shader vertexShader(L"Shaders//TiledShadingVS.hlsl", L"Main", L"vs_6_1");
	Shader pixelShader(L"Shaders//TiledShadingPS.hlsl", L"Main", L"ps_6_1", shaderDefines, ARRAYSIZE(shaderDefines));

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

void TiledShadingPass::AddResourceBarrierIfRequired(GraphicsResource* pResource, D3D12_RESOURCE_STATES currState, D3D12_RESOURCE_STATES requiredState)
{
	if (currState != requiredState)
		m_ResourceBarriers.emplace_back(pResource, currState, requiredState);
}
