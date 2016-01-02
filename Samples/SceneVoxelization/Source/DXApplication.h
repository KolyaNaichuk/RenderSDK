#pragma once

#include "Common/Application.h"

class DXDevice;
class DXSwapChain;
class DXCommandQueue;
class DXCommandAllocator;
class DXCommandList;
class DXDescriptorHeap;
class DXResource;
class DXFence;
class DXEvent;

class ClearVoxelGridRecorder;
class VisualizeMeshRecorder;
class Mesh;
class Camera;

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
	DXCommandList* m_pCommandList;
	DXDescriptorHeap* m_pRTVHeap;
	DXDescriptorHeap* m_pDSVHeap;
	DXDescriptorHeap* m_pCBVHeap;
	DXResource* m_pDSVTexture;
	DXResource* m_pCBVBuffer;
	DXFence* m_pFence;
	DXEvent* m_pFenceEvent;
	UINT64 m_FenceValues[kBackBufferCount];
	UINT m_BackBufferIndex;

	ClearVoxelGridRecorder* m_pClearVoxelGridRecorder;
	VisualizeMeshRecorder* m_pVisualizeNormalRecorder;
	VisualizeMeshRecorder* m_pVisualizeColorRecorder;
	Mesh* m_pMesh;
	Camera* m_pCamera;
};
