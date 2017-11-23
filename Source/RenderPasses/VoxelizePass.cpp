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
	assert(false && "Verify voxel position is compatible with texture coordinates");

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
	assert(false);
	/*
	D3D12_DESCRIPTOR_RANGE descriptorRangesVS[] = { CBVDescriptorRange(1, 0) };
	D3D12_DESCRIPTOR_RANGE descriptorRangesGS[] = { CBVDescriptorRange(1, 0) };
	D3D12_DESCRIPTOR_RANGE descriptorRangesPS[] = { CBVDescriptorRange(1, 0), SRVDescriptorRange(1, 0), UAVDescriptorRange(1, 0) };

	D3D12_ROOT_PARAMETER rootParams[kNumRootParams];
	rootParams[kCBVRootParamVS] = RootDescriptorTableParameter(ARRAYSIZE(descriptorRangesVS), &descriptorRangesVS[0], D3D12_SHADER_VISIBILITY_VERTEX);
	rootParams[kCBVRootParamGS] = RootDescriptorTableParameter(ARRAYSIZE(descriptorRangesGS), &descriptorRangesGS[0], D3D12_SHADER_VISIBILITY_GEOMETRY);
	rootParams[kSRVRootParamPS] = RootDescriptorTableParameter(ARRAYSIZE(descriptorRangesPS), &descriptorRangesPS[0], D3D12_SHADER_VISIBILITY_PIXEL);
	rootParams[kConstant32BitRootParamPS] = Root32BitConstantsParameter(1, D3D12_SHADER_VISIBILITY_PIXEL, 1);

	RootSignatureDesc rootSignatureDesc(kNumRootParams, rootParams, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
	m_pRootSignature = new RootSignature(pRenderEnv->m_pDevice, &rootSignatureDesc, L"VoxelizePass::m_pRootSignature");
	*/
}

void VoxelizePass::InitPipelineState(InitParams* pParams)
{
	assert(false);
	// DepthStencilDesc disabled

	/*
	RenderEnv* pRenderEnv = pParams->m_pRenderEnv;

	Shader vertexShader(L"Shaders//CreateVoxelGridVS.hlsl", "Main", "vs_4_0");
	Shader geometryShader(L"Shaders//CreateVoxelGridGS.hlsl", "Main", "gs_4_0");
	Shader pixelShader(L"Shaders//CreateVoxelGridPS.hlsl", "Main", "ps_5_1");

	GraphicsPipelineStateDesc pipelineStateDesc;
	pipelineStateDesc.SetRootSignature(m_pRootSignature);
	pipelineStateDesc.SetVertexShader(&vertexShader);
	pipelineStateDesc.SetGeometryShader(&geometryShader);
	pipelineStateDesc.SetPixelShader(&pixelShader);
	pipelineStateDesc.RasterizerState = RasterizerDesc(RasterizerDesc::CullNoneConservative);
	pipelineStateDesc.InputLayout = *pMeshBatch->GetInputLayout(); // Kolya. Fix me
	pipelineStateDesc.PrimitiveTopologyType = pMeshBatch->GetPrimitiveTopologyType(); // Kolya. Fix me

	m_pPipelineState = new PipelineState(pRenderEnv->m_pDevice, &pipelineStateDesc, L"VoxelizePass::m_pPipelineState");
	*/
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
