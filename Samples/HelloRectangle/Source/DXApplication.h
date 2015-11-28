#pragma once

#include "Common/Application.h"

class DXDevice;
class DXSwapChain;
class DXCommandQueue;
class DXCommandList;
class DXCommandAllocator;
class DXDescriptorHeap;
class DXRootSignature;
class DXPipelineState;
class DXResource;
class DXFence;
class DXEvent;

struct DXVertexBufferView;
struct DXIndexBufferView;
struct DXViewport;
struct DXRect;

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

	DXDevice* m_pDevice;
	DXSwapChain* m_pSwapChain;
	DXCommandQueue* m_pCommandQueue;
	DXDescriptorHeap* m_pRTVHeap;
	DXRootSignature* m_pRootSignature;
	DXPipelineState* m_pPipelineState;
	DXCommandAllocator* m_CommandAllocators[kBackBufferCount];
	DXCommandList* m_pCommandList;
	DXResource* m_pVertexBuffer;
	DXVertexBufferView* m_pVertexBufferView;
	DXResource* m_pIndexBuffer;
	DXIndexBufferView* m_pIndexBufferView;
	DXFence* m_pFence;
	DXEvent* m_pFenceEvent;
	UINT64 m_FenceValues[kBackBufferCount];
	DXViewport* m_pViewport;
	DXRect* m_pScissorRect;
	UINT m_BackBufferIndex;
};
