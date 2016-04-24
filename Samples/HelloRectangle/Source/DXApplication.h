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
class DXBuffer;
class DXColorTexture;
class DXFence;
class DXEvent;

struct DXHeapProperties;
struct DXRenderEnvironment;
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
	DXDescriptorHeap* m_pRTVDescriptorHeap;
	DXRootSignature* m_pRootSignature;
	DXPipelineState* m_pPipelineState;
	DXCommandAllocator* m_CommandAllocators[kBackBufferCount];
	DXCommandList* m_pCommandList;
	DXBuffer* m_pVertexBuffer;
	DXBuffer* m_pIndexBuffer;
	DXHeapProperties* m_pDefaultHeapProps;
	DXHeapProperties* m_pUploadHeapProps;
	DXRenderEnvironment* m_pEnv;
	DXFence* m_pFence;
	DXEvent* m_pFenceEvent;
	UINT64 m_FenceValues[kBackBufferCount];
	DXViewport* m_pViewport;
	DXRect* m_pScissorRect;
	UINT m_BackBufferIndex;
};
