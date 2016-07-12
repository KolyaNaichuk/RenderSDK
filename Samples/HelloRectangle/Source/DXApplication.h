#pragma once

#include "Common/Application.h"

class D3DDevice;
class D3DSwapChain;
class D3DCommandQueue;
class D3DCommandList;
class D3DCommandAllocator;
class D3DDescriptorHeap;
class D3DRootSignature;
class D3DPipelineState;
class D3DBuffer;
class D3DColorTexture;
class D3DFence;

struct D3DHeapProperties;
struct D3DRenderEnv;
struct D3DVertexBufferView;
struct D3DIndexBufferView;
struct D3DViewport;
struct D3DRect;

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

	D3DDevice* m_pDevice;
	D3DSwapChain* m_pSwapChain;
	D3DCommandQueue* m_pCommandQueue;
	D3DDescriptorHeap* m_pShaderInvisibleRTVHeap;
	D3DDescriptorHeap* m_pShaderInvisibleSRVHeap;
	D3DRootSignature* m_pRootSignature;
	D3DPipelineState* m_pPipelineState;
	D3DCommandAllocator* m_CommandAllocators[kBackBufferCount];
	D3DCommandList* m_pCommandList;
	D3DBuffer* m_pVertexBuffer;
	D3DBuffer* m_pIndexBuffer;
	D3DHeapProperties* m_pDefaultHeapProps;
	D3DHeapProperties* m_pUploadHeapProps;
	D3DRenderEnv* m_pRenderEnv;
	D3DFence* m_pFence;
	UINT64 m_FenceValues[kBackBufferCount];
	D3DViewport* m_pViewport;
	D3DRect* m_pScissorRect;
	UINT m_BackBufferIndex;
};
