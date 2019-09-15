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

class PathTracingPass;
class VisualizeTexturePass;
class Camera;

class DXApplication : public Application
{
public:
	DXApplication(HINSTANCE hApp);
	~DXApplication();

private:
	void OnInit() override;
	void OnUpdate(float deltaTimeInMS) override;
	void OnRender() override;
	void OnDestroy() override;

	void InitRenderEnvironment();

	void InitPathTracingPass();
	CommandList* RecordPathTracingPass();

	void InitVisualizePathTracedResultPass();
	CommandList* RecordVisualizePathTracedResultPass();

	CommandList* RecordPostRenderPass();
	
private:
	enum { kNumBackBuffers = 3 };

	GraphicsDevice* m_pDevice = nullptr;
	SwapChain* m_pSwapChain = nullptr;
	CommandQueue* m_pCommandQueue = nullptr;
	CommandListPool* m_pCommandListPool = nullptr;
	DescriptorHeap* m_pShaderInvisibleRTVHeap = nullptr;
	DescriptorHeap* m_pShaderInvisibleSRVHeap = nullptr;
	DescriptorHeap* m_pShaderVisibleSRVHeap = nullptr;
	HeapProperties* m_pDefaultHeapProps = nullptr;
	HeapProperties* m_pUploadHeapProps = nullptr;
	HeapProperties* m_pReadbackHeapProps = nullptr;
	RenderEnv* m_pRenderEnv = nullptr;
	Fence* m_pFence = nullptr;
	Viewport* m_pViewport = nullptr;
	Rect* m_pScissorRect = nullptr;
	UINT m_BackBufferIndex = 0;

	UINT64 m_FrameCompletionFenceValues[kNumBackBuffers] = {0, 0, 0};
	Buffer* m_AppDataBuffers[kNumBackBuffers] = {nullptr, nullptr, nullptr};
	void* m_AppData[kNumBackBuffers] = {nullptr, nullptr, nullptr};

	PathTracingPass* m_pPathTracingPass = nullptr;
	VisualizeTexturePass* m_VisualizePathTracedResultPasses[kNumBackBuffers] = {nullptr, nullptr, nullptr};
	GPUProfiler* m_pGPUProfiler = nullptr;
	Camera* m_pCamera = nullptr;
};
