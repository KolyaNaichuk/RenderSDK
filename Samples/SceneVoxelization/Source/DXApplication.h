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
class RenderGBufferCommandsPass;
class RenderGBufferPass;
class ClearVoxelGridPass;
class CreateVoxelGridPass;
class InjectVPLsIntoVoxelGridPass;
class VisualizeVoxelGridPass;
class VisualizeMeshPass;
class CopyTexturePass;
class TiledLightCullingPass;
class TiledShadingPass;
class ViewFrustumCullingPass;
class RenderShadowMapCommandsPass;
class RenderTiledShadowMapPass;

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

	RenderGBufferPass* m_pRenderGBufferPass;
	D3DResourceList* m_pRenderGBufferResources;

	TiledLightCullingPass* m_pTiledLightCullingPass;
	D3DResourceList* m_pTiledLightCullingResources;

	TiledShadingPass* m_pTiledShadingPass;
	D3DResourceList* m_pTiledShadingResources;

	ClearVoxelGridPass* m_pClearVoxelGridPass;
	D3DResourceList* m_pClearVoxelGridResources;

	CreateVoxelGridPass* m_pCreateVoxelGridPass;
	D3DResourceList* m_pCreateVoxelGridResources;

	InjectVPLsIntoVoxelGridPass* m_pInjectVPLsIntoVoxelGridPass;

	VisualizeVoxelGridPass* m_pVisualizeVoxelGridPass;
	D3DResourceList* m_VisualizeVoxelGridResources[kBackBufferCount];

	VisualizeMeshPass* m_pVisualizeMeshPass;
	D3DResourceList* m_VisualizeMeshResources[kBackBufferCount];

	ViewFrustumCullingPass* m_pDetectVisibleMeshesPass;
	D3DResourceList* m_pDetectVisibleMeshesResources;

	ViewFrustumCullingPass* m_pDetectVisiblePointLightsPass;
	D3DResourceList* m_pDetectVisiblePointLightsResources;

	ViewFrustumCullingPass* m_pDetectVisibleSpotLightsPass;
	D3DResourceList* m_pDetectVisibleSpotLightsResources;

	RenderGBufferCommandsPass* m_pRenderGBufferCommandsPass;
	D3DResourceList* m_pRenderGBufferCommandsResources;

	RenderShadowMapCommandsPass* m_pRenderShadowMapCommandsPass;
	D3DResourceList* m_pRenderShadowMapCommandsResources;
	D3DBuffer* m_pRenderShadowMapCommandsArgumentBuffer;

	RenderTiledShadowMapPass* m_pRenderSpotLightTiledShadowMapPass;
	D3DResourceList* m_pRenderSpotLightTiledShadowMapResources;

	CopyTexturePass* m_pCopyTexturePass;
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
