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
		kRootCBVParamAll = 0,
		kRoot32BitConstantsParamVS,
		kRootSRVTableParamVS,
		kRootSRVTableParamPS,
		kRootSamplerTableParamPS,
		kNumRootParams
	};
}

TiledShadingPass::TiledShadingPass(InitParams* pParams)
	: m_Name(pParams->m_pName)
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
	u32 profileIndex = pGPUProfiler->StartProfile(pCommandList, m_Name.c_str());
#endif // ENABLE_PROFILING

	pCommandList->SetGraphicsRootSignature(m_pRootSignature);

	if (!m_ResourceBarriers.empty())
		pCommandList->ResourceBarrier((UINT)m_ResourceBarriers.size(), m_ResourceBarriers.data());

	pCommandList->SetDescriptorHeaps(pRenderEnv->m_pShaderVisibleSRVHeap, pRenderEnv->m_pShaderVisibleSamplerHeap);

	const UINT constants32BitVS[] = {meshType, numMeshTypes};
	pCommandList->SetGraphicsRootConstantBufferView(kRootCBVParamAll, pParams->m_pAppDataBuffer);
	pCommandList->SetGraphicsRoot32BitConstants(kRoot32BitConstantsParamVS, ARRAYSIZE(constants32BitVS), constants32BitVS, 0);
	pCommandList->SetGraphicsRootDescriptorTable(kRootSRVTableParamVS, m_SRVHeapStartVS);
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
	m_OutputResourceStates.m_ShadingRectangleMinPointBufferState = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
	m_OutputResourceStates.m_ShadingRectangleMaxPointBufferState = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
	m_OutputResourceStates.m_DepthTextureState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	m_OutputResourceStates.m_GBuffer1State = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	m_OutputResourceStates.m_GBuffer2State = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	m_OutputResourceStates.m_GBuffer3State = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	m_OutputResourceStates.m_GBuffer4State = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	m_OutputResourceStates.m_FirstResourceIndexPerMaterialIDBufferState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	m_OutputResourceStates.m_SpotLightWorldBoundsBufferState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
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

	AddResourceBarrierIfRequired(pParams->m_pShadingRectangleMinPointBuffer,
		pParams->m_InputResourceStates.m_ShadingRectangleMinPointBufferState,
		m_OutputResourceStates.m_ShadingRectangleMinPointBufferState);

	AddResourceBarrierIfRequired(pParams->m_pShadingRectangleMaxPointBuffer,
		pParams->m_InputResourceStates.m_ShadingRectangleMaxPointBufferState,
		m_OutputResourceStates.m_ShadingRectangleMaxPointBufferState);

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

	AddResourceBarrierIfRequired(pParams->m_pFirstResourceIndexPerMaterialIDBuffer,
		pParams->m_InputResourceStates.m_FirstResourceIndexPerMaterialIDBufferState,
		m_OutputResourceStates.m_FirstResourceIndexPerMaterialIDBufferState);

	if (pParams->m_EnableSpotLights)
	{
		AddResourceBarrierIfRequired(pParams->m_pSpotLightWorldBoundsBuffer,
			pParams->m_InputResourceStates.m_SpotLightWorldBoundsBufferState,
			m_OutputResourceStates.m_SpotLightWorldBoundsBufferState);

		AddResourceBarrierIfRequired(pParams->m_pSpotLightPropsBuffer,
			pParams->m_InputResourceStates.m_SpotLightPropsBufferState,
			m_OutputResourceStates.m_SpotLightPropsBufferState);

		AddResourceBarrierIfRequired(pParams->m_pSpotLightIndexPerTileBuffer,
			pParams->m_InputResourceStates.m_SpotLightIndexPerTileBufferState,
			m_OutputResourceStates.m_SpotLightIndexPerTileBufferState);

		AddResourceBarrierIfRequired(pParams->m_pSpotLightRangePerTileBuffer,
			pParams->m_InputResourceStates.m_SpotLightRangePerTileBufferState,
			m_OutputResourceStates.m_SpotLightRangePerTileBufferState);

		assert(false && "Think how better to do transition");
		AddResourceBarrierIfRequired(pParams->m_pSpotLightShadowMaps,
			pParams->m_InputResourceStates.m_SpotLightShadowMapsState,
			m_OutputResourceStates.m_SpotLightShadowMapsState);

		AddResourceBarrierIfRequired(pParams->m_pSpotLightViewProjMatrixBuffer,
			pParams->m_InputResourceStates.m_SpotLightViewProjMatrixBufferState,
			m_OutputResourceStates.m_SpotLightViewProjMatrixBufferState);
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
			pParams->m_pSpotLightWorldBoundsBuffer->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		pRenderEnv->m_pDevice->CopyDescriptor(pRenderEnv->m_pShaderVisibleSRVHeap->Allocate(),
			pParams->m_pSpotLightPropsBuffer->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		pRenderEnv->m_pDevice->CopyDescriptor(pRenderEnv->m_pShaderVisibleSRVHeap->Allocate(),
			pParams->m_pSpotLightIndexPerTileBuffer->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		pRenderEnv->m_pDevice->CopyDescriptor(pRenderEnv->m_pShaderVisibleSRVHeap->Allocate(),
			pParams->m_pSpotLightRangePerTileBuffer->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		pRenderEnv->m_pDevice->CopyDescriptor(pRenderEnv->m_pShaderVisibleSRVHeap->Allocate(),
			pParams->m_pSpotLightShadowMaps->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		pRenderEnv->m_pDevice->CopyDescriptor(pRenderEnv->m_pShaderVisibleSRVHeap->Allocate(),
			pParams->m_pSpotLightViewProjMatrixBuffer->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	}
	
	pRenderEnv->m_pDevice->CopyDescriptor(pRenderEnv->m_pShaderVisibleSRVHeap->Allocate(),
		pParams->m_pFirstResourceIndexPerMaterialIDBuffer->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	for (decltype(pParams->m_NumMaterialTextures) index = 0; index < pParams->m_NumMaterialTextures; ++index)
	{
		ColorTexture* pMaterialTexture = pParams->m_ppMaterialTextures[index];

		pRenderEnv->m_pDevice->CopyDescriptor(pRenderEnv->m_pShaderVisibleSRVHeap->Allocate(),
			pMaterialTexture->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	}

	SamplerDesc anisoSamplerDesc(SamplerDesc::Anisotropic);
	Sampler anisoSampler(pRenderEnv, &anisoSamplerDesc);
	
	m_SamplerHeapStartPS = pRenderEnv->m_pShaderVisibleSamplerHeap->Allocate();
	pRenderEnv->m_pDevice->CopyDescriptor(m_SamplerHeapStartPS,
		anisoSampler.GetHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);

	m_RTVHeapStart = pParams->m_pAccumLightTexture->GetRTVHandle();
	m_DSVHeapStart = pParams->m_pMeshTypeDepthTexture->GetDSVHandle();
}

void TiledShadingPass::InitRootSignature(InitParams* pParams)
{
	assert(m_pRootSignature == nullptr);
	RenderEnv* pRenderEnv = pParams->m_pRenderEnv;
		
	D3D12_ROOT_PARAMETER rootParams[kNumRootParams];
	rootParams[kRootCBVParamAll] = RootCBVParameter(0, D3D12_SHADER_VISIBILITY_ALL);
	rootParams[kRoot32BitConstantsParamVS] = Root32BitConstantsParameter(1, D3D12_SHADER_VISIBILITY_VERTEX, 2);
	
	std::vector<D3D12_DESCRIPTOR_RANGE> srvRangesVS = {SRVDescriptorRange(2, 0)};
	rootParams[kRootSRVTableParamVS] = RootDescriptorTableParameter((UINT)srvRangesVS.size(), srvRangesVS.data(), D3D12_SHADER_VISIBILITY_VERTEX);
	
	std::vector<D3D12_DESCRIPTOR_RANGE> srvRangesPS = {SRVDescriptorRange(5, 0)};
	if (pParams->m_EnableSpotLights)
		srvRangesPS.push_back(SRVDescriptorRange(6, 5));
		
	srvRangesPS.push_back(SRVDescriptorRange(1, 11));
	srvRangesPS.push_back(SRVDescriptorRange(pParams->m_NumMaterialTextures, 12));
	rootParams[kRootSRVTableParamPS] = RootDescriptorTableParameter((UINT)srvRangesPS.size(), srvRangesPS.data(), D3D12_SHADER_VISIBILITY_PIXEL);

	std::vector<D3D12_DESCRIPTOR_RANGE> samplerRangesPS = {SamplerRange(1, 0)};
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
	std::string enableSpotLightsStr = std::to_string(pParams->m_EnableSpotLights ? 1 : 0);
	std::string enableDirectionalLightStr = std::to_string(pParams->m_EnableDirectionalLight ? 1 : 0);

	const ShaderMacro shaderDefines[] =
	{
		ShaderMacro("SHADING_MODE", shadingModeStr.c_str()),
		ShaderMacro("NUM_MATERIAL_TEXTURES", numMaterialTexturesStr.c_str()),
		ShaderMacro("ENABLE_SPOT_LIGHTS", enableSpotLightsStr.c_str()),
		ShaderMacro("ENABLE_DIRECTIONAL_LIGHT", enableDirectionalLightStr.c_str()),
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

void TiledShadingPass::AddResourceBarrierIfRequired(GraphicsResource* pResource, D3D12_RESOURCE_STATES currState, D3D12_RESOURCE_STATES requiredState)
{
	if (currState != requiredState)
		m_ResourceBarriers.emplace_back(pResource, currState, requiredState);
}
