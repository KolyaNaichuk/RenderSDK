#include "DXApplication.h"
#include "D3DWrapper/GraphicsFactory.h"
#include "D3DWrapper/GraphicsDevice.h"
#include "D3DWrapper/SwapChain.h"
#include "D3DWrapper/CommandQueue.h"
#include "D3DWrapper/CommandList.h"
#include "D3DWrapper/DescriptorHeap.h"
#include "D3DWrapper/GraphicsResource.h"
#include "D3DWrapper/RenderEnv.h"
#include "D3DWrapper/Fence.h"
#include "D3DWrapper/GraphicsUtils.h"
#include "Scene/Mesh.h"
#include "Math/Vector3.h"

DXApplication::DXApplication(HINSTANCE hApp)
	: Application(hApp, L"Hello Triangle", 0, 0, 1024, 512)
	, m_pDevice(nullptr)
	, m_pSwapChain(nullptr)
	, m_pCommandQueue(nullptr)
	, m_pCommandListPool(nullptr)
	, m_pShaderInvisibleRTVHeap(nullptr)
	, m_pShaderInvisibleSRVHeap(nullptr)
	, m_pDefaultHeapProps(new HeapProperties(D3D12_HEAP_TYPE_DEFAULT))
	, m_pUploadHeapProps(new HeapProperties(D3D12_HEAP_TYPE_UPLOAD))
	, m_pRenderEnv(new RenderEnv())
	, m_pFence(nullptr)
	, m_pViewport(nullptr)
	, m_pScissorRect(nullptr)
	, m_BackBufferIndex(0)
{
	for (u8 index = 0; index < kNumBackBuffers; ++index)
		m_FrameCompletionFenceValues[index] = m_pRenderEnv->m_LastSubmissionFenceValue;
}

DXApplication::~DXApplication()
{
	SafeDelete(m_pCommandListPool);
	SafeDelete(m_pRenderEnv);
	SafeDelete(m_pDefaultHeapProps);
	SafeDelete(m_pUploadHeapProps);
	SafeDelete(m_pViewport);
	SafeDelete(m_pScissorRect);
	SafeDelete(m_pFence);
	SafeDelete(m_pShaderInvisibleSRVHeap);
	SafeDelete(m_pShaderInvisibleRTVHeap);
	SafeDelete(m_pDevice);
	SafeDelete(m_pSwapChain);
	SafeDelete(m_pCommandQueue);
}

void DXApplication::OnInit()
{
	InitRenderEnvironment();

	const Vector3f vertices[] =
	{
		Vector3f(0.0f, 10.0f, 2.0f),
		Vector3f(10.0f, -10.0f, 2.0f),
		Vector3f(-10.0f, -10.0f, 2.0f)
	};
	const WORD indices[] = {0, 1, 2};
}

void DXApplication::OnUpdate()
{
}

void DXApplication::OnRender()
{
	/*
	CommandList* pCommandList = m_pCommandListPool->Create(L"renderRectCommandList");
	
	pCommandList->Begin(m_pPipelineState);
	pCommandList->SetGraphicsRootSignature(m_pRootSignature);
		
	ColorTexture* pRenderTarget = m_pSwapChain->GetBackBuffer(m_BackBufferIndex);
	ResourceTransitionBarrier renderTargetBarrier(pRenderTarget, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
	pCommandList->ResourceBarrier(1, &renderTargetBarrier);
		
	const FLOAT clearColor[4] = {0.1f, 0.7f, 0.4f, 1.0f};
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = pRenderTarget->GetRTVHandle();
	pCommandList->ClearRenderTargetView(rtvHandle, clearColor);

	pCommandList->OMSetRenderTargets(1, &rtvHandle);
	pCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	pCommandList->IASetVertexBuffers(0, 1, m_pVertexBuffer->GetVBView());
	pCommandList->IASetIndexBuffer(m_pIndexBuffer->GetIBView());
	pCommandList->RSSetViewports(1, m_pViewport);
	pCommandList->RSSetScissorRects(1, m_pScissorRect);
	pCommandList->DrawIndexedInstanced(6, 1, 0, 0, 0);

	ResourceTransitionBarrier presentStateBarrier(pRenderTarget, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
	pCommandList->ResourceBarrier(1, &presentStateBarrier);
	pCommandList->End();

	++m_pRenderEnv->m_LastSubmissionFenceValue;
	m_pCommandQueue->ExecuteCommandLists(1, &pCommandList, m_pFence, m_pRenderEnv->m_LastSubmissionFenceValue);

	++m_pRenderEnv->m_LastSubmissionFenceValue;
	m_pSwapChain->Present(1, 0);
	m_pCommandQueue->Signal(m_pFence, m_pRenderEnv->m_LastSubmissionFenceValue);

	m_FrameCompletionFenceValues[m_BackBufferIndex] = m_pRenderEnv->m_LastSubmissionFenceValue;
	m_BackBufferIndex = m_pSwapChain->GetCurrentBackBufferIndex();
	m_pFence->WaitForSignalOnCPU(m_FrameCompletionFenceValues[m_BackBufferIndex]);
	*/
}

void DXApplication::OnDestroy()
{
	m_pCommandQueue->Signal(m_pFence, m_pRenderEnv->m_LastSubmissionFenceValue);
	m_pFence->WaitForSignalOnCPU(m_pRenderEnv->m_LastSubmissionFenceValue);
}

void DXApplication::InitRenderEnvironment()
{
	GraphicsFactory factory;
	m_pDevice = new GraphicsDevice(&factory, D3D_FEATURE_LEVEL_12_1);

	D3D12_FEATURE_DATA_D3D12_OPTIONS5 supportedOptions;
	m_pDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &supportedOptions, sizeof(supportedOptions));
	assert(supportedOptions.RaytracingTier == D3D12_RAYTRACING_TIER_1_0);

	DescriptorHeapDesc rtvHeapDesc(D3D12_DESCRIPTOR_HEAP_TYPE_RTV, kNumBackBuffers, false);
	m_pShaderInvisibleRTVHeap = new DescriptorHeap(m_pDevice, &rtvHeapDesc, L"m_pShaderInvisibleRTVHeap");

	DescriptorHeapDesc srvHeapDesc(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 32, false);
	m_pShaderInvisibleSRVHeap = new DescriptorHeap(m_pDevice, &srvHeapDesc, L"m_pShaderInvisibleSRVHeap");

	CommandQueueDesc commandQueueDesc(D3D12_COMMAND_LIST_TYPE_DIRECT);
	m_pCommandQueue = new CommandQueue(m_pDevice, &commandQueueDesc, L"m_pCommandQueue");

	m_pCommandListPool = new CommandListPool(m_pDevice, D3D12_COMMAND_LIST_TYPE_DIRECT);
	m_pFence = new Fence(m_pDevice, m_pRenderEnv->m_LastSubmissionFenceValue, L"m_pFence");

	m_pRenderEnv->m_pDevice = m_pDevice;
	m_pRenderEnv->m_pCommandListPool = m_pCommandListPool;
	m_pRenderEnv->m_pCommandQueue = m_pCommandQueue;
	m_pRenderEnv->m_pFence = m_pFence;
	m_pRenderEnv->m_pDefaultHeapProps = m_pDefaultHeapProps;
	m_pRenderEnv->m_pUploadHeapProps = m_pUploadHeapProps;
	m_pRenderEnv->m_pShaderInvisibleRTVHeap = m_pShaderInvisibleRTVHeap;
	m_pRenderEnv->m_pShaderInvisibleSRVHeap = m_pShaderInvisibleSRVHeap;

	const RECT bufferRect = m_pWindow->GetClientRect();
	const UINT bufferWidth = bufferRect.right - bufferRect.left;
	const UINT bufferHeight = bufferRect.bottom - bufferRect.top;

	m_pViewport = new Viewport(0.0f, 0.0f, FLOAT(bufferWidth), FLOAT(bufferHeight));
	m_pScissorRect = new Rect(0, 0, bufferWidth, bufferHeight);

	SwapChainDesc swapChainDesc(kNumBackBuffers, m_pWindow->GetHWND(), bufferWidth, bufferHeight);
	m_pSwapChain = new SwapChain(&factory, m_pRenderEnv, &swapChainDesc, m_pCommandQueue);
	m_BackBufferIndex = m_pSwapChain->GetCurrentBackBufferIndex();
}
