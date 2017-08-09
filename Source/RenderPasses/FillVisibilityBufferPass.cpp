#include "RenderPasses/FillVisibilityBufferPass.h"
#include "D3DWrapper/CommandList.h"
#include "D3DWrapper/GraphicsDevice.h"
#include "D3DWrapper/PipelineState.h"
#include "D3DWrapper/RenderEnv.h"
#include "D3DWrapper/RootSignature.h"

namespace
{
	enum RootParams
	{
		kRootSRVTableParamVS = 0,
		kRootUAVParamPS,
		kNumRootParams
	};
}

FillVisibilityBufferPass::FillVisibilityBufferPass(InitParams* pParams)
	: m_pRootSignature(nullptr)
	, m_pPipelineState(nullptr)
	, m_pUnitAABBIndexBuffer(nullptr)
	, m_pVisibilityBuffer(nullptr)
	, m_pArgumentBuffer(nullptr)
{
	InitResources(pParams);
	InitRootSignature(pParams);
	InitPipelineState(pParams);
}

FillVisibilityBufferPass::~FillVisibilityBufferPass()
{
	SafeDelete(m_pArgumentBuffer);
	SafeDelete(m_pVisibilityBuffer);
	SafeDelete(m_pUnitAABBIndexBuffer);
	SafeDelete(m_pPipelineState);
	SafeDelete(m_pRootSignature);
}

void FillVisibilityBufferPass::Record(RenderParams* pParams)
{
	assert(false);
	// Clear visibility buffer to 0
}

void FillVisibilityBufferPass::InitResources(InitParams* pParams)
{
	RenderEnv* pRenderEnv = pParams->m_pRenderEnv;

	assert(m_pUnitAABBIndexBuffer == nullptr);
	const u16 unitAABBTriangleStripIndices[] = {1, 0, 3, 2, 6, 0, 4, 1, 5, 3, 7, 6, 5, 4};
	IndexBufferDesc unitAABBIndexBufferDesc(ARRAYSIZE(unitAABBTriangleStripIndices), sizeof(unitAABBTriangleStripIndices[0]));
	m_pUnitAABBIndexBuffer = new Buffer(pRenderEnv, pRenderEnv->m_pDefaultHeapProps, &unitAABBIndexBufferDesc, D3D12_RESOURCE_STATE_COPY_DEST, L"m_pUnitAABBIndexBuffer");
	UploadData(pRenderEnv, m_pUnitAABBIndexBuffer, D3D12_RESOURCE_STATE_INDEX_BUFFER, &unitAABBIndexBufferDesc, unitAABBTriangleStripIndices, sizeof(unitAABBTriangleStripIndices));

	assert(m_pVisibilityBuffer == nullptr);
	FormattedBufferDesc visibilityBufferDesc(pParams->m_MaxNumInstances, DXGI_FORMAT_R32_UINT, true, true);
	m_pVisibilityBuffer = new Buffer(pRenderEnv, pRenderEnv->m_pDefaultHeapProps, &visibilityBufferDesc,
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS, L"FillVisibilityBufferPass::m_pVisibilityBuffer");

	assert(m_pArgumentBuffer == nullptr);
	assert(false && "m_pArgumentBuffer");

	assert(false && "Verify m_OutputResourceStates.m_DepthTextureState");
	m_OutputResourceStates.m_InstanceIndexBufferState = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
	m_OutputResourceStates.m_InstanceWorldViewProjMatrixBufferState = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
	m_OutputResourceStates.m_NumInstancesBufferState = D3D12_RESOURCE_STATE_COPY_SOURCE;
	m_OutputResourceStates.m_DepthTextureState = D3D12_RESOURCE_STATE_DEPTH_READ;
	m_OutputResourceStates.m_VisibilityBufferState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;

	assert(m_ResourceBarriers.empty());
	CreateResourceBarrierIfRequired(pParams->m_pInstanceIndexBuffer,
		pParams->m_InputResourceStates.m_InstanceIndexBufferState,
		m_OutputResourceStates.m_InstanceIndexBufferState);

	CreateResourceBarrierIfRequired(pParams->m_pInstanceWorldViewProjMatrixBuffer,
		pParams->m_InputResourceStates.m_InstanceWorldViewProjMatrixBufferState,
		m_OutputResourceStates.m_InstanceWorldViewProjMatrixBufferState);

	CreateResourceBarrierIfRequired(pParams->m_pNumInstancesBuffer,
		pParams->m_InputResourceStates.m_NumInstancesBufferState,
		m_OutputResourceStates.m_NumInstancesBufferState);

	CreateResourceBarrierIfRequired(pParams->m_pDepthTexture,
		pParams->m_InputResourceStates.m_DepthTextureState,
		m_OutputResourceStates.m_DepthTextureState);

	CreateResourceBarrierIfRequired(m_pVisibilityBuffer,
		pParams->m_InputResourceStates.m_VisibilityBufferState,
		m_OutputResourceStates.m_VisibilityBufferState);

	m_SRVHeapStartVS = pRenderEnv->m_pShaderInvisibleSRVHeap->Allocate();
	pRenderEnv->m_pDevice->CopyDescriptor(m_SRVHeapStartVS,
		pParams->m_pInstanceIndexBuffer->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	pRenderEnv->m_pDevice->CopyDescriptor(pRenderEnv->m_pShaderInvisibleSRVHeap->Allocate(),
		pParams->m_pInstanceWorldViewProjMatrixBuffer->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	m_SRVHeapStartPS = pRenderEnv->m_pShaderInvisibleSRVHeap->Allocate();
	pRenderEnv->m_pDevice->CopyDescriptor(m_SRVHeapStartPS,
		m_pVisibilityBuffer->GetUAVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	m_DSVHeapStart = pParams->m_pDepthTexture->GetDSVHandle();
}

void FillVisibilityBufferPass::InitRootSignature(InitParams* pParams)
{
	assert(m_pRootSignature == nullptr);
	D3D12_ROOT_PARAMETER rootParams[kNumRootParams];

	D3D12_DESCRIPTOR_RANGE descriptorRangesVS[] = {SRVDescriptorRange(2, 0)};
	rootParams[kRootSRVTableParamVS] = RootDescriptorTableParameter(ARRAYSIZE(descriptorRangesVS), descriptorRangesVS, D3D12_SHADER_VISIBILITY_VERTEX);
	
	rootParams[kRootUAVParamPS] = RootUAVParameter(0, D3D12_SHADER_VISIBILITY_PIXEL);

	RootSignatureDesc rootSignatureDesc(kNumRootParams, rootParams);
	m_pRootSignature = new RootSignature(pParams->m_pRenderEnv->m_pDevice, &rootSignatureDesc, L"FillVisibilityBufferPass::m_pRootSignature");
}

void FillVisibilityBufferPass::InitPipelineState(InitParams* pParams)
{
	assert(m_pRootSignature != nullptr);
	assert(m_pPipelineState == nullptr);

	RenderEnv* pRenderEnv = pParams->m_pRenderEnv;

	std::string clampVerticesBehindCameraNearPlaneStr = std::to_string(pParams->m_ClampVerticesBehindCameraNearPlane ? 1 : 0);
	const ShaderMacro shaderDefinesVS[] =
	{
		ShaderMacro("CLAMP_VERTICES_BEHIND_CAMERA_NEAR_PLANE", clampVerticesBehindCameraNearPlaneStr.c_str()),
		ShaderMacro()
	};
	Shader vertexShader(L"Shaders//FillVisibilityBufferVS.hlsl", "Main", "vs_4_0", shaderDefinesVS);
	Shader pixelShader(L"Shaders//FillVisibilityBufferPS.hlsl", "Main", "ps_4_0");

	assert(false && "pipelineStateDesc.InputLayout");
	assert(false && "pipelineStateDesc.PrimitiveTopologyType");

	GraphicsPipelineStateDesc pipelineStateDesc;
	pipelineStateDesc.SetRootSignature(m_pRootSignature);
	pipelineStateDesc.SetVertexShader(&vertexShader);
	pipelineStateDesc.SetPixelShader(&pixelShader);
	//pipelineStateDesc.InputLayout;
	//pipelineStateDesc.PrimitiveTopologyType;
	pipelineStateDesc.DepthStencilState = DepthStencilDesc(DepthStencilDesc::Enabled);
	pipelineStateDesc.SetRenderTargetFormats(0, nullptr, GetDepthStencilViewFormat(pParams->m_pDepthTexture->GetFormat()));

	m_pPipelineState = new PipelineState(pRenderEnv->m_pDevice, &pipelineStateDesc, L"FillVisibilityBufferPass::m_pPipelineState");
}

void FillVisibilityBufferPass::CreateResourceBarrierIfRequired(GraphicsResource* pResource, D3D12_RESOURCE_STATES currState, D3D12_RESOURCE_STATES requiredState)
{
	if (currState != requiredState)
		m_ResourceBarriers.emplace_back(pResource, currState, requiredState);
}
