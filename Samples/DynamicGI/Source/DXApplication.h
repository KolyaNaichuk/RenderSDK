#pragma once

#include "Common/Application.h"

struct Frustum;
struct HeapProperties;
struct RenderEnv;
struct SpotLightRenderData;
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
class MaterialRenderResources;
class DownscaleAndReprojectDepthPass;
class FrustumMeshCullingPass;
class FillVisibilityBufferPass;
class CreateMainDrawCommandsPass;
class CreateFalseNegativeDrawCommandsPass;
class CubeMapToSHCoefficientsPass;
class FillMeshTypeDepthBufferPass;
class RenderGBufferPass;
class SpotLightShadowMapRenderer;
class TiledLightCullingPass;
class TiledShadingPass;
class CreateVoxelizeCommandsPass;
class VisualizeNumLightsPerTilePass;
class VisualizeTexturePass;
class VisualizeDepthTexturePass;
class VisualizeVoxelReflectancePass;
class VoxelizePass;
class Scene;
class CPUProfiler;
class GPUProfiler;

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
		NumLightsPerTile,
		Unknown
	};
	
	DXApplication(HINSTANCE hApp);
	~DXApplication();

private:
	void OnInit() override;
	void OnUpdate() override;
	void OnRender() override;
	void OnDestroy() override;
	
	void InitRenderEnvironment(UINT backBufferWidth, UINT backBufferHeight);
	void InitScene(UINT backBufferWidth, UINT backBufferHeight, Scene* pScene);
	
	void InitDownscaleAndReprojectDepthPass();
	CommandList* RecordDownscaleAndReprojectDepthPass();

	CommandList* RecordPreRenderPass();

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

	void InitFillMeshTypeDepthBufferPass();
	CommandList* RecordFillMeshTypeDepthBufferPass();

	CommandList* RecordUploadLightDataPass();

	void InitRenderSpotLightShadowMaps(Scene* pScene);
	CommandList* RecordRenderSpotLightShadowMaps();

	void InitTiledLightCullingPass();
	CommandList* RecordTiledLightCullingPass();
	
	void InitCreateVoxelizeCommandsPass();
	CommandList* RecordCreateVoxelizeCommandsPass();

	void InitVoxelizePass();
	CommandList* RecordVoxelizePass();
		
	void InitTiledShadingPass();
	CommandList* RecordTiledShadingPass();

	void InitCubeMapToSHCoefficientsPass();
	CommandList* RecordCubeMapToSHCoefficientsPass();

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
	
	void InitVisualizeNumLightsPerTilePass();
	CommandList* RecordVisualizeNumLightsPerTilePass();

	void InitVisualizeVoxelReflectancePass();
	CommandList* RecordVisualizeVoxelReflectancePass();

	CommandList* RecordVisualizeDisplayResultPass();
	CommandList* RecordPostRenderPass();

	void InitSpotLightRenderResources(Scene* pScene);
	void SetupSpotLightDataForUpload(const Frustum& cameraWorldFrustum);
	
	void UpdateDisplayResult(DisplayResult displayResult);
	void HandleUserInput();

#ifdef DEBUG_RENDER_PASS
	void OuputDebugRenderPassResult();
#endif // DEBUG_RENDER_PASS
	
private:
	enum { kNumBackBuffers = 3 };
		
	DisplayResult m_DisplayResult = DisplayResult::Unknown;
	
	GraphicsDevice* m_pDevice = nullptr;
	SwapChain* m_pSwapChain = nullptr;
	CommandQueue* m_pCommandQueue = nullptr;
	CommandListPool* m_pCommandListPool = nullptr;
	HeapProperties* m_pUploadHeapProps = nullptr;
	HeapProperties* m_pDefaultHeapProps = nullptr;
	HeapProperties* m_pReadbackHeapProps = nullptr;
	DescriptorHeap* m_pShaderInvisibleRTVHeap = nullptr;
	DescriptorHeap* m_pShaderInvisibleDSVHeap = nullptr;
	DescriptorHeap* m_pShaderInvisibleSRVHeap = nullptr;
	DescriptorHeap* m_pShaderInvisibleSamplerHeap = nullptr;
	DescriptorHeap* m_pShaderVisibleSRVHeap = nullptr;
	DescriptorHeap* m_pShaderVisibleSamplerHeap = nullptr;
	CPUProfiler* m_pCPUProfiler = nullptr;
	GPUProfiler* m_pGPUProfiler = nullptr;
	DepthTexture* m_pDepthTexture = nullptr;
	ColorTexture* m_pAccumLightTexture = nullptr;
	Viewport* m_pBackBufferViewport = nullptr;
	RenderEnv* m_pRenderEnv = nullptr;
	Fence* m_pFence = nullptr;
	UINT64 m_FrameCompletionFenceValues[kNumBackBuffers] = {0, 0, 0};
	UINT m_BackBufferIndex = 0;

	Camera* m_pCamera = nullptr;
	MeshRenderResources* m_pMeshRenderResources = nullptr;
	MaterialRenderResources* m_pMaterialRenderResources = nullptr;
	GeometryBuffer* m_pGeometryBuffer = nullptr;
	DownscaleAndReprojectDepthPass* m_pDownscaleAndReprojectDepthPass = nullptr;
	FrustumMeshCullingPass* m_pFrustumMeshCullingPass = nullptr;
	FillVisibilityBufferPass* m_pFillVisibilityBufferMainPass = nullptr;
	CreateMainDrawCommandsPass* m_pCreateMainDrawCommandsPass = nullptr;
	RenderGBufferPass* m_pRenderGBufferMainPass = nullptr;
	FillVisibilityBufferPass* m_pFillVisibilityBufferFalseNegativePass = nullptr;
	CreateFalseNegativeDrawCommandsPass* m_pCreateFalseNegativeDrawCommandsPass = nullptr;
	RenderGBufferPass* m_pRenderGBufferFalseNegativePass = nullptr;
	FillMeshTypeDepthBufferPass* m_pFillMeshTypeDepthBufferPass = nullptr;
	SpotLightShadowMapRenderer* m_pSpotLightShadowMapRenderer = nullptr;
	CreateVoxelizeCommandsPass* m_pCreateVoxelizeCommandsPass = nullptr;
	VoxelizePass* m_pVoxelizePass = nullptr;
	TiledLightCullingPass* m_pTiledLightCullingPass = nullptr;
	TiledShadingPass* m_pTiledShadingPass = nullptr;
	
	VisualizeTexturePass* m_VisualizeAccumLightPasses[kNumBackBuffers] = {nullptr, nullptr, nullptr};
	VisualizeDepthTexturePass* m_VisualizeDepthBufferPasses[kNumBackBuffers] = {nullptr, nullptr, nullptr};
	VisualizeDepthTexturePass* m_VisualizeReprojectedDepthBufferPasses[kNumBackBuffers] = {nullptr, nullptr, nullptr};
	VisualizeTexturePass* m_VisualizeNormalBufferPasses[kNumBackBuffers] = {nullptr, nullptr, nullptr};
	VisualizeTexturePass* m_VisualizeTexCoordBufferPasses[kNumBackBuffers] = {nullptr, nullptr, nullptr};
	VisualizeTexturePass* m_VisualizeDepthBufferWithMeshTypePasses[kNumBackBuffers] = {nullptr, nullptr, nullptr};
	VisualizeNumLightsPerTilePass* m_VisualizeNumLightsPerTilePasses[kNumBackBuffers] = {nullptr, nullptr, nullptr};
	VisualizeVoxelReflectancePass* m_VisualizeVoxelReflectancePasses[kNumBackBuffers] = {nullptr, nullptr, nullptr};
	
	ColorTexture* m_pCubeMap = nullptr;
	Buffer* m_pSHCoefficientBuffer = nullptr;
	CubeMapToSHCoefficientsPass* m_pCubeMapToSHCoefficientsPass = nullptr;

	u32 m_NumSpotLights = 0;
	SpotLightRenderData* m_pSpotLights = nullptr;

	u32 m_NumActiveSpotLights = 0;
	SpotLightRenderData** m_ppActiveSpotLights = nullptr;
	u32* m_pActiveSpotLightIndices = nullptr;
		
	Buffer* m_pActiveSpotLightWorldBoundsBuffer = nullptr;
	Buffer* m_pActiveSpotLightPropsBuffer = nullptr;
	
	Buffer* m_UploadAppDataBuffers[kNumBackBuffers] = {nullptr, nullptr, nullptr};
	void* m_UploadAppData[kNumBackBuffers] = {nullptr, nullptr, nullptr};
		
	Buffer* m_UploadActiveSpotLightWorldBoundsBuffers[kNumBackBuffers] = {nullptr, nullptr, nullptr};
	void* m_UploadActiveSpotLightWorldBounds[kNumBackBuffers] = {nullptr, nullptr, nullptr};
	Buffer* m_UploadActiveSpotLightPropsBuffers[kNumBackBuffers] = {nullptr, nullptr, nullptr};
	void* m_UploadActiveSpotLightProps[kNumBackBuffers] = {nullptr, nullptr, nullptr};
};
