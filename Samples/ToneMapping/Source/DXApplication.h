#pragma once

#include "Common/Application.h"

class GraphicsDevice;
class SwapChain;
class CommandQueue;
class CommandList;
class DescriptorHeap;
class ColorTexture;
class Fence;

struct RenderEnv;
struct HeapProperties;

class CalcTextureLuminancePass;

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
	enum { kNumBackBuffers = 3 };

	GraphicsDevice* m_pDevice;
	SwapChain* m_pSwapChain;
	CommandQueue* m_pCommandQueue;
	DescriptorHeap* m_pShaderInvisibleRTVHeap;
	DescriptorHeap* m_pShaderInvisibleSRVHeap;
	DescriptorHeap* m_pShaderInvisibleSamplerHeap;
	Fence* m_pFence;
	CommandList* m_CommandLists[kNumBackBuffers];
	UINT64 m_FenceValues[kNumBackBuffers];
	UINT m_BackBufferIndex;

	ColorTexture* m_pHDRTexture;
	CalcTextureLuminancePass* m_pCalcTextureLuminancePass;
	CalcTextureLuminancePass* m_pCalcTextureLogLuminancePass;

	DisplayResult m_DisplayResult;
};
