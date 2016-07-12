#pragma once

#include "Common/Application.h"

class D3DDevice;
class D3DSwapChain;
class D3DCommandQueue;
class D3DCommandList;
class D3DCommandAllocator;
class D3DDescriptorHeap;
class D3DColorTexture;
class D3DFence;

struct D3DRenderEnv;
struct D3DHeapProperties;

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

	D3DDevice* m_pDevice;
	D3DSwapChain* m_pSwapChain;
	D3DCommandQueue* m_pCommandQueue;
	D3DDescriptorHeap* m_pShaderInvisibleRTVHeap;
	D3DDescriptorHeap* m_pShaderInvisibleSRVHeap;
	D3DDescriptorHeap* m_pShaderInvisibleSamplerHeap;
	D3DCommandAllocator* m_CommandAllocators[kBackBufferCount];
	D3DCommandList* m_pCommandList;
	D3DFence* m_pFence;
	UINT64 m_FenceValues[kBackBufferCount];
	UINT m_BackBufferIndex;

	D3DColorTexture* m_pHDRTexture;
	CopyTextureRecorder* m_pCopyTextureRecorder;
	CalcTextureLuminanceRecorder* m_pCalcTextureLuminanceRecorder;
	CalcTextureLuminanceRecorder* m_pCalcTextureLogLuminanceRecorder;

	DisplayResult m_DisplayResult;
};
