#pragma once

#include "Common/Application.h"

class D3DDevice;
class D3DSwapChain;
class D3DCommandQueue;
class D3DCommandAllocator;
class D3DCommandList;
class D3DCommandListPool;
class D3DDescriptorHeap;
class D3DColorTexture;
class D3DDepthTexture;
class D3DBuffer;
class D3DFence;

class Camera;
class MeshBatch;
class LightBuffer;
class FillGBufferCommandsRecorder;
class FillGBufferRecorder;
class ClearVoxelGridRecorder;
class CreateVoxelGridRecorder;
class InjectVPLsIntoVoxelGridRecorder;
class VisualizeVoxelGridRecorder;
class VisualizeMeshRecorder;
class CopyTextureRecorder;
class TiledLightCullingRecorder;
class TiledShadingRecorder;
class ViewFrustumCullingRecorder;
class RenderShadowMapCommandsRecorder;
class RenderTiledShadowMapRecorder;

struct D3DHeapProperties;
struct D3DRenderEnv;
struct D3DResourceList;
struct D3DViewport;

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
	D3DDevice* m_pDevice;
	D3DSwapChain* m_pSwapChain;
	D3DCommandQueue* m_pCommandQueue;
	D3DCommandAllocator* m_CommandAllocators[kBackBufferCount];
	D3DCommandList* m_pCommandList;
	D3DCommandListPool* m_pCommandListPool;
	D3DHeapProperties* m_pDefaultHeapProps;
	D3DHeapProperties* m_pUploadHeapProps;
	D3DDescriptorHeap* m_pShaderInvisibleRTVHeap;
	D3DDescriptorHeap* m_pShaderInvisibleDSVHeap;
	D3DDescriptorHeap* m_pShaderInvisibleSRVHeap;
	D3DDescriptorHeap* m_pShaderVisibleSRVHeap;
	D3DDepthTexture* m_pDepthTexture;
	D3DDepthTexture* m_pSpotLightTiledShadowMap;
	D3DColorTexture* m_pDiffuseTexture;
	D3DColorTexture* m_pNormalTexture;
	D3DColorTexture* m_pSpecularTexture;
	D3DColorTexture* m_pAccumLightTexture;
	D3DViewport* m_pViewport;
	D3DBuffer* m_pObjectTransformBuffer;
	D3DBuffer* m_pCameraTransformBuffer;
	D3DBuffer* m_pGridBuffer;
	D3DBuffer* m_pGridConfigBuffer;
	D3DBuffer* m_pViewFrustumCullingDataBuffer;
	D3DBuffer* m_pTiledLightCullingDataBuffer;
	D3DBuffer* m_pTiledShadingDataBuffer;
	D3DBuffer* m_pDrawMeshCommandBuffer;
	D3DBuffer* m_pNumVisibleMeshesBuffer;
	D3DBuffer* m_pVisibleMeshIndexBuffer;
	D3DBuffer* m_pNumPointLightsPerTileBuffer;
	D3DBuffer* m_pPointLightIndexPerTileBuffer;
	D3DBuffer* m_pPointLightRangePerTileBuffer;
	D3DBuffer* m_pNumSpotLightsPerTileBuffer;
	D3DBuffer* m_pSpotLightIndexPerTileBuffer;
	D3DBuffer* m_pSpotLightRangePerTileBuffer;
	D3DBuffer* m_pShadowCastingPointLightIndexBuffer;
	D3DBuffer* m_pNumShadowCastingPointLightsBuffer;
	D3DBuffer* m_pDrawPointLightShadowCasterCommandBuffer;
	D3DBuffer* m_pNumDrawPointLightShadowCastersBuffer;
	D3DBuffer* m_pShadowCastingSpotLightIndexBuffer;
	D3DBuffer* m_pNumShadowCastingSpotLightsBuffer;
	D3DBuffer* m_pDrawSpotLightShadowCasterCommandBuffer;
	D3DBuffer* m_pNumDrawSpotLightShadowCastersBuffer;
	D3DRenderEnv* m_pRenderEnv;
	D3DFence* m_pFence;
	UINT64 m_FenceValues[kBackBufferCount];
	UINT m_BackBufferIndex;

	FillGBufferRecorder* m_pFillGBufferRecorder;
	D3DResourceList* m_pFillGBufferResources;

	TiledLightCullingRecorder* m_pTiledLightCullingRecorder;
	D3DResourceList* m_pTiledLightCullingResources;

	TiledShadingRecorder* m_pTiledShadingRecorder;
	D3DResourceList* m_pTiledShadingResources;

	ClearVoxelGridRecorder* m_pClearVoxelGridRecorder;
	D3DResourceList* m_pClearVoxelGridResources;

	CreateVoxelGridRecorder* m_pCreateVoxelGridRecorder;
	D3DResourceList* m_pCreateVoxelGridResources;

	InjectVPLsIntoVoxelGridRecorder* m_pInjectVPLsIntoVoxelGridRecorder;

	VisualizeVoxelGridRecorder* m_pVisualizeVoxelGridRecorder;
	D3DResourceList* m_VisualizeVoxelGridResources[kBackBufferCount];

	VisualizeMeshRecorder* m_pVisualizeMeshRecorder;
	D3DResourceList* m_VisualizeMeshResources[kBackBufferCount];

	ViewFrustumCullingRecorder* m_pDetectVisibleMeshesRecorder;
	D3DResourceList* m_pDetectVisibleMeshesResources;

	ViewFrustumCullingRecorder* m_pDetectVisiblePointLightsRecorder;
	D3DResourceList* m_pDetectVisiblePointLightsResources;

	ViewFrustumCullingRecorder* m_pDetectVisibleSpotLightsRecorder;
	D3DResourceList* m_pDetectVisibleSpotLightsResources;

	FillGBufferCommandsRecorder* m_pFillGBufferCommandsRecorder;
	D3DResourceList* m_pFillGBufferCommandsResources;

	RenderShadowMapCommandsRecorder* m_pRenderShadowMapCommandsRecorder;
	D3DResourceList* m_pRenderShadowMapCommandsResources;
	D3DBuffer* m_pRenderShadowMapCommandsArgumentBuffer;

	RenderTiledShadowMapRecorder* m_pRenderSpotLightTiledShadowMapRecorder;
	D3DResourceList* m_pRenderSpotLightTiledShadowMapResources;

	CopyTextureRecorder* m_pCopyTextureRecorder;
	D3DResourceList* m_CopyTextureResources[kBackBufferCount];

	MeshBatch* m_pMeshBatch;
	
	LightBuffer* m_pPointLightBuffer;
	D3DBuffer* m_pNumVisiblePointLightsBuffer;
	D3DBuffer* m_pVisiblePointLightIndexBuffer;

	LightBuffer* m_pSpotLightBuffer;
	D3DBuffer* m_pNumVisibleSpotLightsBuffer;
	D3DBuffer* m_pVisibleSpotLightIndexBuffer;
	
	Camera* m_pCamera;
};
