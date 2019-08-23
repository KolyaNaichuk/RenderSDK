#include "DXApplication.h"
#include "RayTracingPass.h"
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
#include "Profiler/GPUProfiler.h"
#include "RenderPasses/VisualizeTexturePass.h"
#include "Scene/Mesh.h"
#include "Math/Vector3.h"

DXApplication::DXApplication(HINSTANCE hApp)
	: Application(hApp, L"Hello Triangle", 0, 0, 1024, 512)
	, m_pDefaultHeapProps(new HeapProperties(D3D12_HEAP_TYPE_DEFAULT))
	, m_pUploadHeapProps(new HeapProperties(D3D12_HEAP_TYPE_UPLOAD))
	, m_pRenderEnv(new RenderEnv())
{
	for (u8 index = 0; index < kNumBackBuffers; ++index)
		m_FrameCompletionFenceValues[index] = m_pRenderEnv->m_LastSubmissionFenceValue;
}

DXApplication::~DXApplication()
{
	SafeDelete(m_pRayTracingPass);
	SafeDelete(m_pVisualizeRayTracedResultPass);
	SafeDelete(m_pAppDataBuffer);
	SafeDelete(m_pGPUProfiler);
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
	InitRayTracingPass();
	InitVisualizeRayTracedResultPass();
}

void DXApplication::OnUpdate()
{
}

void DXApplication::OnRender()
{
#ifdef ENABLE_PROFILING
	m_pGPUProfiler->StartFrame();
#endif // ENABLE_PROFILING
	
	static const u8 commandListBatchSize = 2;
	static CommandList* commandListBatch[commandListBatchSize];
	commandListBatch[0] = RecordRayTracingPass();
	commandListBatch[1] = RecordVisualizeRayTracedResultPass();
	
#ifdef ENABLE_PROFILING
	m_pGPUProfiler->EndFrame(m_pCommandQueue);
	m_pGPUProfiler->OutputToConsole();
#endif // #ifdef ENABLE_PROFILING

	++m_pRenderEnv->m_LastSubmissionFenceValue;
	m_pCommandQueue->ExecuteCommandLists(commandListBatchSize, commandListBatch, m_pFence, m_pRenderEnv->m_LastSubmissionFenceValue);
	ColorTexture* pRenderTarget = m_pSwapChain->GetBackBuffer(m_BackBufferIndex);

	++m_pRenderEnv->m_LastSubmissionFenceValue;

#ifdef ENABLE_PROFILING
	m_pSwapChain->Present(0/*vsync disabled*/, 0);
#else // ENABLE_PROFILING
	m_pSwapChain->Present(1/*vsync enabled*/, 0);
#endif // ENABLE_PROFILING

	m_pCommandQueue->Signal(m_pFence, m_pRenderEnv->m_LastSubmissionFenceValue);
	
	m_FrameCompletionFenceValues[m_BackBufferIndex] = m_pRenderEnv->m_LastSubmissionFenceValue;
	m_BackBufferIndex = m_pSwapChain->GetCurrentBackBufferIndex();
	m_pFence->WaitForSignalOnCPU(m_FrameCompletionFenceValues[m_BackBufferIndex]);
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

#ifdef ENABLE_PROFILING
	m_pGPUProfiler = new GPUProfiler(m_pRenderEnv, 2/*maxNumProfiles*/, kNumBackBuffers);
#endif // ENABLE_PROFILING
	m_pRenderEnv->m_pGPUProfiler = m_pGPUProfiler;

	const RECT bufferRect = m_pWindow->GetClientRect();
	const UINT bufferWidth = bufferRect.right - bufferRect.left;
	const UINT bufferHeight = bufferRect.bottom - bufferRect.top;

	m_pViewport = new Viewport(0.0f, 0.0f, FLOAT(bufferWidth), FLOAT(bufferHeight));
	m_pScissorRect = new Rect(0, 0, bufferWidth, bufferHeight);

	SwapChainDesc swapChainDesc(kNumBackBuffers, m_pWindow->GetHWND(), bufferWidth, bufferHeight);
	m_pSwapChain = new SwapChain(&factory, m_pRenderEnv, &swapChainDesc, m_pCommandQueue);
	m_BackBufferIndex = m_pSwapChain->GetCurrentBackBufferIndex();
}

void DXApplication::InitRayTracingPass()
{
	assert(m_pRayTracingPass == nullptr);

	const Vector3f vertices[] =
	{
		Vector3f(0.0f, 10.0f, 2.0f),
		Vector3f(10.0f, -10.0f, 2.0f),
		Vector3f(-10.0f, -10.0f, 2.0f)
	};
	const WORD indices[] = {0, 1, 2};

	assert(false);
}

CommandList* DXApplication::RecordRayTracingPass()
{
	assert(m_pRayTracingPass != nullptr);

	RayTracingPass::RenderParams params;
	params.m_pRenderEnv = m_pRenderEnv;
	params.m_pCommandList = m_pCommandListPool->Create(L"pRayTracingCommandList");
	
	m_pRayTracingPass->Record(&params);
	return params.m_pCommandList;
}

void DXApplication::InitVisualizeRayTracedResultPass()
{
	assert(m_pVisualizeRayTracedResultPass == nullptr);
	assert(false);
}

CommandList* DXApplication::RecordVisualizeRayTracedResultPass()
{
	assert(m_pVisualizeRayTracedResultPass != nullptr);
	
	VisualizeTexturePass::RenderParams params;
	params.m_pRenderEnv = m_pRenderEnv;
	params.m_pCommandList = m_pCommandListPool->Create(L"pVisualizeRayTracedResultCommandList");;
	params.m_pViewport = m_pViewport;

	m_pVisualizeRayTracedResultPass->Record(&params);
	return params.m_pCommandList;
}
