#pragma once

#include "Common/Application.h"

class GraphicsDevice;
class SwapChain;
class CommandList;
class CommandQueue;
class CommandListPool;
class DescriptorHeap;
class RootSignature;
class PipelineState;
class Buffer;
class ColorTexture;
class Fence;
class GPUProfiler;

struct HeapProperties;
struct RenderEnv;
struct VertexBufferView;
struct IndexBufferView;
struct Viewport;
struct Rect;

class RayTracingPass;
class VisualizeTexturePass;

class DXApplication : public Application
{
public:
	DXApplication(HINSTANCE hApp);
	~DXApplication();

private:
	void OnInit() override;
	void OnUpdate() override;
	void OnRender() override;
	void OnDestroy() override;

	void InitRenderEnvironment();

	void InitRayTracingPass();
	CommandList* RecordRayTracingPass();

	void InitVisualizeRayTracedResultPass();
	CommandList* RecordVisualizeRayTracedResultPass();
	
private:
	enum { kNumBackBuffers = 3 };

	GraphicsDevice* m_pDevice = nullptr;
	SwapChain* m_pSwapChain = nullptr;
	CommandQueue* m_pCommandQueue = nullptr;
	CommandListPool* m_pCommandListPool = nullptr;
	DescriptorHeap* m_pShaderInvisibleRTVHeap = nullptr;
	DescriptorHeap* m_pShaderInvisibleSRVHeap = nullptr;
	HeapProperties* m_pDefaultHeapProps = nullptr;
	HeapProperties* m_pUploadHeapProps = nullptr;
	RenderEnv* m_pRenderEnv = nullptr;
	Fence* m_pFence = nullptr;
	Viewport* m_pViewport = nullptr;
	Rect* m_pScissorRect = nullptr;
	UINT m_BackBufferIndex = 0;
	UINT64 m_FrameCompletionFenceValues[kNumBackBuffers] = {0, 0, 0};

	Buffer* m_pAppDataBuffer = nullptr;
	RayTracingPass* m_pRayTracingPass = nullptr;
	VisualizeTexturePass* m_pVisualizeRayTracedResultPass = nullptr;
	GPUProfiler* m_pGPUProfiler = nullptr;
};
