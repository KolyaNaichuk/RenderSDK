#pragma once

#include "Common/Application.h"

class DXDevice;
class DXSwapChain;
class DXCommandQueue;
class DXCommandAllocator;
class DXCommandList;
class DXDescriptorHeap;
class DXRenderTarget;
class DXDepthStencilTexture;
class DXBuffer;
class DXSampler;
class DXFence;
class DXEvent;

class Mesh;
class Camera;
class FillGBufferRecorder;
class ClearVoxelGridRecorder;
class CreateVoxelGridRecorder;
class InjectVPLsIntoVoxelGridRecorder;
class VisualizeVoxelGridRecorder;
class VisualizeMeshRecorder;
class TiledShadingRecorder;
class ViewFrustumCullingRecorder;

struct DXHeapProperties;
struct DXRenderEnvironment;

enum
{ 
	kBackBufferCount = 3
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
	DXDevice* m_pDevice;
	DXSwapChain* m_pSwapChain;
	DXCommandQueue* m_pCommandQueue;
	DXCommandAllocator* m_CommandAllocators[kBackBufferCount];
	DXCommandList* m_pCommandList;
	DXHeapProperties* m_pDefaultHeapProps;
	DXHeapProperties* m_pUploadHeapProps;
	DXDescriptorHeap* m_pRTVDescriptorHeap;
	DXDescriptorHeap* m_pDSVDescriptorHeap;
	DXDescriptorHeap* m_pSRVDescriptorHeap;
	DXDescriptorHeap* m_pSamplerDescriptorHeap;
	DXDepthStencilTexture* m_pDepthTexture;
	DXRenderTarget* m_pDiffuseTexture;
	DXRenderTarget* m_pNormalTexture;
	DXRenderTarget* m_pSpecularTexture;
	DXRenderTarget* m_pAccumLightTexture;
	DXBuffer* m_pObjectTransformBuffer;
	DXBuffer* m_pCameraTransformBuffer;
	DXBuffer* m_pGridBuffer;
	DXBuffer* m_pGridConfigBuffer;
	DXSampler* m_pAnisoSampler;
	DXRenderEnvironment* m_pEnv;
	DXFence* m_pFence;
	DXEvent* m_pFenceEvent;
	UINT64 m_FenceValues[kBackBufferCount];
	UINT m_BackBufferIndex;

	FillGBufferRecorder* m_pFillGBufferRecorder;
	TiledShadingRecorder* m_pTiledShadingRecorder;
	ClearVoxelGridRecorder* m_pClearVoxelGridRecorder;
	CreateVoxelGridRecorder* m_pCreateVoxelGridRecorder;
	InjectVPLsIntoVoxelGridRecorder* m_pInjectVPLsIntoVoxelGridRecorder;
	VisualizeVoxelGridRecorder* m_pVisualizeVoxelGridRecorder;
	VisualizeMeshRecorder* m_pVisualizeMeshRecorder;
	ViewFrustumCullingRecorder* m_pViewFrustumCullingRecorder;

	Mesh* m_pMesh;
	Camera* m_pCamera;
};
