#pragma once

#include "Common/Application.h"

class GraphicsDevice;
class SwapChain;
class CommandQueue;
class CommandList;
class CommandAllocator;
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
	virtual void OnInit();
	virtual void OnUpdate();
	virtual void OnRender();
	virtual void OnDestroy();
	virtual void OnKeyDown(UINT8 key);
	virtual void OnKeyUp(UINT8 key);

	void WaitForGPU();
	void MoveToNextFrame();

private:
	enum { kBackBufferCount = 3 };

	GraphicsDevice* m_pDevice;
	SwapChain* m_pSwapChain;
	CommandQueue* m_pCommandQueue;
	DescriptorHeap* m_pShaderInvisibleRTVHeap;
	DescriptorHeap* m_pShaderInvisibleSRVHeap;
	RootSignature* m_pRootSignature;
	PipelineState* m_pPipelineState;
	CommandAllocator* m_CommandAllocators[kBackBufferCount];
	CommandList* m_pCommandList;
	Buffer* m_pVertexBuffer;
	Buffer* m_pIndexBuffer;
	HeapProperties* m_pDefaultHeapProps;
	HeapProperties* m_pUploadHeapProps;
	RenderEnv* m_pRenderEnv;
	Fence* m_pFence;
	UINT64 m_FenceValues[kBackBufferCount];
	Viewport* m_pViewport;
	Rect* m_pScissorRect;
	UINT m_BackBufferIndex;
};
