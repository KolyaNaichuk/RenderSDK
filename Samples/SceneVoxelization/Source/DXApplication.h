#pragma once

#include "Common/Application.h"

class DXDevice;
class DXSwapChain;
class DXCommandQueue;
class DXCommandAllocator;
class DXDescriptorHeap;
class DXFence;
class DXEvent;

class ClearVoxelGridRecorder;

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
	DXCommandAllocator* m_CommandAllocators[kBackBufferCount];
	DXDescriptorHeap* m_pRTVHeap;
	DXFence* m_pFence;
	DXEvent* m_pFenceEvent;
	UINT64 m_FenceValues[kBackBufferCount];
	UINT m_BackBufferIndex;

	ClearVoxelGridRecorder* m_pClearVoxelGridRecorder;
};
