#pragma once

#include "Common/Application.h"

class DXDevice;
class DXSwapChain;
class DXCommandQueue;
class DXCommandList;
class DXCommandAllocator;
class DXDescriptorHeap;
class DXColorTexture;
class DXFence;
class DXEvent;

struct DXRenderEnvironment;
struct DXHeapProperties;

class CopyTextureRecorder;
class CalcTextureLuminanceRecorder;

enum DisplayResult
{
	DisplayResult_HDRImage,
	DisplayResult_ImageLuminance,
	DisplayResult_ImageLogLuminance
};

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
	DXDescriptorHeap* m_pSRVDescriptorHeap;
	DXDescriptorHeap* m_pSamplerDescriptorHeap;
	DXCommandAllocator* m_CommandAllocators[kBackBufferCount];
	DXCommandList* m_pCommandList;
	DXFence* m_pFence;
	DXEvent* m_pFenceEvent;
	UINT64 m_FenceValues[kBackBufferCount];
	UINT m_BackBufferIndex;

	DXColorTexture* m_pHDRTexture;
	CopyTextureRecorder* m_pCopyTextureRecorder;
	CalcTextureLuminanceRecorder* m_pCalcTextureLuminanceRecorder;
	CalcTextureLuminanceRecorder* m_pCalcTextureLogLuminanceRecorder;

	DisplayResult m_DisplayResult;
};
