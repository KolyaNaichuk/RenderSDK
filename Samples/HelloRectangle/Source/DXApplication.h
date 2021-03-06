#pragma once

#include "Common/Application.h"

class GraphicsDevice;
class SwapChain;
class CommandQueue;
class CommandListPool;
class DescriptorHeap;
class RootSignature;
class PipelineState;
class Buffer;
class ColorTexture;
class Fence;

struct HeapProperties;
struct RenderEnv;
struct VertexBufferView;
struct IndexBufferView;
struct Viewport;
struct Rect;

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
	
private:
	enum { kNumBackBuffers = 3 };

	GraphicsDevice* m_pDevice;
	SwapChain* m_pSwapChain;
	CommandQueue* m_pCommandQueue;
	CommandListPool* m_pCommandListPool;
	DescriptorHeap* m_pShaderInvisibleRTVHeap;
	DescriptorHeap* m_pShaderInvisibleSRVHeap;
	RootSignature* m_pRootSignature;
	PipelineState* m_pPipelineState;
	Buffer* m_pVertexBuffer;
	Buffer* m_pIndexBuffer;
	HeapProperties* m_pDefaultHeapProps;
	HeapProperties* m_pUploadHeapProps;
	RenderEnv* m_pRenderEnv;
	Fence* m_pFence;
	Viewport* m_pViewport;
	Rect* m_pScissorRect;
	UINT m_BackBufferIndex;
	UINT64 m_FrameCompletionFenceValues[kNumBackBuffers];
};
