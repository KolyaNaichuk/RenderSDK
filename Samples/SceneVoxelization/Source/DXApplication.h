#pragma once

#include "Common/Application.h"

class GraphicsDevice;
class SwapChain;
class CommandQueue;
class CommandListPool;
class DescriptorHeap;
class ColorTexture;
class DepthTexture;
class Buffer;
class Fence;

class Camera;
class MeshBatch;
class LightBuffer;
class CreateRenderGBufferCommandsPass;
class RenderGBufferPass;
class ClearVoxelGridPass;
class CreateVoxelGridPass;
class InjectVPLsIntoVoxelGridPass;
class VisualizeVoxelGridPass;
class CopyTexturePass;
class TiledLightCullingPass;
class TiledShadingPass;
class ViewFrustumCullingPass;
class CreateRenderShadowMapCommandsPass;
class RenderTiledShadowMapPass;

struct HeapProperties;
struct RenderEnv;
struct BindingResourceList;
struct Viewport;

#define FOR_DEBUG_ONLY
//#define ENABLE_INDIRECT_LIGHTING

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

private:
	enum { kNumBackBuffers = 3 };

	GraphicsDevice* m_pDevice;
	SwapChain* m_pSwapChain;
	CommandQueue* m_pCommandQueue;
	CommandListPool* m_pCommandListPool;
	HeapProperties* m_pUploadHeapProps;
	HeapProperties* m_pDefaultHeapProps;
	HeapProperties* m_pReadbackHeapProps;
	DescriptorHeap* m_pShaderInvisibleRTVHeap;
	DescriptorHeap* m_pShaderInvisibleDSVHeap;
	DescriptorHeap* m_pShaderInvisibleSRVHeap;
	DescriptorHeap* m_pShaderVisibleSRVHeap;
	DepthTexture* m_pDepthTexture;
	DepthTexture* m_pSpotLightTiledShadowMap;
	ColorTexture* m_pDiffuseTexture;
	ColorTexture* m_pNormalTexture;
	ColorTexture* m_pSpecularTexture;
	ColorTexture* m_pAccumLightTexture;
	Viewport* m_pViewport;
	Buffer* m_pObjectTransformBuffer;
	Buffer* m_pCameraTransformBuffer;
	Buffer* m_pGridBuffer;
	Buffer* m_pGridConfigBuffer;
	Buffer* m_pViewFrustumMeshCullingDataBuffer;
	Buffer* m_pViewFrustumSpotLightCullingDataBuffer;
	Buffer* m_pViewFrustumPointLightCullingDataBuffer;
	Buffer* m_pTiledLightCullingDataBuffer;
	Buffer* m_pTiledShadingDataBuffer;
	Buffer* m_pDrawMeshCommandBuffer;
	Buffer* m_pNumVisibleMeshesBuffer;
	Buffer* m_pVisibleMeshIndexBuffer;
	Buffer* m_pNumPointLightsPerTileBuffer;
	Buffer* m_pPointLightIndexPerTileBuffer;
	Buffer* m_pPointLightRangePerTileBuffer;
	Buffer* m_pNumSpotLightsPerTileBuffer;
	Buffer* m_pSpotLightIndexPerTileBuffer;
	Buffer* m_pSpotLightRangePerTileBuffer;
	Buffer* m_pShadowCastingPointLightIndexBuffer;
	Buffer* m_pNumShadowCastingPointLightsBuffer;
	Buffer* m_pDrawPointLightShadowCasterCommandBuffer;
	Buffer* m_pNumDrawPointLightShadowCastersBuffer;
	Buffer* m_pShadowCastingSpotLightIndexBuffer;
	Buffer* m_pNumShadowCastingSpotLightsBuffer;
	Buffer* m_pDrawSpotLightShadowCasterCommandBuffer;
	Buffer* m_pNumDrawSpotLightShadowCastersBuffer;
	RenderEnv* m_pRenderEnv;
	Fence* m_pFence;
	UINT64 m_LastSubmissionFenceValue;
	UINT64 m_FrameCompletionFenceValues[kNumBackBuffers];
	UINT m_BackBufferIndex;

	RenderGBufferPass* m_pRenderGBufferPass;
	BindingResourceList* m_pRenderGBufferResources;

	TiledLightCullingPass* m_pTiledLightCullingPass;
	BindingResourceList* m_pTiledLightCullingResources;

	TiledShadingPass* m_pTiledShadingPass;
	BindingResourceList* m_pTiledShadingResources;

	ClearVoxelGridPass* m_pClearVoxelGridPass;
	BindingResourceList* m_pClearVoxelGridResources;

	CreateVoxelGridPass* m_pCreateVoxelGridPass;
	BindingResourceList* m_pCreateVoxelGridResources;

	InjectVPLsIntoVoxelGridPass* m_pInjectVPLsIntoVoxelGridPass;

	VisualizeVoxelGridPass* m_pVisualizeVoxelGridPass;
	BindingResourceList* m_VisualizeVoxelGridResources[kNumBackBuffers];

	ViewFrustumCullingPass* m_pDetectVisibleMeshesPass;
	BindingResourceList* m_pDetectVisibleMeshesResources;

	ViewFrustumCullingPass* m_pDetectVisiblePointLightsPass;
	BindingResourceList* m_pDetectVisiblePointLightsResources;

	ViewFrustumCullingPass* m_pDetectVisibleSpotLightsPass;
	BindingResourceList* m_pDetectVisibleSpotLightsResources;

	CreateRenderGBufferCommandsPass* m_pCreateRenderGBufferCommandsPass;
	BindingResourceList* m_pCreateRenderGBufferCommandsResources;

	BindingResourceList* m_pCreateRenderShadowMapCommandsArgumentBufferResources;
	CreateRenderShadowMapCommandsPass* m_pCreateRenderShadowMapCommandsPass;
	BindingResourceList* m_pCreateRenderShadowMapCommandsResources;
	Buffer* m_pCreateRenderShadowMapCommandsArgumentBuffer;
	
	RenderTiledShadowMapPass* m_pRenderSpotLightTiledShadowMapPass;
	BindingResourceList* m_pRenderSpotLightTiledShadowMapResources;

	CopyTexturePass* m_pCopyTexturePass;
	BindingResourceList* m_CopyTextureResources[kNumBackBuffers];

	MeshBatch* m_pMeshBatch;
	
	LightBuffer* m_pPointLightBuffer;
	Buffer* m_pNumVisiblePointLightsBuffer;
	Buffer* m_pVisiblePointLightIndexBuffer;

	LightBuffer* m_pSpotLightBuffer;
	Buffer* m_pNumVisibleSpotLightsBuffer;
	Buffer* m_pVisibleSpotLightIndexBuffer;
	
	Camera* m_pCamera;

#ifdef FOR_DEBUG_ONLY
	Buffer* m_pDebugShadowCastingSpotLightIndexBuffer;
	Buffer* m_pDebugNumShadowCastingSpotLightsBuffer;
	Buffer* m_pDebugDrawSpotLightShadowCasterCommandBuffer;
	Buffer* m_pDebugNumDrawSpotLightShadowCastersBuffer;
#endif // FOR_DEBUG_ONLY
};
