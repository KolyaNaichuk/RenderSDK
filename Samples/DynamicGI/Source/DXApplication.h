#pragma once

#include "Common/Application.h"

struct HeapProperties;
struct RenderEnv;
struct Viewport;

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
class GeometryBuffer;
class MeshRenderResources;
class LightRenderResources;
class MaterialRenderResources;
class DownscaleAndReprojectDepthPass;
class FrustumMeshCullingPass;
class FillVisibilityBufferPass;
class CreateMainDrawCommandsPass;
class CreateFalseNegativeDrawCommandsPass;
class FillMeshTypeDepthBufferPass;
class RenderGBufferPass;
class TiledLightCullingPass;
class TiledShadingPass;
class CalcShadingRectanglesPass;
class FrustumLightCullingPass;
class Scene;

// Old
class ClearVoxelGridPass;
class CreateVoxelGridPass;
class InjectVirtualPointLightsPass;
class PropagateLightPass;
class VisualizeVoxelGridPass;
class VisualizeTexturePass;
class VisualizeVoxelGridPass;
class VisualizeIntensityPass;
class CreateRenderShadowMapCommandsPass;
class RenderTiledShadowMapPass;
class SetupTiledShadowMapPass;

//#define DEBUG_RENDER_PASS

class DXApplication : public Application
{
public:
	enum class DisplayResult
	{
		ShadingResult,
		DepthBuffer,
		ReprojectedDepthBuffer,
		NormalBuffer,
		TexCoordBuffer,
		DepthBufferWithMeshType,
		IndirectLightIntensityResult,
		SpotLightTiledShadowMap,
		PointLightTiledShadowMap,
		VoxelGridDiffuse,
		VoxelGridNormal,
		Unspecified
	};
	enum class TileShadingMode
	{
		DirectLight,
		IndirectLight,
		DirectAndIndirectLight
	};
	enum IndirectLightIntensity
	{
		IndirectLightIntensity_Previous = 0,
		IndirectLightIntensity_Current,
		IndirectLightIntensity_Accumulated
	};
	enum IndirectLightComponent
	{
		IndirectLightComponent_Red = 0,
		IndirectLightComponent_Green,
		IndirectLightComponent_Blue
	};

	DXApplication(HINSTANCE hApp);
	~DXApplication();

private:
	void OnInit() override;
	void OnUpdate() override;
	void OnRender() override;
	void OnDestroy() override;
	void OnKeyDown(UINT8 key) override;
	void OnKeyUp(UINT8 key) override;

	void InitRenderEnv(UINT backBufferWidth, UINT backBufferHeight);
	void InitScene(Scene* pScene, UINT backBufferWidth, UINT backBufferHeight);

	void InitDownscaleAndReprojectDepthPass();
	CommandList* RecordDownscaleAndReprojectDepthPass();

	CommandList* RecordClearResourcesPass();

	void InitFrustumMeshCullingPass();
	CommandList* RecordFrustumMeshCullingPass();
	
	void InitFillVisibilityBufferMainPass();
	CommandList* RecordFillVisibilityBufferMainPass();

	void InitCreateMainDrawCommandsPass();
	CommandList* RecordCreateMainDrawCommandsPass();

	void InitRenderGBufferMainPass(UINT bufferWidth, UINT bufferHeight);
	CommandList* RecordRenderGBufferMainPass();

	void InitFillVisibilityBufferFalseNegativePass();
	CommandList* RecordFillVisibilityBufferFalseNegativePass();

	void InitCreateFalseNegativeDrawCommandsPass();
	CommandList* RecordCreateFalseNegativeDrawCommandsPass();

	void InitRenderGBufferFalseNegativePass(UINT bufferWidth, UINT bufferHeight);
	CommandList* RecordRenderGBufferFalseNegativePass();
	
	void InitCalcShadingRectanglesPass();
	CommandList* RecordCalcShadingRectanglesPass();

	void InitFillMeshTypeDepthBufferPass();
	CommandList* RecordFillMeshTypeDepthBufferPass();
		
	void InitFrustumPointLightCullingPass();
	CommandList* RecordFrustumPointLightCullingPass();

	void InitFrustumSpotLightCullingPass();
	CommandList* RecordFrustumSpotLightCullingPass();

	void InitTiledLightCullingPass();
	CommandList* RecordTiledLightCullingPass();
		
	void InitTiledShadingPass();
	CommandList* RecordTiledShadingPass();

	void InitVisualizeDepthBufferPass();
	CommandList* RecordVisualizeDepthBufferPass();

	void InitVisualizeReprojectedDepthBufferPass();
	CommandList* RecordVisualizeReprojectedDepthBufferPass();

	void InitVisualizeNormalBufferPass();
	CommandList* RecordVisualizeNormalBufferPass();

	void InitVisualizeTexCoordBufferPass();
	CommandList* RecordVisualizeTexCoordBufferPass();

	void InitVisualizeDepthBufferWithMeshTypePass();
	CommandList* RecordVisualizeDepthBufferWithMeshTypePass();

	void InitVisualizeAccumLightPass();
	CommandList* RecordVisualizeAccumLightPass();

	CommandList* RecordPostRenderPass();
	
	// Old
	void InitClearVoxelGridPass();
	void InitCreateVoxelGridPass();
	void InitInjectVirtualPointLightsPass();
	void InitPropagateLightPass();
	void InitVisualizeVoxelGridDiffusePass();
	void InitVisualizeVoxelGridNormalPass();
	void InitVisualizeIntensityPass();

	void InitSetupSpotLightTiledShadowMapPass();
	void InitSetupPointLightTiledShadowMapPass();
	void InitCreateRenderShadowMapCommandsPass();
	void InitRenderSpotLightTiledShadowMapPass();
	void InitRenderPointLightTiledShadowMapPass();
	void InitVisualizeSpotLightTiledShadowMapPass();
	void InitVisualizePointLightTiledShadowMapPass();

	CommandList* RecordUpdateCreateRenderShadowMapCommandsArgumentBufferPass();
	CommandList* RecordCreateRenderShadowMapCommandsPass();
	CommandList* RecordSetupSpotLightTiledShadowMapPass();
	CommandList* RecordSetupPointLightTiledShadowMapPass();
	CommandList* RecordRenderSpotLightTiledShadowMapPass();
	CommandList* RecordRenderPointLightTiledShadowMapPass();
	CommandList* RecordClearVoxelGridPass();
	CommandList* RecordCreateVoxelGridPass();
	CommandList* RecordInjectVirtualPointLightsPass();
	CommandList* RecordPropagateLightPass();
	CommandList* RecordVisualizeVoxelGridDiffusePass();
	CommandList* RecordVisualizeVoxelGridNormalPass();
	CommandList* RecordVisualizeSpotLightTiledShadowMapPass();
	CommandList* RecordVisualizePointLightTiledShadowMapPass();
	CommandList* RecordVisualizeIntensityPass();
	CommandList* RecordDisplayResultPass();
	
	void UpdateDisplayResult(DisplayResult displayResult);
		
#ifdef DEBUG_RENDER_PASS
	void OuputDebugRenderPassResult();
#endif // DEBUG_RENDER_PASS
	
private:
	enum { kNumBackBuffers = 3 };
	
	DisplayResult m_DisplayResult;
	TileShadingMode m_ShadingMode;
	IndirectLightIntensity m_IndirectLightIntensity;
	IndirectLightComponent m_IndirectLightComponent;
	UINT m_NumPropagationIterations;

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
	DescriptorHeap* m_pShaderInvisibleSamplerHeap;
	DescriptorHeap* m_pShaderVisibleSRVHeap;
	DescriptorHeap* m_pShaderVisibleSamplerHeap;
	DepthTexture* m_pDepthTexture;
	ColorTexture* m_pAccumLightTexture;

	DepthTexture* m_pSpotLightTiledShadowMap;
	DepthTexture* m_pPointLightTiledShadowMap;		
	ColorTexture* m_IntensityRCoeffsTextures[2];
	ColorTexture* m_IntensityGCoeffsTextures[2];
	ColorTexture* m_IntensityBCoeffsTextures[2];
	ColorTexture* m_pAccumIntensityRCoeffsTexture;
	ColorTexture* m_pAccumIntensityGCoeffsTexture;
	ColorTexture* m_pAccumIntensityBCoeffsTexture;
	Viewport* m_pBackBufferViewport;
	Viewport* m_pSpotLightTiledShadowMapViewport;
	Viewport* m_pPointLightTiledShadowMapViewport;
	Buffer* m_pGridBuffer;
	Buffer* m_pGridConfigDataBuffer;
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
	UINT64 m_FrameCompletionFenceValues[kNumBackBuffers];
	UINT m_BackBufferIndex;

	ClearVoxelGridPass* m_pClearVoxelGridPass;
	CreateVoxelGridPass* m_pCreateVoxelGridPass;
	InjectVirtualPointLightsPass* m_pInjectVirtualPointLightsPass;
	PropagateLightPass* m_pPropagateLightPass;
	VisualizeVoxelGridPass* m_pVisualizeVoxelGridDiffusePass;
	VisualizeVoxelGridPass* m_pVisualizeVoxelGridNormalPass;
	CreateRenderShadowMapCommandsPass* m_pCreateRenderShadowMapCommandsPass;
	Buffer* m_pCreateRenderShadowMapCommandsArgumentBuffer;
	RenderTiledShadowMapPass* m_pRenderSpotLightTiledShadowMapPass;
	RenderTiledShadowMapPass* m_pRenderPointLightTiledShadowMapPass;
	SetupTiledShadowMapPass* m_pSetupSpotLightTiledShadowMapPass;
	SetupTiledShadowMapPass* m_pSetupPointLightTiledShadowMapPass;
	VisualizeTexturePass* m_pVisualizeSpotLightTiledShadowMapPass;
	VisualizeTexturePass* m_pVisualizePointLightTiledShadowMapPass;
	VisualizeIntensityPass* m_pVisualizeIntensityPass;
		
	// New render passes
	Camera* m_pCamera;
	MeshRenderResources* m_pMeshRenderResources;
	LightRenderResources* m_pPointLightRenderResources;
	LightRenderResources* m_pSpotLightRenderResources;
	MaterialRenderResources* m_pMaterialRenderResources;
	GeometryBuffer* m_pGeometryBuffer;
	DownscaleAndReprojectDepthPass* m_pDownscaleAndReprojectDepthPass;
	FrustumMeshCullingPass* m_pFrustumMeshCullingPass;
	FillVisibilityBufferPass* m_pFillVisibilityBufferMainPass;
	CreateMainDrawCommandsPass* m_pCreateMainDrawCommandsPass;
	RenderGBufferPass* m_pRenderGBufferMainPass;
	FillVisibilityBufferPass* m_pFillVisibilityBufferFalseNegativePass;
	CreateFalseNegativeDrawCommandsPass* m_pCreateFalseNegativeDrawCommandsPass;
	RenderGBufferPass* m_pRenderGBufferFalseNegativePass;
	FillMeshTypeDepthBufferPass* m_pFillMeshTypeDepthBufferPass;
	CalcShadingRectanglesPass* m_pCalcShadingRectanglesPass;
	FrustumLightCullingPass* m_pFrustumPointLightCullingPass;
	FrustumLightCullingPass* m_pFrustumSpotLightCullingPass;
	TiledLightCullingPass* m_pTiledLightCullingPass;
	TiledShadingPass* m_pTiledShadingPass;
	VisualizeTexturePass* m_pVisualizeAccumLightPasses[kNumBackBuffers];
	VisualizeTexturePass* m_VisualizeDepthBufferPasses[kNumBackBuffers];
	VisualizeTexturePass* m_VisualizeReprojectedDepthBufferPasses[kNumBackBuffers];
	VisualizeTexturePass* m_VisualizeNormalBufferPasses[kNumBackBuffers];
	VisualizeTexturePass* m_VisualizeTexCoordBufferPasses[kNumBackBuffers];
	VisualizeTexturePass* m_VisualizeDepthBufferWithMeshTypePasses[kNumBackBuffers];
	Buffer* m_pAppDataBuffers[kNumBackBuffers];
	void* m_pAppDataPointers[kNumBackBuffers];
};
