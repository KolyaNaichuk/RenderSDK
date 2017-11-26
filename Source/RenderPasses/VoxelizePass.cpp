#include "RenderPasses/VoxelizePass.h"
#include "D3DWrapper/PipelineState.h"
#include "D3DWrapper/CommandSignature.h"
#include "D3DWrapper/CommandList.h"
#include "D3DWrapper/DescriptorHeap.h"
#include "D3DWrapper/GraphicsUtils.h"
#include "D3DWrapper/GraphicsDevice.h"
#include "D3DWrapper/RootSignature.h"
#include "D3DWrapper/RenderEnv.h"
#include "Common/MeshRenderResources.h"

namespace
{
	enum RootParams
	{
		kRootCBVParamAll = 0,
		kRoot32BitConstantsParamVS,
		kRootSRVTableParamVS,
		kRoot32BitConstantsParamPS,
		kRootSRVTableParamPS,
		kRootSamplerTableParamPS,
		kNumRootParams
	};
}

VoxelizePass::VoxelizePass(InitParams* pParams)
	: m_pRootSignature(nullptr)
	, m_pPipelineState(nullptr)
	, m_pCommandSignature(nullptr)
	, m_pVoxelReflectanceTexture(nullptr)
{
	RenderEnv* pRenderEnv = pParams->m_pRenderEnv;

	D3D12_FEATURE_DATA_D3D12_OPTIONS supportedOptions;
	pRenderEnv->m_pDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &supportedOptions, sizeof(supportedOptions));
	assert(supportedOptions.ConservativeRasterizationTier != D3D12_CONSERVATIVE_RASTERIZATION_TIER_NOT_SUPPORTED);
	assert(supportedOptions.ROVsSupported == TRUE);
	
	InitResources(pParams);
	InitRootSignature(pParams);
	InitPipelineState(pParams);
	InitCommandSignature(pParams);
}

VoxelizePass::~VoxelizePass()
{
	SafeDelete(m_pCommandSignature);
	SafeDelete(m_pPipelineState);
	SafeDelete(m_pRootSignature);
	SafeDelete(m_pVoxelReflectanceTexture);
}

void VoxelizePass::Record(RenderParams* pParams)
{
	assert(false);
	/*
	RenderEnv* pRenderEnv = pParams->m_pRenderEnv;
	CommandList* pCommandList = pParams->m_pCommandList;
	ResourceList* pResources = pParams->m_pResources;
	MeshBatch* pMeshBatch = pParams->m_pMeshBatch;

	pCommandList->Begin(m_pPipelineState);
	pCommandList->SetGraphicsRootSignature(m_pRootSignature);
			
	pCommandList->OMSetRenderTargets(0, nullptr);
	
	pCommandList->SetRequiredResourceStates(&pResources->m_RequiredResourceStates);
	pCommandList->SetDescriptorHeaps(pRenderEnv->m_pShaderVisibleSRVHeap);

	pCommandList->SetGraphicsRootDescriptorTable(kCBVRootParamVS, pResources->m_SRVHeapStart);
	pCommandList->SetGraphicsRootDescriptorTable(kCBVRootParamGS, DescriptorHandle(pResources->m_SRVHeapStart, 1));
	pCommandList->SetGraphicsRootDescriptorTable(kSRVRootParamPS, DescriptorHandle(pResources->m_SRVHeapStart, 2));
	pCommandList->IASetPrimitiveTopology(pMeshBatch->GetPrimitiveTopology());
	pCommandList->IASetVertexBuffers(0, 1, pMeshBatch->GetVertexBuffer()->GetVBView());
	pCommandList->IASetIndexBuffer(pMeshBatch->GetIndexBuffer()->GetIBView());
	
	pCommandList->RSSetViewports(1, pParams->m_pViewport);
	
	Rect scissorRect(ExtractRect(pParams->m_pViewport));
	pCommandList->RSSetScissorRects(1, &scissorRect);
	
	pCommandList->ExecuteIndirect(m_pCommandSignature, pMeshBatch->GetNumMeshes(), pParams->m_pDrawMeshCommandBuffer, 0, pParams->m_pNumDrawMeshesBuffer, 0);
	pCommandList->End();
	*/
}

void VoxelizePass::InitResources(InitParams* pParams)
{
	assert(false && "Fix format for voxel reflectance texture");
	assert(false && "Verify voxel texture position is compatible with texture coordinates in VoxelizePS.hlsl");

	RenderEnv* pRenderEnv = pParams->m_pRenderEnv;

	const FLOAT optimizedClearColor[] = {0.0f, 0.0f, 0.0f, 0.0f};
	assert(m_pVoxelReflectanceTexture == nullptr);
	ColorTexture3DDesc voxelReflectanceTextureDesc(DXGI_FORMAT_R16G16B16A16_FLOAT, pParams->m_NumVoxelsX, pParams->m_NumVoxelsY, pParams->m_NumVoxelsZ, false, true, true);
	m_pVoxelReflectanceTexture = new ColorTexture(pRenderEnv, pRenderEnv->m_pDefaultHeapProps, &voxelReflectanceTextureDesc,
		pParams->m_InputResourceStates.m_VoxelReflectanceTextureState, optimizedClearColor, L"VoxelizePass::m_pVoxelReflectanceTexture");

	m_OutputResourceStates.m_InstanceIndexBufferState = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
	m_OutputResourceStates.m_InstanceWorldMatrixBufferState = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
	m_OutputResourceStates.m_VoxelReflectanceTextureState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
	m_OutputResourceStates.m_FirstResourceIndexPerMaterialIDBufferState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	m_OutputResourceStates.m_PointLightBoundsBufferState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	m_OutputResourceStates.m_PointLightPropsBufferState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	m_OutputResourceStates.m_NumPointLightsBufferState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	m_OutputResourceStates.m_PointLightIndexBufferState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	m_OutputResourceStates.m_SpotLightWorldBoundsBufferState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	m_OutputResourceStates.m_SpotLightPropsBufferState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	m_OutputResourceStates.m_NumSpotLightsBufferState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	m_OutputResourceStates.m_SpotLightIndexBufferState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;

	assert(m_ResourceBarriers.empty());
	AddResourceBarrierIfRequired(pParams->m_pInstanceIndexBuffer,
		pParams->m_InputResourceStates.m_InstanceIndexBufferState,
		m_OutputResourceStates.m_InstanceIndexBufferState);
	
	AddResourceBarrierIfRequired(pParams->m_pInstanceWorldMatrixBuffer,
		pParams->m_InputResourceStates.m_InstanceWorldMatrixBufferState,
		m_OutputResourceStates.m_InstanceWorldMatrixBufferState);

	AddResourceBarrierIfRequired(m_pVoxelReflectanceTexture,
		pParams->m_InputResourceStates.m_VoxelReflectanceTextureState,
		m_OutputResourceStates.m_VoxelReflectanceTextureState);

	AddResourceBarrierIfRequired(pParams->m_pFirstResourceIndexPerMaterialIDBuffer,
		pParams->m_InputResourceStates.m_FirstResourceIndexPerMaterialIDBufferState,
		m_OutputResourceStates.m_FirstResourceIndexPerMaterialIDBufferState);

	if (pParams->m_EnablePointLights)
	{
		AddResourceBarrierIfRequired(pParams->m_pPointLightBoundsBuffer,
			pParams->m_InputResourceStates.m_PointLightBoundsBufferState,
			m_OutputResourceStates.m_PointLightBoundsBufferState);

		AddResourceBarrierIfRequired(pParams->m_pPointLightPropsBuffer,
			pParams->m_InputResourceStates.m_PointLightPropsBufferState,
			m_OutputResourceStates.m_PointLightPropsBufferState);

		AddResourceBarrierIfRequired(pParams->m_pNumPointLightsBuffer,
			pParams->m_InputResourceStates.m_NumPointLightsBufferState,
			m_OutputResourceStates.m_NumPointLightsBufferState);

		AddResourceBarrierIfRequired(pParams->m_pPointLightIndexBuffer,
			pParams->m_InputResourceStates.m_PointLightIndexBufferState,
			m_OutputResourceStates.m_PointLightIndexBufferState);
	}

	if (pParams->m_EnableSpotLights)
	{
		AddResourceBarrierIfRequired(pParams->m_pSpotLightWorldBoundsBuffer,
			pParams->m_InputResourceStates.m_SpotLightWorldBoundsBufferState,
			m_OutputResourceStates.m_SpotLightWorldBoundsBufferState);

		AddResourceBarrierIfRequired(pParams->m_pSpotLightPropsBuffer,
			pParams->m_InputResourceStates.m_SpotLightPropsBufferState,
			m_OutputResourceStates.m_SpotLightPropsBufferState);

		AddResourceBarrierIfRequired(pParams->m_pNumSpotLightsBuffer,
			pParams->m_InputResourceStates.m_NumSpotLightsBufferState,
			m_OutputResourceStates.m_NumSpotLightsBufferState);

		AddResourceBarrierIfRequired(pParams->m_pSpotLightIndexBuffer,
			pParams->m_InputResourceStates.m_SpotLightIndexBufferState,
			m_OutputResourceStates.m_SpotLightIndexBufferState);
	}

	m_SRVHeapStartVS = pRenderEnv->m_pShaderVisibleSRVHeap->Allocate();
	pRenderEnv->m_pDevice->CopyDescriptor(m_SRVHeapStartVS,
		pParams->m_pInstanceIndexBuffer->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	pRenderEnv->m_pDevice->CopyDescriptor(pRenderEnv->m_pShaderVisibleSRVHeap->Allocate(),
		pParams->m_pInstanceWorldMatrixBuffer->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	m_SRVHeapStartPS = pRenderEnv->m_pShaderVisibleSRVHeap->Allocate();
	pRenderEnv->m_pDevice->CopyDescriptor(m_SRVHeapStartPS,
		m_pVoxelReflectanceTexture->GetUAVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	if (pParams->m_EnablePointLights)
	{
		pRenderEnv->m_pDevice->CopyDescriptor(pRenderEnv->m_pShaderVisibleSRVHeap->Allocate(),
			pParams->m_pPointLightBoundsBuffer->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		pRenderEnv->m_pDevice->CopyDescriptor(pRenderEnv->m_pShaderVisibleSRVHeap->Allocate(),
			pParams->m_pPointLightPropsBuffer->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		pRenderEnv->m_pDevice->CopyDescriptor(pRenderEnv->m_pShaderVisibleSRVHeap->Allocate(),
			pParams->m_pNumPointLightsBuffer->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		pRenderEnv->m_pDevice->CopyDescriptor(pRenderEnv->m_pShaderVisibleSRVHeap->Allocate(),
			pParams->m_pPointLightIndexBuffer->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	}

	if (pParams->m_EnableSpotLights)
	{
		pRenderEnv->m_pDevice->CopyDescriptor(pRenderEnv->m_pShaderVisibleSRVHeap->Allocate(),
			pParams->m_pSpotLightWorldBoundsBuffer->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		pRenderEnv->m_pDevice->CopyDescriptor(pRenderEnv->m_pShaderVisibleSRVHeap->Allocate(),
			pParams->m_pSpotLightPropsBuffer->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		pRenderEnv->m_pDevice->CopyDescriptor(pRenderEnv->m_pShaderVisibleSRVHeap->Allocate(),
			pParams->m_pNumSpotLightsBuffer->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		pRenderEnv->m_pDevice->CopyDescriptor(pRenderEnv->m_pShaderVisibleSRVHeap->Allocate(),
			pParams->m_pSpotLightIndexBuffer->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	}

	pRenderEnv->m_pDevice->CopyDescriptor(pRenderEnv->m_pShaderVisibleSRVHeap->Allocate(),
		pParams->m_pFirstResourceIndexPerMaterialIDBuffer->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	for (decltype(pParams->m_NumMaterialTextures) index = 0; index < pParams->m_NumMaterialTextures; ++index)
	{
		ColorTexture* pMaterialTexture = pParams->m_ppMaterialTextures[index];

		pRenderEnv->m_pDevice->CopyDescriptor(pRenderEnv->m_pShaderVisibleSRVHeap->Allocate(),
			pMaterialTexture->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	}
}

void VoxelizePass::InitRootSignature(InitParams* pParams)
{
	assert(m_pRootSignature == nullptr);
	RenderEnv* pRenderEnv = pParams->m_pRenderEnv;

	D3D12_ROOT_PARAMETER rootParams[kNumRootParams];
	rootParams[kRootCBVParamAll] = RootCBVParameter(0, D3D12_SHADER_VISIBILITY_ALL);
	rootParams[kRoot32BitConstantsParamVS] = Root32BitConstantsParameter(1, D3D12_SHADER_VISIBILITY_VERTEX, 1);

	std::vector<D3D12_DESCRIPTOR_RANGE> srvRangesVS = {SRVDescriptorRange(2, 0)};
	rootParams[kRootSRVTableParamVS] = RootDescriptorTableParameter((UINT)srvRangesVS.size(), srvRangesVS.data(), D3D12_SHADER_VISIBILITY_VERTEX);
			
	rootParams[kRoot32BitConstantsParamPS] = Root32BitConstantsParameter(1, D3D12_SHADER_VISIBILITY_PIXEL, 1);
	
	std::vector<D3D12_DESCRIPTOR_RANGE> srvRangesPS = {UAVDescriptorRange(1, 0)};
	if (pParams->m_EnablePointLights)
		srvRangesPS.push_back(SRVDescriptorRange(4, 0));
	if (pParams->m_EnableSpotLights)
		srvRangesPS.push_back(SRVDescriptorRange(4, 4));

	srvRangesPS.push_back(SRVDescriptorRange(1, 8));
	srvRangesPS.push_back(SRVDescriptorRange(pParams->m_NumMaterialTextures, 9));

	rootParams[kRootSRVTableParamPS] = RootDescriptorTableParameter((UINT)srvRangesPS.size(), srvRangesPS.data(), D3D12_SHADER_VISIBILITY_PIXEL);
	
	std::vector<D3D12_DESCRIPTOR_RANGE> samplerRangesPS = {SamplerRange(1, 0)};
	rootParams[kRootSamplerTableParamPS] = RootDescriptorTableParameter((UINT)samplerRangesPS.size(), samplerRangesPS.data(), D3D12_SHADER_VISIBILITY_PIXEL);

	RootSignatureDesc rootSignatureDesc(kNumRootParams, rootParams, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
	m_pRootSignature = new RootSignature(pRenderEnv->m_pDevice, &rootSignatureDesc, L"VoxelizePass::m_pRootSignature");
}

void VoxelizePass::InitPipelineState(InitParams* pParams)
{
	assert(m_pRootSignature != nullptr);
	assert(m_pPipelineState == nullptr);
	
	RenderEnv* pRenderEnv = pParams->m_pRenderEnv;

	const MeshRenderResources* pMeshRenderResources = pParams->m_pMeshRenderResources;
	assert(pMeshRenderResources->GetNumMeshTypes() == 1);
	const u32 meshType = 0;

	std::string enablePointLightsStr = std::to_string(pParams->m_EnablePointLights ? 1 : 0);
	std::string enableSpotLightsStr = std::to_string(pParams->m_EnableSpotLights ? 1 : 0);
	std::string enableDirectionalLightStr = std::to_string(pParams->m_EnableDirectionalLight ? 1 : 0);

	const ShaderMacro shaderDefinesPS[] =
	{
		ShaderMacro("ENABLE_POINT_LIGHTS", enablePointLightsStr.c_str()),
		ShaderMacro("ENABLE_SPOT_LIGHTS", enableSpotLightsStr.c_str()),
		ShaderMacro("ENABLE_DIRECTIONAL_LIGHT", enableDirectionalLightStr.c_str()),
		ShaderMacro()
	};

	Shader vertexShader(L"Shaders//VoxelizeVS.hlsl", "Main", "vs_4_0");
	Shader geometryShader(L"Shaders//VoxelizeGS.hlsl", "Main", "gs_4_0");
	Shader pixelShader(L"Shaders//VoxelizePS.hlsl", "Main", "ps_5_1", shaderDefinesPS);

	const InputLayoutDesc& inputLayout = pMeshRenderResources->GetInputLayout(meshType);
	assert(inputLayout.NumElements == 3);
	assert(HasVertexSemantic(inputLayout, "POSITION"));
	assert(HasVertexSemantic(inputLayout, "NORMAL"));
	assert(HasVertexSemantic(inputLayout, "TEXCOORD"));

	GraphicsPipelineStateDesc pipelineStateDesc;
	pipelineStateDesc.SetRootSignature(m_pRootSignature);
	pipelineStateDesc.SetVertexShader(&vertexShader);
	pipelineStateDesc.SetGeometryShader(&geometryShader);
	pipelineStateDesc.SetPixelShader(&pixelShader);
	pipelineStateDesc.DepthStencilState = DepthStencilDesc(DepthStencilDesc::Disabled);
	pipelineStateDesc.RasterizerState = RasterizerDesc(RasterizerDesc::CullNoneConservative);
	pipelineStateDesc.InputLayout = inputLayout;
	pipelineStateDesc.PrimitiveTopologyType = pMeshRenderResources->GetPrimitiveTopologyType(meshType);

	m_pPipelineState = new PipelineState(pRenderEnv->m_pDevice, &pipelineStateDesc, L"VoxelizePass::m_pPipelineState");
}

void VoxelizePass::InitCommandSignature(InitParams* pParams)
{
	assert(false);
}

void VoxelizePass::AddResourceBarrierIfRequired(GraphicsResource* pResource, D3D12_RESOURCE_STATES currState, D3D12_RESOURCE_STATES requiredState)
{
	if (currState != requiredState)
		m_ResourceBarriers.emplace_back(pResource, currState, requiredState);
}
