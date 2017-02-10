#pragma once

#include "Common/Application.h"
#include "RenderPasses/VisualizeVoxelGridPass.h"

class GraphicsDevice;
class SwapChain;
class CommandQueue;
class CommandList;
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
class InjectVirtualPointLightsPass;
class PropagateLightPass;
class CalcIndirectLightPass;
class VisualizeVoxelGridPass;
class VisualizeTexturePass;
class TiledLightCullingPass;
class TiledShadingPass;
class ViewFrustumCullingPass;
class CreateRenderShadowMapCommandsPass;
class RenderTiledShadowMapPass;
class SetupTiledShadowMapPass;
class Scene;

struct HeapProperties;
struct RenderEnv;
struct BindingResourceList;
struct Viewport;

//#define DEBUG_RENDER_PASS

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

	void InitRenderEnv(UINT backBufferWidth, UINT backBufferHeight);
	void InitScene(Scene* pScene, UINT backBufferWidth, UINT backBufferHeight);
	void InitDetectVisibleMeshesPass();
	void InitDetectVisiblePointLightsPass();
	void InitDetectVisibleSpotLightsPass();
	void InitCreateRenderGBufferCommandsPass();
	void InitRenderGBufferPass(UINT backBufferWidth, UINT backBufferHeight);
	void InitTiledLightCullingPass();
	void InitTiledShadingPass();
	void InitSetupSpotLightTiledShadowMapPass();
	void InitSetupPointLightTiledShadowMapPass();
	void InitCreateRenderShadowMapCommandsPass();
	void InitRenderSpotLightTiledShadowMapPass();
	void InitRenderPointLightTiledShadowMapPass();
	void InitVisualizeTexturePass();
	void InitClearVoxelGridPass();
	void InitCreateVoxelGridPass();
	void InitInjectVirtualPointLightsPass();
	void InitPropagateLightPass();
	void InitCalcIndirectLightPass(UINT backBufferWidth, UINT backBufferHeight);
	void InitVisualizeVoxelGridPass(VisualizeVoxelGridPass::VoxelDataType voxelDataType);
	void InitConstantBuffers(const Scene* pScene, UINT backBufferWidth, UINT backBufferHeight);

	CommandList* RecordClearBackBufferPass(u8 clearFlags);
	CommandList* RecordDetectVisibleMeshesPass();
	CommandList* RecordDetectVisiblePointLightsPass();
	CommandList* RecordDetectVisibleSpotLightsPass();
	CommandList* RecordCreateRenderGBufferCommandsPass();
	CommandList* RecordRenderGBufferPass();
	CommandList* RecordTiledLightCullingPass();
	CommandList* RecordUpdateCreateRenderShadowMapCommandsArgumentBufferPass();
	CommandList* RecordCreateRenderShadowMapCommandsPass();
	CommandList* RecordSetupSpotLightTiledShadowMapPass();
	CommandList* RecordSetupPointLightTiledShadowMapPass();
	CommandList* RecordRenderSpotLightTiledShadowMapPass();
	CommandList* RecordRenderPointLightTiledShadowMapPass();
	CommandList* RecordTiledShadingPass();
	CommandList* RecordClearVoxelGridPass();
	CommandList* RecordCreateVoxelGridPass();
	CommandList* RecordInjectVirtualPointLightsPass();
	CommandList* RecordPropagateLightPass();
	CommandList* RecordCalcIndirectLightPass();
	CommandList* RecordVisualizeVoxelGridPass();
	CommandList* RecordVisualizeTexturePass();
	CommandList* RecordPresentResourceBarrierPass();
	
#ifdef DEBUG_RENDER_PASS
	void InitDebugRenderPass(const Scene* pScene);
	CommandList* RecordDebugRenderPass();
	void OuputDebugRenderPassResult();
#endif // DEBUG_RENDER_PASS
	
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
	DepthTexture* m_pPointLightTiledShadowMap;
	ColorTexture* m_pDiffuseTexture;
	ColorTexture* m_pNormalTexture;
	ColorTexture* m_pSpecularTexture;
	ColorTexture* m_pAccumLightTexture;
	ColorTexture* m_pIndirectLightTexture;
	ColorTexture* m_FluxRCoeffsTextures[2];
	ColorTexture* m_FluxGCoeffsTextures[2];
	ColorTexture* m_FluxBCoeffsTextures[2];
	Viewport* m_pBackBufferViewport;
	Viewport* m_pSpotLightTiledShadowMapViewport;
	Viewport* m_pPointLightTiledShadowMapViewport;
	Buffer* m_pObjectTransformBuffer;
	Buffer* m_pCameraTransformBuffer;
	Buffer* m_pGridBuffer;
	Buffer* m_pGridConfigBuffer;
	Buffer* m_pViewFrustumMeshCullingDataBuffer;
	Buffer* m_pViewFrustumSpotLightCullingDataBuffer;
	Buffer* m_pViewFrustumPointLightCullingDataBuffer;
	Buffer* m_pTiledLightCullingDataBuffer;
	Buffer* m_pTiledShadingDataBuffer;
	Buffer* m_pVisualizeTextureDataBuffer;
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
	Buffer* m_pSpotLightShadowMapDataBuffer;
	Buffer* m_pSpotLightShadowMapTileBuffer;
	Buffer* m_pSpotLightViewTileProjMatrixBuffer;
	Buffer* m_pPointLightShadowMapDataBuffer;
	Buffer* m_pPointLightShadowMapTileBuffer;
	Buffer* m_pPointLightViewTileProjMatrixBuffer;
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

	InjectVirtualPointLightsPass* m_pInjectVirtualPointLightsPass;
	BindingResourceList* m_pInjectVirtualPointLightsResources;

	PropagateLightPass* m_pPropagateLightPass;
	BindingResourceList* m_PropagateLightResources[2];

	CalcIndirectLightPass* m_pCalcIndirectLightPass;
	BindingResourceList* m_pCalcIndirectLightResources;
	
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
	
	RenderTiledShadowMapPass* m_pRenderPointLightTiledShadowMapPass;
	BindingResourceList* m_pRenderPointLightTiledShadowMapResources;
	
	SetupTiledShadowMapPass* m_pSetupSpotLightTiledShadowMapPass;
	BindingResourceList* m_pSetupSpotLightTiledShadowMapResources;

	SetupTiledShadowMapPass* m_pSetupPointLightTiledShadowMapPass;
	BindingResourceList* m_pSetupPointLightTiledShadowMapResources;

	VisualizeTexturePass* m_pVisualizeTexturePass;
	BindingResourceList* m_VisualizeTextureResources[kNumBackBuffers];

	MeshBatch* m_pMeshBatch;
	
	LightBuffer* m_pPointLightBuffer;
	Buffer* m_pNumVisiblePointLightsBuffer;
	Buffer* m_pVisiblePointLightIndexBuffer;

	LightBuffer* m_pSpotLightBuffer;
	Buffer* m_pNumVisibleSpotLightsBuffer;
	Buffer* m_pVisibleSpotLightIndexBuffer;
	
	Camera* m_pCamera;

#ifdef DEBUG_RENDER_PASS
	BindingResourceList* m_pDebugResources;
	Buffer* m_pDebugPointLightRangePerTileBuffer;
	Buffer* m_pDebugNumVisibleMeshesBuffer;
	Buffer* m_pDebugVisibleMeshIndexBuffer;
	Buffer* m_pDebugShadowCastingSpotLightIndexBuffer;
	Buffer* m_pDebugNumShadowCastingSpotLightsBuffer;
	Buffer* m_pDebugDrawSpotLightShadowCasterCommandBuffer;
	Buffer* m_pDebugNumDrawSpotLightShadowCastersBuffer;
	Buffer* m_pDebugSpotLightShadowMapTileBuffer;
	Buffer* m_pDebugSpotLightViewTileProjMatrixBuffer;
	Buffer* m_pDebugPointLightShadowMapTileBuffer;
#endif // DEBUG_RENDER_PASS
};
