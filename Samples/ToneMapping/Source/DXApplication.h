#pragma once

#include "Common/Application.h"

class GraphicsDevice;
class SwapChain;
class CommandQueue;
class CommandList;
class CommandAllocator;
class DescriptorHeap;
class ColorTexture;
class Fence;

struct RenderEnv;
struct HeapProperties;

class CopyTexturePass;
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
	enum { kBackBufferCount = 3 };

	GraphicsDevice* m_pDevice;
	SwapChain* m_pSwapChain;
	CommandQueue* m_pCommandQueue;
	DescriptorHeap* m_pShaderInvisibleRTVHeap;
	DescriptorHeap* m_pShaderInvisibleSRVHeap;
	DescriptorHeap* m_pShaderInvisibleSamplerHeap;
	CommandAllocator* m_CommandAllocators[kBackBufferCount];
	CommandList* m_pCommandList;
	Fence* m_pFence;
	UINT64 m_FenceValues[kBackBufferCount];
	UINT m_BackBufferIndex;

	ColorTexture* m_pHDRTexture;
	CopyTexturePass* m_pCopyTexturePass;
	CalcTextureLuminancePass* m_pCalcTextureLuminancePass;
	CalcTextureLuminancePass* m_pCalcTextureLogLuminancePass;

	DisplayResult m_DisplayResult;
};
