#pragma once

#include "Common/Application.h"
#include "Common/Light.h"
#include "Math/Vector2.h"
#include "Math/Vector3.h"
#include "Math/Sphere.h"
#include "Math/Plane.h"
#include "RenderPasses/RenderTiledShadowMapPass.h"

struct HeapProperties;
struct RenderEnv;
struct Viewport;
struct Frustum;

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
class CreateShadowMapCommandsPass;
class ShadowMapTileAllocator;
class FillMeshTypeDepthBufferPass;
class RenderGBufferPass;
class TiledLightCullingPass;
class TiledShadingPass;
class CalcShadingRectanglesPass;
class CreateVoxelizeCommandsPass;
class VisualizeTexturePass;
class VisualizeVoxelReflectancePass;
class VoxelizePass;
class Scene;
class PointLight;
class SpotLight;
class CPUProfiler;
class GPUProfiler;

struct LightFrustum
{
	Plane m_LeftPlane;
	Plane m_RightPlane;
	Plane m_TopPlane;
	Plane m_BottomPlane;
};

struct PointLightData
{
	Vector3f m_Color;
	Sphere m_WorldBounds;
	f32 m_ShadowNearPlane;
	u32 m_AffectedScreenArea;
	Matrix4f m_ViewProjMatrices[kNumCubeMapFaces];
	ShadowMapTile m_ShadowMapTiles[kNumCubeMapFaces];
	LightFrustum m_WorldFrustums[kNumCubeMapFaces];
};

struct SpotLightData
{
	Vector3f m_Color;
	Vector3f m_WorldSpaceDir;
	Sphere m_WorldBounds;
	f32 m_ShadowNearPlane;
	f32 m_LightRange;
	f32 m_CosHalfInnerConeAngle;
	f32 m_CosHalfOuterConeAngle;
	u32 m_AffectedScreenArea;
	Matrix4f m_ViewProjMatrix;
	ShadowMapTile m_ShadowMapTile;
	LightFrustum m_WorldFrustum;
};

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
		PointLightTiledShadowMap,
		SpotLightTiledShadowMap,
		VoxelRelectance,
		Unknown
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
	void InitScene(UINT backBufferWidth, UINT backBufferHeight);

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
	
	void InitCalcShadingRectanglesPass();
	CommandList* RecordCalcShadingRectanglesPass();

	void InitFillMeshTypeDepthBufferPass();
	CommandList* RecordFillMeshTypeDepthBufferPass();

	CommandList* RecordUploadLightDataPass();

	void InitTiledLightCullingPass();
	CommandList* RecordTiledLightCullingPass();

	void InitCreateShadowMapCommandsPass();
	CommandList* RecordCreateShadowMapCommandsPass();

	void InitRenderPointLightTiledShadowMapPass();
	CommandList* RecordRenderPointLightTiledShadowMapPass();

	void InitRenderSpotLightTiledShadowMapPass();
	CommandList* RecordRenderSpotLightTiledShadowMapPass();

	void InitCreateVoxelizeCommandsPass();
	CommandList* RecordCreateVoxelizeCommandsPass();

	void InitVoxelizePass();
	CommandList* RecordVoxelizePass();
		
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

	void InitVisualizePointLightTiledShadowMapPass();
	CommandList* RecordVisualizePointLightTiledShadowMapPass();

	void InitVisualizeSpotLightTiledShadowMapPass();
	CommandList* RecordVisualizeSpotLightTiledShadowMapPass();

	void InitVisualizeVoxelReflectancePass();
	CommandList* RecordVisualizeVoxelReflectancePass();

	CommandList* RecordVisualizeDisplayResultPass();
	CommandList* RecordPostRenderPass();

	void InitPointLightRenderResources(Scene* pScene);
	void InitSpotLightRenderResources(Scene* pScene);

	void SetupPointLightDataForUpload(const Frustum& cameraWorldFrustum);
	void SetupSpotLightDataForUpload(const Frustum& cameraWorldFrustum);
			
	void UpdateDisplayResult(DisplayResult displayResult);
		
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
	CalcShadingRectanglesPass* m_pCalcShadingRectanglesPass = nullptr;
	CreateShadowMapCommandsPass* m_pCreateShadowMapCommandsPass = nullptr;
	RenderTiledShadowMapPass* m_pRenderPointLightTiledShadowMapPass = nullptr;
	ShadowMapTileAllocator* m_pPointLightShadowMapTileAllocator = nullptr;
	RenderTiledShadowMapPass* m_pRenderSpotLightTiledShadowMapPass = nullptr;
	ShadowMapTileAllocator* m_pSpotLightShadowMapTileAllocator = nullptr;
	CreateVoxelizeCommandsPass* m_pCreateVoxelizeCommandsPass = nullptr;
	VoxelizePass* m_pVoxelizePass = nullptr;
	TiledLightCullingPass* m_pTiledLightCullingPass = nullptr;
	TiledShadingPass* m_pTiledShadingPass = nullptr;
	VisualizeTexturePass* m_pVisualizeAccumLightPasses[kNumBackBuffers] = {nullptr, nullptr, nullptr};
	VisualizeTexturePass* m_VisualizeDepthBufferPasses[kNumBackBuffers] = {nullptr, nullptr, nullptr};
	VisualizeTexturePass* m_VisualizeReprojectedDepthBufferPasses[kNumBackBuffers] = {nullptr, nullptr, nullptr};
	VisualizeTexturePass* m_VisualizeNormalBufferPasses[kNumBackBuffers] = {nullptr, nullptr, nullptr};
	VisualizeTexturePass* m_VisualizeTexCoordBufferPasses[kNumBackBuffers] = {nullptr, nullptr, nullptr};
	VisualizeTexturePass* m_VisualizeDepthBufferWithMeshTypePasses[kNumBackBuffers] = {nullptr, nullptr, nullptr};
	VisualizeTexturePass* m_VisualizePointLightTiledShadowMapPasses[kNumBackBuffers] = {nullptr, nullptr, nullptr};
	VisualizeTexturePass* m_VisualizeSpotLightTiledShadowMapPasses[kNumBackBuffers] = {nullptr, nullptr, nullptr};
	VisualizeVoxelReflectancePass* m_VisualizeVoxelReflectancePasses[kNumBackBuffers] = {nullptr, nullptr, nullptr};

	u32 m_NumPointLights = 0;
	PointLightData* m_pPointLights = nullptr;
	
	u32 m_NumActivePointLights = 0;
	PointLightData** m_ppActivePointLights = nullptr;

	u32 m_NumSpotLights = 0;
	SpotLightData* m_pSpotLights = nullptr;

	u32 m_NumActiveSpotLights = 0;
	SpotLightData** m_ppActiveSpotLights = nullptr;

	Buffer* m_pActivePointLightWorldBoundsBuffer = nullptr;
	Buffer* m_pActivePointLightPropsBuffer = nullptr;
	Buffer* m_pActivePointLightWorldFrustumBuffer = nullptr;
	Buffer* m_pActivePointLightViewProjMatrixBuffer = nullptr;

	Buffer* m_pActiveSpotLightWorldBoundsBuffer = nullptr;
	Buffer* m_pActiveSpotLightPropsBuffer = nullptr;
	Buffer* m_pActiveSpotLightWorldFrustumBuffer = nullptr;
	Buffer* m_pActiveSpotLightViewProjMatrixBuffer = nullptr;

	Buffer* m_pUploadAppDataBuffers[kNumBackBuffers] = {nullptr, nullptr, nullptr};
	void* m_pUploadAppData[kNumBackBuffers] = {nullptr, nullptr, nullptr};

	Buffer* m_pUploadActivePointLightWorldBoundsBuffers[kNumBackBuffers] = {nullptr, nullptr, nullptr};
	void* m_pUploadActivePointLightWorldBounds[kNumBackBuffers] = {nullptr, nullptr, nullptr};
	Buffer* m_pUploadActivePointLightPropsBuffers[kNumBackBuffers] = {nullptr, nullptr, nullptr};
	void* m_pUploadActivePointLightProps[kNumBackBuffers] = {nullptr, nullptr, nullptr};
	Buffer* m_pUploadActivePointLightWorldFrustumBuffers[kNumBackBuffers] = {nullptr, nullptr, nullptr};
	void* m_pUploadActivePointLightWorldFrustums[kNumBackBuffers] = {nullptr, nullptr, nullptr};
	Buffer* m_pUploadActivePointLightViewProjMatrixBuffers[kNumBackBuffers] = {nullptr, nullptr, nullptr};
	void* m_pUploadActivePointLightViewProjMatrices[kNumBackBuffers] = {nullptr, nullptr, nullptr};

	Buffer* m_pUploadActiveSpotLightWorldBoundsBuffers[kNumBackBuffers] = {nullptr, nullptr, nullptr};
	void* m_pUploadActiveSpotLightWorldBounds[kNumBackBuffers] = {nullptr, nullptr, nullptr};
	Buffer* m_pUploadActiveSpotLightPropsBuffers[kNumBackBuffers] = {nullptr, nullptr, nullptr};
	void* m_pUploadActiveSpotLightProps[kNumBackBuffers] = {nullptr, nullptr, nullptr};
	Buffer* m_pUploadActiveSpotLightWorldFrustumBuffers[kNumBackBuffers] = {nullptr, nullptr, nullptr};
	void* m_pUploadActiveSpotLightWorldFrustums[kNumBackBuffers] = {nullptr, nullptr, nullptr};
	Buffer* m_pUploadActiveSpotLightViewProjMatrixBuffers[kNumBackBuffers] = {nullptr, nullptr, nullptr};
	void* m_pUploadActiveSpotLightViewProjMatrices[kNumBackBuffers] = {nullptr, nullptr, nullptr};
};