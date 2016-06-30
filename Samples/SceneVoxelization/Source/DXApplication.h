#pragma once

#include "Common/Application.h"

class DXDevice;
class DXSwapChain;
class DXCommandQueue;
class DXCommandAllocator;
class DXCommandList;
class DXCommandListPool;
class DXDescriptorHeap;
class DXColorTexture;
class DXDepthTexture;
class DXBuffer;
class DXFence;

class Camera;
class MeshBatch;
class LightBuffer;
class FillGBufferRecorder;
class ClearVoxelGridRecorder;
class CreateVoxelGridRecorder;
class InjectVPLsIntoVoxelGridRecorder;
class VisualizeVoxelGridRecorder;
class VisualizeMeshRecorder;
class CopyTextureRecorder;
class TiledShadingRecorder;
class ViewFrustumCullingRecorder;
class FillGBufferCommandsRecorder;
class RenderShadowMapCommandsRecorder;

struct DXHeapProperties;
struct DXRenderEnvironment;
struct DXBindingResourceList;
struct DXViewport;

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
	DXCommandListPool* m_pCommandListPool;
	DXHeapProperties* m_pDefaultHeapProps;
	DXHeapProperties* m_pUploadHeapProps;
	DXDescriptorHeap* m_pShaderInvisibleRTVHeap;
	DXDescriptorHeap* m_pShaderInvisibleDSVHeap;
	DXDescriptorHeap* m_pShaderInvisibleSRVHeap;
	DXDescriptorHeap* m_pShaderVisibleSRVHeap;
	DXDepthTexture* m_pDepthTexture;
	DXColorTexture* m_pDiffuseTexture;
	DXColorTexture* m_pNormalTexture;
	DXColorTexture* m_pSpecularTexture;
	DXColorTexture* m_pAccumLightTexture;
	DXViewport* m_pViewport;
	DXBuffer* m_pObjectTransformBuffer;
	DXBuffer* m_pCameraTransformBuffer;
	DXBuffer* m_pGridBuffer;
	DXBuffer* m_pGridConfigBuffer;
	DXBuffer* m_pCullingDataBuffer;
	DXBuffer* m_pShadingDataBuffer;
	DXBuffer* m_pDrawMeshCommandBuffer;
	DXBuffer* m_pNumVisibleMeshesBuffer;
	DXBuffer* m_pVisibleMeshIndexBuffer;
	DXBuffer* m_pShadowCastingPointLightIndexBuffer;
	DXBuffer* m_pNumShadowCastingPointLightsBuffer;
	DXBuffer* m_pDrawPointLightShadowCasterCommandBuffer;
	DXBuffer* m_pNumDrawPointLightShadowCastersBuffer;
	DXBuffer* m_pShadowCastingSpotLightIndexBuffer;
	DXBuffer* m_pNumShadowCastingSpotLightsBuffer;
	DXBuffer* m_pDrawSpotLightShadowCasterCommandBuffer;
	DXBuffer* m_pNumDrawSpotLightShadowCastersBuffer;
	DXRenderEnvironment* m_pRenderEnv;
	DXFence* m_pFence;
	UINT64 m_FenceValues[kBackBufferCount];
	UINT m_BackBufferIndex;

	FillGBufferRecorder* m_pFillGBufferRecorder;
	DXBindingResourceList* m_pFillGBufferResources;

	TiledShadingRecorder* m_pTiledShadingRecorder;
	DXBindingResourceList* m_pTiledShadingResources;

	ClearVoxelGridRecorder* m_pClearVoxelGridRecorder;
	DXBindingResourceList* m_pClearVoxelGridResources;

	CreateVoxelGridRecorder* m_pCreateVoxelGridRecorder;
	DXBindingResourceList* m_pCreateVoxelGridResources;

	InjectVPLsIntoVoxelGridRecorder* m_pInjectVPLsIntoVoxelGridRecorder;

	VisualizeVoxelGridRecorder* m_pVisualizeVoxelGridRecorder;
	DXBindingResourceList* m_VisualizeVoxelGridResources[kBackBufferCount];

	VisualizeMeshRecorder* m_pVisualizeMeshRecorder;
	DXBindingResourceList* m_VisualizeMeshResources[kBackBufferCount];

	ViewFrustumCullingRecorder* m_pDetectVisibleMeshesRecorder;
	DXBindingResourceList* m_pDetectVisibleMeshesResources;

	ViewFrustumCullingRecorder* m_pDetectVisiblePointLightsRecorder;
	DXBindingResourceList* m_pDetectVisiblePointLightsResources;

	ViewFrustumCullingRecorder* m_pDetectVisibleSpotLightsRecorder;
	DXBindingResourceList* m_pDetectVisibleSpotLightsResources;

	FillGBufferCommandsRecorder* m_pFillGBufferCommandsRecorder;
	DXBindingResourceList* m_pFillGBufferCommandsResources;

	RenderShadowMapCommandsRecorder* m_pRenderShadowMapCommandsRecorder;
	DXBindingResourceList* m_pRenderShadowMapCommandsResources;
	DXBuffer* m_pRenderShadowMapCommandsArgumentBuffer;

	CopyTextureRecorder* m_pCopyTextureRecorder;
	DXBindingResourceList* m_CopyTextureResources[kBackBufferCount];

	MeshBatch* m_pMeshBatch;
	
	LightBuffer* m_pPointLightBuffer;
	DXBuffer* m_pNumVisiblePointLightsBuffer;
	DXBuffer* m_pVisiblePointLightIndexBuffer;

	LightBuffer* m_pSpotLightBuffer;
	DXBuffer* m_pNumVisibleSpotLightsBuffer;
	DXBuffer* m_pVisibleSpotLightIndexBuffer;
	
	Camera* m_pCamera;
};
