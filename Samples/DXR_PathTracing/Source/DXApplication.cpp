#include "DXApplication.h"
#include "PathTracingPass.h"
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
#include "RenderPasses/MeshRenderResources.h"
#include "Scene/Camera.h"
#include "Scene/Scene.h"
#include "Scene/SceneLoader.h"
#include "Scene/MeshBatch.h"
#include "Scene/Mesh.h"
#include "Math/Vector3.h"
#include "Math/Transform.h"
#include "Common/Color.h"
#include "Common/KeyboardInput.h"

struct AppData
{
	Vector3f m_CameraWorldPos;
	f32 m_RayMinExtent;
	Vector3f m_CameraWorldAxisX;
	f32 m_RayMaxExtent;
	Vector3f m_CameraWorldAxisY;
	f32 m_NotUsed1;
	Vector3f m_CameraWorldAxisZ;
	f32 m_NotUsed2;
	f32 m_NotUsed3[16];
	f32 m_NotUsed4[16];
	f32 m_NotUsed5[16];
};

enum
{
	kBackBufferWidth = 1024,
	kBackBufferHeight = 512
};

DXApplication::DXApplication(HINSTANCE hApp)
	: Application(hApp, L"DXR Experiments", 0, 0, kBackBufferWidth, kBackBufferHeight)
	, m_pDefaultHeapProps(new HeapProperties(D3D12_HEAP_TYPE_DEFAULT))
	, m_pUploadHeapProps(new HeapProperties(D3D12_HEAP_TYPE_UPLOAD))
	, m_pReadbackHeapProps(new HeapProperties(D3D12_HEAP_TYPE_READBACK))
	, m_pRenderEnv(new RenderEnv())
{
	for (u8 index = 0; index < kNumBackBuffers; ++index)
		m_FrameCompletionFenceValues[index] = m_pRenderEnv->m_LastSubmissionFenceValue;
}

DXApplication::~DXApplication()
{
	SafeDelete(m_pPathTracingPass);

	for (u8 index = 0; index < kNumBackBuffers; ++index)
	{
		SafeDelete(m_VisualizePathTracedResultPasses[index]);
		SafeDelete(m_AppDataBuffers[index]);
	}
	
	SafeDelete(m_pCamera);
	SafeDelete(m_pGPUProfiler);
	SafeDelete(m_pCommandListPool);
	SafeDelete(m_pRenderEnv);
	SafeDelete(m_pDefaultHeapProps);
	SafeDelete(m_pUploadHeapProps);
	SafeDelete(m_pReadbackHeapProps);
	SafeDelete(m_pViewport);
	SafeDelete(m_pScissorRect);
	SafeDelete(m_pFence);
	SafeDelete(m_pShaderVisibleSRVHeap);
	SafeDelete(m_pShaderInvisibleSRVHeap);
	SafeDelete(m_pShaderInvisibleRTVHeap);
	SafeDelete(m_pDevice);
	SafeDelete(m_pSwapChain);
	SafeDelete(m_pCommandQueue);
}

void DXApplication::OnInit()
{
	InitRenderEnvironment();
	InitPathTracingPass();
	InitVisualizePathTracedResultPass();
}

void DXApplication::OnUpdate(float deltaTimeInMS)
{
	KeyboardInput::Poll();

	assert(m_pCamera != nullptr);
	m_pCamera->Update(deltaTimeInMS);

	AppData* pAppData = (AppData*)m_AppData[m_BackBufferIndex];
	pAppData->m_CameraWorldPos = m_pCamera->GetWorldPosition();
	pAppData->m_CameraWorldAxisX = m_pCamera->GetWorldOrientation().m_XAxis;
	pAppData->m_CameraWorldAxisY = m_pCamera->GetWorldOrientation().m_YAxis;
	pAppData->m_CameraWorldAxisZ = m_pCamera->GetWorldOrientation().m_ZAxis;
	pAppData->m_RayMinExtent = 0.0001f;
	pAppData->m_RayMaxExtent = 1.5f * m_pCamera->GetFarClipDistance();
}

void DXApplication::OnRender()
{
#ifdef ENABLE_PROFILING
	m_pGPUProfiler->StartFrame();
#endif // ENABLE_PROFILING
	
	static const u8 commandListBatchSize = 3;
	static CommandList* commandListBatch[commandListBatchSize];
	commandListBatch[0] = RecordPathTracingPass();
	commandListBatch[1] = RecordVisualizePathTracedResultPass();
	commandListBatch[2] = RecordPostRenderPass();
	
#ifdef ENABLE_PROFILING
	m_pGPUProfiler->EndFrame(m_pCommandQueue);
	m_pGPUProfiler->OutputToConsole();
#endif // #ifdef ENABLE_PROFILING

	++m_pRenderEnv->m_LastSubmissionFenceValue;
	m_pCommandQueue->ExecuteCommandLists(commandListBatchSize, commandListBatch, m_pFence, m_pRenderEnv->m_LastSubmissionFenceValue);
	
	++m_pRenderEnv->m_LastSubmissionFenceValue;
#ifdef ENABLE_PROFILING
	m_pSwapChain->Present(0/*vsync disabled*/, 0);
#else // ENABLE_PROFILING
	m_pSwapChain->Present(1/*vsync enabled*/, 0);
#endif // ENABLE_PROFILING
	m_pCommandQueue->Signal(m_pFence, m_pRenderEnv->m_LastSubmissionFenceValue);

#ifdef DEBUG_RENDER_PASS
	m_pFence->WaitForSignalOnCPU(m_pRenderEnv->m_LastSubmissionFenceValue);
	OuputDebugRenderPassResult();
#endif // DEBUG_RENDER_PASS

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

	DescriptorHeapDesc shaderInvisibleSRVHeapDesc(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 32, false);
	m_pShaderInvisibleSRVHeap = new DescriptorHeap(m_pDevice, &shaderInvisibleSRVHeapDesc, L"m_pShaderInvisibleSRVHeap");

	DescriptorHeapDesc shaderVisibleSRVHeapDesc(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 32, true);
	m_pShaderVisibleSRVHeap = new DescriptorHeap(m_pDevice, &shaderVisibleSRVHeapDesc, L"m_pShaderVisibleSRVHeap");

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
	m_pRenderEnv->m_pReadbackHeapProps = m_pReadbackHeapProps;
	m_pRenderEnv->m_pShaderInvisibleRTVHeap = m_pShaderInvisibleRTVHeap;
	m_pRenderEnv->m_pShaderInvisibleSRVHeap = m_pShaderInvisibleSRVHeap;
	m_pRenderEnv->m_pShaderVisibleSRVHeap = m_pShaderVisibleSRVHeap;

#ifdef ENABLE_PROFILING
	m_pGPUProfiler = new GPUProfiler(m_pRenderEnv, 8/*maxNumProfiles*/, kNumBackBuffers);
#endif // ENABLE_PROFILING
	m_pRenderEnv->m_pGPUProfiler = m_pGPUProfiler;
	
	m_pViewport = new Viewport(0.0f, 0.0f, FLOAT(kBackBufferWidth), FLOAT(kBackBufferHeight));
	m_pScissorRect = new Rect(0, 0, kBackBufferWidth, kBackBufferHeight);

	SwapChainDesc swapChainDesc(kNumBackBuffers, m_pWindow->GetHWND(), kBackBufferWidth, kBackBufferHeight);
	m_pSwapChain = new SwapChain(&factory, m_pRenderEnv, &swapChainDesc, m_pCommandQueue);
	m_BackBufferIndex = m_pSwapChain->GetCurrentBackBufferIndex();

	MemoryRange readRange(0, 0);
	ConstantBufferDesc appDataBufferDesc(sizeof(AppData));
	for (u8 index = 0; index < kNumBackBuffers; ++index)
	{
		m_AppDataBuffers[index] = new Buffer(m_pRenderEnv, m_pRenderEnv->m_pUploadHeapProps, &appDataBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, L"m_pAppDataBuffer");
		m_AppData[index] = m_AppDataBuffers[index]->Map(0, &readRange);
	}
	
	assert(m_pCamera == nullptr);
	m_pCamera = new Camera(Vector3f::ZERO, BasisAxes(), PI_DIV_4, 1.0f, 0.1f, 20.0f, Vector3f(0.01f), Vector3f(0.001f));
}

void DXApplication::InitPathTracingPass()
{
	assert(m_pPathTracingPass == nullptr);	
	MeshBatch meshBatch(VertexData::FormatFlag_Position, DXGI_FORMAT_R16_UINT, D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE, D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	const Matrix4f worldMatrices[] =
	{
		CreateTranslationMatrix( 0.0f, 0.0f, 0.0f),
		CreateTranslationMatrix(-2.5f, 0.0f, 5.0f),
		CreateTranslationMatrix(-7.5f, 0.0f, 5.0f),
		CreateTranslationMatrix( 2.5f, 0.0f, 5.0f),
		CreateTranslationMatrix( 7.5f, 0.0f, 5.0f),
	};
	for (u32 index = 0; index < ARRAYSIZE(worldMatrices); ++index)
	{
		const Vector3f positions[] = {Vector3f(0.0f, 2.0f, 5.0f), Vector3f(2.0f, -2.0f, 5.0f), Vector3f(-2.0f, -2.0f, 5.0f)};
		const u16 indices[] = {0, 1, 2};

		VertexData* pVertexData = new VertexData(ARRAYSIZE(positions), positions);
		IndexData* pIndexData = new IndexData(ARRAYSIZE(indices), indices);

		const u32 numInstances = 1;
		Matrix4f* pInstanceWorldMatrices = new Matrix4f[numInstances];
		pInstanceWorldMatrices[0] = worldMatrices[index];

		Mesh mesh(pVertexData, pIndexData, numInstances, pInstanceWorldMatrices, -1, D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE, D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		meshBatch.AddMesh(&mesh);
	}

	PathTracingPass::InitParams params;
	params.m_pRenderEnv = m_pRenderEnv;
	params.m_InputResourceStates.m_RayTracedResultState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	params.m_pMeshBatch = &meshBatch;
	params.m_NumRaysX = kBackBufferWidth;
	params.m_NumRaysY = kBackBufferHeight;

	m_pPathTracingPass = new PathTracingPass(&params);
}

CommandList* DXApplication::RecordPathTracingPass()
{
	assert(m_pPathTracingPass != nullptr);

	PathTracingPass::RenderParams params;
	params.m_pRenderEnv = m_pRenderEnv;
	params.m_pCommandList = m_pCommandListPool->Create(L"pPathTracingCommandList");
	params.m_pAppDataBuffer = m_AppDataBuffers[m_BackBufferIndex];
	params.m_NumRaysX = kBackBufferWidth;
	params.m_NumRaysY = kBackBufferHeight;
	
	m_pPathTracingPass->Record(&params);
	return params.m_pCommandList;
}

void DXApplication::InitVisualizePathTracedResultPass()
{
	assert(m_pPathTracingPass != nullptr);
	
	const PathTracingPass::ResourceStates* pPathTracingPassResourceStates = m_pPathTracingPass->GetOutputResourceStates();
	ColorTexture* pRayTracedResult = m_pPathTracingPass->GetPathTracedResult();

	for (u8 index = 0; index < kNumBackBuffers; ++index)
	{
		assert(m_VisualizePathTracedResultPasses[index] == nullptr);

		VisualizeTexturePass::InitParams params;
		params.m_pName = "VisualizePathTracedResultPass";
		params.m_pRenderEnv = m_pRenderEnv;
		params.m_InputResourceStates.m_InputTextureState = pPathTracingPassResourceStates->m_RayTracedResultState;
		params.m_InputResourceStates.m_BackBufferState = D3D12_RESOURCE_STATE_PRESENT;
		params.m_pInputTexture = pRayTracedResult;
		params.m_InputTextureSRV = pRayTracedResult->GetSRVHandle();
		params.m_pBackBuffer = m_pSwapChain->GetBackBuffer(index);
		params.m_TextureType = VisualizeTexturePass::TextureType_RGB;

		m_VisualizePathTracedResultPasses[index] = new VisualizeTexturePass(&params);
	}
}

CommandList* DXApplication::RecordVisualizePathTracedResultPass()
{
	VisualizeTexturePass* pVisualizeRayTracedResultPass = m_VisualizePathTracedResultPasses[m_BackBufferIndex];
	assert(pVisualizeRayTracedResultPass != nullptr);
		
	VisualizeTexturePass::RenderParams params;
	params.m_pRenderEnv = m_pRenderEnv;
	params.m_pCommandList = m_pCommandListPool->Create(L"pVisualizePathTracedResultCommandList");;
	params.m_pViewport = m_pViewport;

	pVisualizeRayTracedResultPass->Record(&params);
	return params.m_pCommandList;
}

CommandList* DXApplication::RecordPostRenderPass()
{
	CommandList* pCommandList = m_pCommandListPool->Create(L"pPostRenderCommandList");
	pCommandList->Begin();
#ifdef ENABLE_PROFILING
	u32 profileIndex = m_pGPUProfiler->StartProfile(pCommandList, "PostRenderPass");
#endif // ENABLE_PROFILING

	ColorTexture* pRenderTarget = m_pSwapChain->GetBackBuffer(m_BackBufferIndex);
	ResourceTransitionBarrier resourceBarrier(pRenderTarget, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
	pCommandList->ResourceBarrier(1, &resourceBarrier);

#ifdef ENABLE_PROFILING
	m_pGPUProfiler->EndProfile(pCommandList, profileIndex);
#endif // ENABLE_PROFILING
	pCommandList->End();

	return pCommandList;
}
