#include "RenderPasses/FillVisibilityBufferPass.h"
#include "D3DWrapper/CommandList.h"
#include "D3DWrapper/CommandSignature.h"
#include "D3DWrapper/GraphicsDevice.h"
#include "D3DWrapper/GraphicsUtils.h"
#include "D3DWrapper/PipelineState.h"
#include "D3DWrapper/RenderEnv.h"
#include "D3DWrapper/RootSignature.h"

namespace
{
	enum RootParams
	{
		kRootCBVParamVS = 0,
		kRootSRVTableParamVS,
		kRootSRVTableParamPS,
		kNumRootParams
	};
}

FillVisibilityBufferPass::FillVisibilityBufferPass(InitParams* pParams)
	: m_pRootSignature(nullptr)
	, m_pPipelineState(nullptr)
	, m_pCommandSignature(nullptr)
	, m_pViewport(nullptr)
	, m_pUnitAABBIndexBuffer(nullptr)
	, m_pVisibilityBuffer(nullptr)
	, m_pArgumentBuffer(nullptr)
{
	InitResources(pParams);
	InitRootSignature(pParams);
	InitPipelineState(pParams);
	InitCommandSignature(pParams);
}

FillVisibilityBufferPass::~FillVisibilityBufferPass()
{
	SafeDelete(m_pArgumentBuffer);
	SafeDelete(m_pVisibilityBuffer);
	SafeDelete(m_pUnitAABBIndexBuffer);
	SafeDelete(m_pViewport);
	SafeDelete(m_pCommandSignature);
	SafeDelete(m_pPipelineState);
	SafeDelete(m_pRootSignature);
}

void FillVisibilityBufferPass::Record(RenderParams* pParams)
{
	RenderEnv* pRenderEnv = pParams->m_pRenderEnv;
	CommandList* pCommandList = pParams->m_pCommandList;

	pCommandList->Begin(m_pPipelineState);
	pCommandList->SetGraphicsRootSignature(m_pRootSignature);
	pCommandList->SetDescriptorHeaps(pRenderEnv->m_pShaderVisibleSRVHeap);

	pCommandList->ResourceBarrier((UINT)m_ResourceBarriers.size(), m_ResourceBarriers.data());
	pCommandList->CopyBufferRegion(m_pArgumentBuffer, sizeof(u32), pParams->m_pNumInstancesBuffer, 0, sizeof(u32));

	ResourceBarrier argumentBarrier(m_pArgumentBuffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
	pCommandList->ResourceBarrier(1, &argumentBarrier);
	
	pCommandList->SetGraphicsRootConstantBufferView(kRootCBVParamVS, pParams->m_pAppDataBuffer);
	pCommandList->SetGraphicsRootDescriptorTable(kRootSRVTableParamVS, m_SRVHeapStartVS);
	pCommandList->SetGraphicsRootDescriptorTable(kRootSRVTableParamPS, m_SRVHeapStartPS);

	const u32 visibilityValue[] = {0, 0, 0, 0};
	pCommandList->ClearUnorderedAccessView(m_SRVHeapStartPS, m_pVisibilityBuffer->GetUAVHandle(), m_pVisibilityBuffer, visibilityValue);

	D3D12_CPU_DESCRIPTOR_HANDLE dsvHeapStart = m_DSVHeapStart;
	pCommandList->OMSetRenderTargets(0, nullptr, FALSE, &dsvHeapStart);
	pCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	pCommandList->IASetVertexBuffers(0, 1, nullptr);
	pCommandList->IASetIndexBuffer(m_pUnitAABBIndexBuffer->GetIBView());

	Rect scissorRect(ExtractRect(m_pViewport));
	pCommandList->RSSetViewports(1, m_pViewport);
	pCommandList->RSSetScissorRects(1, &scissorRect);

	pCommandList->ExecuteIndirect(m_pCommandSignature, 1, m_pArgumentBuffer, 0, nullptr, 0);
	pCommandList->End();
}

void FillVisibilityBufferPass::InitResources(InitParams* pParams)
{
	RenderEnv* pRenderEnv = pParams->m_pRenderEnv;

	assert(m_pUnitAABBIndexBuffer == nullptr);
	const u16 unitAABBTriangleStripIndices[] = {1, 0, 3, 2, 6, 0, 4, 1, 5, 3, 7, 6, 5, 4};
	const u32 unitAABBIndexCount = ARRAYSIZE(unitAABBTriangleStripIndices);

	IndexBufferDesc unitAABBIndexBufferDesc(unitAABBIndexCount, sizeof(unitAABBTriangleStripIndices[0]));
	m_pUnitAABBIndexBuffer = new Buffer(pRenderEnv, pRenderEnv->m_pDefaultHeapProps, &unitAABBIndexBufferDesc, D3D12_RESOURCE_STATE_COPY_DEST, L"m_pUnitAABBIndexBuffer");
	UploadData(pRenderEnv, m_pUnitAABBIndexBuffer, unitAABBIndexBufferDesc,
		D3D12_RESOURCE_STATE_INDEX_BUFFER, unitAABBTriangleStripIndices, sizeof(unitAABBTriangleStripIndices));

	assert(m_pArgumentBuffer == nullptr);
	DrawIndexedArguments argumentBufferData(unitAABBIndexCount, 0, 0, 0, 0);
	StructuredBufferDesc argumentBufferDesc(1, sizeof(argumentBufferData), false, false);
	m_pArgumentBuffer = new Buffer(pRenderEnv, pRenderEnv->m_pDefaultHeapProps, &argumentBufferDesc,
		D3D12_RESOURCE_STATE_COPY_DEST, L"FillVisibilityBufferPass::m_pArgumentBuffer");
	UploadData(pRenderEnv, m_pArgumentBuffer, argumentBufferDesc,
		D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT, &argumentBufferData, sizeof(argumentBufferData));

	assert(m_pVisibilityBuffer == nullptr);
	FormattedBufferDesc visibilityBufferDesc(pParams->m_MaxNumInstances, DXGI_FORMAT_R32_UINT, true, true);
	m_pVisibilityBuffer = new Buffer(pRenderEnv, pRenderEnv->m_pDefaultHeapProps, &visibilityBufferDesc,
		pParams->m_InputResourceStates.m_VisibilityBufferState, L"FillVisibilityBufferPass::m_pVisibilityBuffer");

	m_OutputResourceStates.m_InstanceIndexBufferState = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
	m_OutputResourceStates.m_InstanceWorldOBBMatrixBufferState = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
	m_OutputResourceStates.m_NumInstancesBufferState = D3D12_RESOURCE_STATE_COPY_SOURCE;
	m_OutputResourceStates.m_DepthTextureState = D3D12_RESOURCE_STATE_DEPTH_WRITE;
	m_OutputResourceStates.m_VisibilityBufferState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;

	assert(m_ResourceBarriers.empty());
	CreateResourceBarrierIfRequired(pParams->m_pInstanceIndexBuffer,
		pParams->m_InputResourceStates.m_InstanceIndexBufferState,
		m_OutputResourceStates.m_InstanceIndexBufferState);

	CreateResourceBarrierIfRequired(pParams->m_pInstanceWorldOBBMatrixBuffer,
		pParams->m_InputResourceStates.m_InstanceWorldOBBMatrixBufferState,
		m_OutputResourceStates.m_InstanceWorldOBBMatrixBufferState);

	CreateResourceBarrierIfRequired(pParams->m_pNumInstancesBuffer,
		pParams->m_InputResourceStates.m_NumInstancesBufferState,
		m_OutputResourceStates.m_NumInstancesBufferState);

	CreateResourceBarrierIfRequired(pParams->m_pDepthTexture,
		pParams->m_InputResourceStates.m_DepthTextureState,
		m_OutputResourceStates.m_DepthTextureState);

	CreateResourceBarrierIfRequired(m_pVisibilityBuffer,
		pParams->m_InputResourceStates.m_VisibilityBufferState,
		m_OutputResourceStates.m_VisibilityBufferState);

	CreateResourceBarrierIfRequired(m_pArgumentBuffer,
		D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT,
		D3D12_RESOURCE_STATE_COPY_DEST);

	m_SRVHeapStartVS = pRenderEnv->m_pShaderVisibleSRVHeap->Allocate();
	pRenderEnv->m_pDevice->CopyDescriptor(m_SRVHeapStartVS,
		pParams->m_pInstanceIndexBuffer->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	pRenderEnv->m_pDevice->CopyDescriptor(pRenderEnv->m_pShaderVisibleSRVHeap->Allocate(),
		pParams->m_pInstanceWorldOBBMatrixBuffer->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	m_SRVHeapStartPS = pRenderEnv->m_pShaderVisibleSRVHeap->Allocate();
	pRenderEnv->m_pDevice->CopyDescriptor(m_SRVHeapStartPS,
		m_pVisibilityBuffer->GetUAVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	m_DSVHeapStart = pParams->m_pDepthTexture->GetDSVHandle();
}

void FillVisibilityBufferPass::InitRootSignature(InitParams* pParams)
{
	assert(m_pRootSignature == nullptr);

	D3D12_ROOT_PARAMETER rootParams[kNumRootParams];
	rootParams[kRootCBVParamVS] = RootCBVParameter(0, D3D12_SHADER_VISIBILITY_VERTEX);

	D3D12_DESCRIPTOR_RANGE descriptorRangesVS[] = {SRVDescriptorRange(2, 0)};
	rootParams[kRootSRVTableParamVS] = RootDescriptorTableParameter(ARRAYSIZE(descriptorRangesVS), descriptorRangesVS, D3D12_SHADER_VISIBILITY_VERTEX);
	
	D3D12_DESCRIPTOR_RANGE descriptorRangesPS[] = {UAVDescriptorRange(1, 0)};
	rootParams[kRootSRVTableParamPS] = RootDescriptorTableParameter(ARRAYSIZE(descriptorRangesPS), descriptorRangesPS, D3D12_SHADER_VISIBILITY_PIXEL);
	
	RootSignatureDesc rootSignatureDesc(kNumRootParams, rootParams);
	m_pRootSignature = new RootSignature(pParams->m_pRenderEnv->m_pDevice, &rootSignatureDesc, L"FillVisibilityBufferPass::m_pRootSignature");
}

void FillVisibilityBufferPass::InitPipelineState(InitParams* pParams)
{
	assert(m_pRootSignature != nullptr);
	assert(m_pPipelineState == nullptr);

	m_pViewport = new Viewport(0.0f, 0.0f, FLOAT(pParams->m_pDepthTexture->GetWidth()), FLOAT(pParams->m_pDepthTexture->GetHeight()));
	RenderEnv* pRenderEnv = pParams->m_pRenderEnv;

	std::string clampVerticesBehindCameraNearPlaneStr = std::to_string(pParams->m_ClampVerticesBehindCameraNearPlane ? 1 : 0);
	const ShaderMacro shaderDefinesVS[] =
	{
		ShaderMacro("CLAMP_VERTICES_BEHIND_CAMERA_NEAR_PLANE", clampVerticesBehindCameraNearPlaneStr.c_str()),
		ShaderMacro()
	};
	Shader vertexShader(L"Shaders//FillVisibilityBufferVS.hlsl", "Main", "vs_4_0", shaderDefinesVS);
	Shader pixelShader(L"Shaders//FillVisibilityBufferPS.hlsl", "Main", "ps_5_0");
	
	GraphicsPipelineStateDesc pipelineStateDesc;
	pipelineStateDesc.SetRootSignature(m_pRootSignature);
	pipelineStateDesc.SetVertexShader(&vertexShader);
	pipelineStateDesc.SetPixelShader(&pixelShader);
	pipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	pipelineStateDesc.DepthStencilState = DepthStencilDesc(DepthStencilDesc::EnabledNoWrites);
	pipelineStateDesc.SetRenderTargetFormats(0, nullptr, GetDepthStencilViewFormat(pParams->m_pDepthTexture->GetFormat()));

	m_pPipelineState = new PipelineState(pRenderEnv->m_pDevice, &pipelineStateDesc, L"FillVisibilityBufferPass::m_pPipelineState");
}

void FillVisibilityBufferPass::InitCommandSignature(InitParams* pParams)
{
	assert(m_pCommandSignature == nullptr);
	RenderEnv* pRenderEnv = pParams->m_pRenderEnv;

	D3D12_INDIRECT_ARGUMENT_DESC argumentDescs[] =
	{
		DrawIndexedArgument()
	};
	CommandSignatureDesc commandSignatureDesc(sizeof(DrawIndexedArguments), ARRAYSIZE(argumentDescs), argumentDescs);
	m_pCommandSignature = new CommandSignature(pRenderEnv->m_pDevice, nullptr, &commandSignatureDesc, L"FillVisibilityBufferPass::m_pCommandSignature");
}

void FillVisibilityBufferPass::CreateResourceBarrierIfRequired(GraphicsResource* pResource, D3D12_RESOURCE_STATES currState, D3D12_RESOURCE_STATES requiredState)
{
	if (currState != requiredState)
		m_ResourceBarriers.emplace_back(pResource, currState, requiredState);
}
