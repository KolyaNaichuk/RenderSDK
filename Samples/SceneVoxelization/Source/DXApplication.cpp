#include "DXApplication.h"
#include "D3DWrapper/BindingResourceList.h"
#include "D3DWrapper/GraphicsDevice.h"
#include "D3DWrapper/GraphicsFactory.h"
#include "D3DWrapper/GraphicsResource.h"
#include "D3DWrapper/GraphicsUtils.h"
#include "D3DWrapper/CommandList.h"
#include "D3DWrapper/CommandQueue.h"
#include "D3DWrapper/CommandSignature.h"
#include "D3DWrapper/DescriptorHeap.h"
#include "D3DWrapper/Fence.h"
#include "D3DWrapper/RenderEnv.h"
#include "D3DWrapper/SwapChain.h"
#include "RenderPasses/ClearVoxelGridPass.h"
#include "RenderPasses/CreateVoxelGridPass.h"
#include "RenderPasses/CreateRenderGBufferCommandsPass.h"
#include "RenderPasses/CreateRenderShadowMapCommandsPass.h"
#include "RenderPasses/InjectVPLsIntoVoxelGridPass.h"
#include "RenderPasses/RenderGBufferPass.h"
#include "RenderPasses/RenderTiledShadowMapPass.h"
#include "RenderPasses/SetupTiledShadowMapPass.h"
#include "RenderPasses/TiledLightCullingPass.h"
#include "RenderPasses/TiledShadingPass.h"
#include "RenderPasses/ViewFrustumCullingPass.h"
#include "RenderPasses/VisualizeVoxelGridPass.h"
#include "RenderPasses/VisualizeTexturePass.h"
#include "Common/MeshData.h"
#include "Common/MeshBatchData.h"
#include "Common/MeshBatch.h"
#include "Common/LightBuffer.h"
#include "Common/Color.h"
#include "Common/Camera.h"
#include "Common/Scene.h"
#include "Common/SceneLoader.h"
#include "Math/Vector3.h"
#include "Math/Vector4.h"
#include "Math/Matrix4.h"
#include "Math/Transform.h"
#include "Math/BasisAxes.h"
#include "Math/Radian.h"

/*
Render passes overview

1.Run view frustum culling of meshes. Generate visible mesh indices (GPU side)
2.Run view furstum culling of point lights. Generate visible point light indices (GPU side)
3.Run view frustum culling of spot lights. Generate visible spot light indices (GPU side)
4.Generate render G-Buffer indirect draw commands (GPU side)
5.Render G-Buffer with visible meshes (GPU side)



4.Run view frustum culling of light bounding spheres (CPU side)
5.Assign shadow map tile for each visible light (CPU side)
5.1.Detect screen space area of visible lights (CPU side)
5.2.Sort light list based on affecting screen space area (CPU side)
5.3.Assign shadow map tile using tile quad tree (CPU side)
6.Copy visible lights to GPU-visible light buffer (CPU side)

6.1.Assign visible lights per each screen space tile (GPU side, compute pipeline)
6.2.Render shadow maps for visible lights (GPU side, graphics pipeline)
7.Run tile deferred shading using assigned lights for each tile and rendered shadow maps (GPU side)

To do:
1.Check that inside CreateRenderShadowMapCommands.hlsl we are checking the bound
against MAX_NUM_SPOT_LIGHTS_PER_SHADOW_CASTER and MAX_NUM_POINT_LIGHTS_PER_SHADOW_CASTER
while writing data to the local storage
*/

enum OutputResult
{
	OutputResult_GBufferDiffuse,
	OutputResult_GBufferSpecular,
	OutputResult_GBufferNormal,
	OutputResult_Depth,
	OutputResult_SpotLightTiledShadowMap,
	OutputResult_AccumLight
};

enum
{
	kOutputResult = OutputResult_GBufferNormal,

	kTileSize = 16,
	kNumTilesX = 40,
	kNumTilesY = 40,

	kGridSizeX = 640,
	kGridSizeY = 640,
	kGridSizeZ = 640,
		
	kNumGridCellsX = 64,
	kNumGridCellsY = 64,
	kNumGridCellsZ = 64,

	kNumShadowMapTiles = 1,
	kShadowMapTileSize = 512,
	kTiledShadowMapSize = kNumShadowMapTiles * kShadowMapTileSize
};

struct ObjectTransform
{
	Matrix4f m_WorldPositionMatrix;
	Matrix4f m_WorldNormalMatrix;
	Matrix4f m_WorldViewProjMatrix;
	Vector4f m_NotUsed[4];
};

struct CameraTransform
{
	Matrix4f m_ViewProjInvMatrix;
	Matrix4f m_ViewProjMatrices[3];
};

struct GridConfig
{
	Vector4f m_WorldSpaceOrigin;
	Vector4f m_RcpCellSize;
	Vector4i m_NumCells;
	Vector4f m_NotUsed[13];
};

struct Range
{
	u32 m_Start;
	u32 m_Length;
};

struct ViewFrustumCullingData
{
	Plane m_ViewFrustumPlanes[6];
	u32 m_NumObjects;
	Vector3u m_NotUsed1;
	Vector4f m_NotUsed2[9];
};

struct TiledLightCullingData
{
	Vector2f m_RcpScreenSize;
	Vector2f m_NotUsed1;
	Vector4f m_NotUsed2;
	Vector4f m_NotUsed3;
	Vector4f m_NotUsed4;
	Matrix4f m_ViewMatrix;
	Matrix4f m_ProjMatrix;
	Matrix4f m_ProjInvMatrix;
};

struct TiledShadingData
{
	Vector2f m_RcpScreenSize;
	Vector2f m_NotUsed1;
	Vector3f m_WorldSpaceLightDir;
	f32 m_NotUsed2;
	Vector3f m_LightColor;
	f32 m_NotUsed3;
	Vector3f m_WorldSpaceCameraPos;
	f32 m_NotUsed4;
	Matrix4f m_ViewProjInvMatrix;
	Matrix4f m_NotUsed5[2];
};

struct Voxel
{
	Vector4f m_ColorAndNumOccluders;
};

struct ShadowMapData
{
	f32 m_SizeInPixels;
	f32 m_RcpSizeInPixels;
	f32 m_TileSizeInPixels;
	f32 m_RcpTileSizeInPixels;
	f32 m_NotUsed[60];
};

struct ShadowMapTile
{
	Vector2f m_TexSpaceTopLeftPos;
	Vector2f m_TexSpaceSize;
};

struct VisualizeGBufferData
{
	Matrix4f m_CameraProjMatrix;
	f32 m_CameraNearPlane;
	f32 m_CameraFarPlane;
	f32 m_NotUsed[46];
};

DXApplication::DXApplication(HINSTANCE hApp)
	: Application(hApp, L"Scene Voxelization", 0, 0, kTileSize * kNumTilesX, kTileSize * kNumTilesY)
	, m_pDevice(nullptr)
	, m_pSwapChain(nullptr)
	, m_pCommandQueue(nullptr)
	, m_pCommandListPool(nullptr)
	, m_pUploadHeapProps(new HeapProperties(D3D12_HEAP_TYPE_UPLOAD))
	, m_pDefaultHeapProps(new HeapProperties(D3D12_HEAP_TYPE_DEFAULT))
	, m_pReadbackHeapProps(new HeapProperties(D3D12_HEAP_TYPE_READBACK))
	, m_pShaderInvisibleRTVHeap(nullptr)
	, m_pShaderInvisibleDSVHeap(nullptr)
	, m_pShaderInvisibleSRVHeap(nullptr)
	, m_pShaderVisibleSRVHeap(nullptr)
	, m_pDepthTexture(nullptr)
	, m_pSpotLightTiledShadowMap(nullptr)
	, m_pDiffuseTexture(nullptr)
	, m_pNormalTexture(nullptr)
	, m_pSpecularTexture(nullptr)
	, m_pAccumLightTexture(nullptr)
	, m_pBackBufferViewport(nullptr)
	, m_pSpotLightTiledShadowMapViewport(nullptr)
	, m_pObjectTransformBuffer(nullptr)
	, m_pCameraTransformBuffer(nullptr)
	, m_pGridBuffer(nullptr)
	, m_pGridConfigBuffer(nullptr)
	, m_pViewFrustumMeshCullingDataBuffer(nullptr)
	, m_pViewFrustumSpotLightCullingDataBuffer(nullptr)
	, m_pViewFrustumPointLightCullingDataBuffer(nullptr)
	, m_pTiledLightCullingDataBuffer(nullptr)
	, m_pTiledShadingDataBuffer(nullptr)
	, m_pVisualizeGBufferDataBuffer(nullptr)
	, m_pDrawMeshCommandBuffer(nullptr)
	, m_pNumVisibleMeshesBuffer(nullptr)
	, m_pVisibleMeshIndexBuffer(nullptr)
	, m_pNumPointLightsPerTileBuffer(nullptr)
	, m_pPointLightIndexPerTileBuffer(nullptr)
	, m_pPointLightRangePerTileBuffer(nullptr)
	, m_pNumSpotLightsPerTileBuffer(nullptr)
	, m_pSpotLightIndexPerTileBuffer(nullptr)
	, m_pSpotLightRangePerTileBuffer(nullptr)
	, m_pShadowCastingPointLightIndexBuffer(nullptr)
	, m_pNumShadowCastingPointLightsBuffer(nullptr)
	, m_pDrawPointLightShadowCasterCommandBuffer(nullptr)
	, m_pNumDrawPointLightShadowCastersBuffer(nullptr)
	, m_pShadowCastingSpotLightIndexBuffer(nullptr)
	, m_pNumShadowCastingSpotLightsBuffer(nullptr)
	, m_pDrawSpotLightShadowCasterCommandBuffer(nullptr)
	, m_pNumDrawSpotLightShadowCastersBuffer(nullptr)
	, m_pSpotLightShadowMapDataBuffer(nullptr)
	, m_pSpotLightShadowMapTileBuffer(nullptr)
	, m_pSpotLightViewTileProjMatrixBuffer(nullptr)
	, m_pRenderEnv(new RenderEnv())
	, m_pFence(nullptr)
	, m_LastSubmissionFenceValue(0)
	, m_BackBufferIndex(0)
	, m_pRenderGBufferPass(nullptr)
	, m_pRenderGBufferResources(new BindingResourceList())
	, m_pTiledLightCullingPass(nullptr)
	, m_pTiledLightCullingResources(new BindingResourceList())
	, m_pTiledShadingPass(nullptr)
	, m_pTiledShadingResources(new BindingResourceList())
	, m_pClearVoxelGridPass(nullptr)
	, m_pClearVoxelGridResources(new BindingResourceList())
	, m_pCreateVoxelGridPass(nullptr)
	, m_pCreateVoxelGridResources(new BindingResourceList())
	, m_pInjectVPLsIntoVoxelGridPass(nullptr)
	, m_pVisualizeVoxelGridPass(nullptr)
	, m_pDetectVisibleMeshesPass(nullptr)
	, m_pDetectVisibleMeshesResources(new BindingResourceList())
	, m_pDetectVisiblePointLightsPass(nullptr)
	, m_pDetectVisiblePointLightsResources(new BindingResourceList())
	, m_pDetectVisibleSpotLightsPass(nullptr)
	, m_pDetectVisibleSpotLightsResources(new BindingResourceList())
	, m_pCreateRenderGBufferCommandsPass(nullptr)
	, m_pCreateRenderGBufferCommandsResources(new BindingResourceList())
	, m_pCreateRenderShadowMapCommandsArgumentBufferResources(new BindingResourceList())
	, m_pCreateRenderShadowMapCommandsPass(nullptr)
	, m_pCreateRenderShadowMapCommandsResources(new BindingResourceList())
	, m_pCreateRenderShadowMapCommandsArgumentBuffer(nullptr)
	, m_pRenderSpotLightTiledShadowMapPass(nullptr)
	, m_pRenderSpotLightTiledShadowMapResources(new BindingResourceList())
	, m_pSetupSpotLightTiledShadowMapPass(nullptr)
	, m_pSetupSpotLightTiledShadowMapResources(new BindingResourceList())
	, m_pVisualizeTexturePass(nullptr)
	, m_pMeshBatch(nullptr)
	, m_pPointLightBuffer(nullptr)
	, m_pNumVisiblePointLightsBuffer(nullptr)
	, m_pVisiblePointLightIndexBuffer(nullptr)
	, m_pSpotLightBuffer(nullptr)
	, m_pNumVisibleSpotLightsBuffer(nullptr)
	, m_pVisibleSpotLightIndexBuffer(nullptr)
	, m_pCamera(nullptr)
#ifdef FOR_DEBUG_ONLY
	, m_pDebugResources(new BindingResourceList())
	, m_pDebugNumVisibleMeshesBuffer(nullptr)
	, m_pDebugVisibleMeshIndexBuffer(nullptr)
	, m_pDebugShadowCastingSpotLightIndexBuffer(nullptr)
	, m_pDebugNumShadowCastingSpotLightsBuffer(nullptr)
	, m_pDebugDrawSpotLightShadowCasterCommandBuffer(nullptr)
	, m_pDebugNumDrawSpotLightShadowCastersBuffer(nullptr)
	, m_pDebugSpotLightShadowMapTileBuffer(nullptr)
	, m_pDebugSpotLightViewTileProjMatrixBuffer(nullptr)
#endif // FOR_DEBUG_ONLY
{
	for (u8 index = 0; index < kNumBackBuffers; ++index)
	{
		m_FrameCompletionFenceValues[index] = m_LastSubmissionFenceValue;
		m_VisualizeVoxelGridResources[index] = new BindingResourceList();
		m_VisualizeTextureResources[index] = new BindingResourceList();
	}
}

DXApplication::~DXApplication()
{
	for (u8 index = 0; index < kNumBackBuffers; ++index)
	{
		SafeDelete(m_VisualizeVoxelGridResources[index]);
		SafeDelete(m_VisualizeTextureResources[index]);
	}
	SafeDelete(m_pVisualizeTexturePass);
	SafeDelete(m_pSpotLightShadowMapDataBuffer);
	SafeDelete(m_pSpotLightShadowMapTileBuffer);
	SafeDelete(m_pSpotLightViewTileProjMatrixBuffer);
	SafeDelete(m_pSetupSpotLightTiledShadowMapPass);
	SafeDelete(m_pSetupSpotLightTiledShadowMapResources);
	SafeDelete(m_pCreateRenderShadowMapCommandsArgumentBufferResources);
	SafeDelete(m_pCommandListPool);
	SafeDelete(m_pCamera);
	SafeDelete(m_pPointLightBuffer);
	SafeDelete(m_pNumVisiblePointLightsBuffer);
	SafeDelete(m_pVisiblePointLightIndexBuffer);
	SafeDelete(m_pSpotLightBuffer);
	SafeDelete(m_pNumVisibleSpotLightsBuffer);
	SafeDelete(m_pVisibleSpotLightIndexBuffer);
	SafeDelete(m_pMeshBatch);
	SafeDelete(m_pShadowCastingPointLightIndexBuffer);
	SafeDelete(m_pNumShadowCastingPointLightsBuffer);
	SafeDelete(m_pDrawPointLightShadowCasterCommandBuffer);
	SafeDelete(m_pNumDrawPointLightShadowCastersBuffer);
	SafeDelete(m_pShadowCastingSpotLightIndexBuffer);
	SafeDelete(m_pNumShadowCastingSpotLightsBuffer);
	SafeDelete(m_pDrawSpotLightShadowCasterCommandBuffer);
	SafeDelete(m_pNumDrawSpotLightShadowCastersBuffer);
	SafeDelete(m_pClearVoxelGridPass);
	SafeDelete(m_pClearVoxelGridResources);
	SafeDelete(m_pCreateVoxelGridPass);
	SafeDelete(m_pCreateVoxelGridResources);
	SafeDelete(m_pInjectVPLsIntoVoxelGridPass);
	SafeDelete(m_pVisualizeVoxelGridPass);
	SafeDelete(m_pTiledLightCullingPass);
	SafeDelete(m_pTiledLightCullingResources);
	SafeDelete(m_pTiledShadingPass);
	SafeDelete(m_pTiledShadingResources);
	SafeDelete(m_pRenderGBufferPass);
	SafeDelete(m_pRenderGBufferResources);
	SafeDelete(m_pCreateRenderGBufferCommandsPass);
	SafeDelete(m_pCreateRenderGBufferCommandsResources);
	SafeDelete(m_pCreateRenderShadowMapCommandsPass);
	SafeDelete(m_pCreateRenderShadowMapCommandsResources);
	SafeDelete(m_pCreateRenderShadowMapCommandsArgumentBuffer);
	SafeDelete(m_pRenderSpotLightTiledShadowMapPass);
	SafeDelete(m_pRenderSpotLightTiledShadowMapResources);
	SafeDelete(m_pDetectVisibleMeshesPass);
	SafeDelete(m_pDetectVisibleMeshesResources);
	SafeDelete(m_pDetectVisiblePointLightsPass);
	SafeDelete(m_pDetectVisiblePointLightsResources);
	SafeDelete(m_pDetectVisibleSpotLightsPass);
	SafeDelete(m_pDetectVisibleSpotLightsResources);
	SafeDelete(m_pFence);
	SafeDelete(m_pUploadHeapProps);
	SafeDelete(m_pDefaultHeapProps);
	SafeDelete(m_pReadbackHeapProps);
	SafeDelete(m_pShaderInvisibleDSVHeap);
	SafeDelete(m_pShaderInvisibleSRVHeap);
	SafeDelete(m_pShaderInvisibleRTVHeap);
	SafeDelete(m_pShaderVisibleSRVHeap);
	SafeDelete(m_pRenderEnv);
	SafeDelete(m_pNumVisibleMeshesBuffer);
	SafeDelete(m_pVisibleMeshIndexBuffer);
	SafeDelete(m_pNumPointLightsPerTileBuffer);
	SafeDelete(m_pPointLightIndexPerTileBuffer);
	SafeDelete(m_pPointLightRangePerTileBuffer);
	SafeDelete(m_pNumSpotLightsPerTileBuffer);
	SafeDelete(m_pSpotLightIndexPerTileBuffer);
	SafeDelete(m_pSpotLightRangePerTileBuffer);
	SafeDelete(m_pDrawMeshCommandBuffer);
	SafeDelete(m_pViewFrustumMeshCullingDataBuffer);
	SafeDelete(m_pViewFrustumSpotLightCullingDataBuffer);
	SafeDelete(m_pViewFrustumPointLightCullingDataBuffer);
	SafeDelete(m_pTiledLightCullingDataBuffer);
	SafeDelete(m_pTiledShadingDataBuffer);
	SafeDelete(m_pGridConfigBuffer);
	SafeDelete(m_pGridBuffer);
	SafeDelete(m_pCameraTransformBuffer);
	SafeDelete(m_pObjectTransformBuffer);
	SafeDelete(m_pDiffuseTexture);
	SafeDelete(m_pNormalTexture);
	SafeDelete(m_pSpecularTexture);
	SafeDelete(m_pAccumLightTexture);
	SafeDelete(m_pSpotLightTiledShadowMap);
	SafeDelete(m_pDepthTexture);
	SafeDelete(m_pCommandQueue);
	SafeDelete(m_pCommandListPool);
	SafeDelete(m_pSwapChain);
	SafeDelete(m_pDevice);
	SafeDelete(m_pBackBufferViewport);
	SafeDelete(m_pSpotLightTiledShadowMapViewport);
	SafeDelete(m_pVisualizeGBufferDataBuffer);

#ifdef FOR_DEBUG_ONLY
	SafeDelete(m_pDebugResources);
	SafeDelete(m_pDebugNumVisibleMeshesBuffer);
	SafeDelete(m_pDebugVisibleMeshIndexBuffer);
	SafeDelete(m_pDebugShadowCastingSpotLightIndexBuffer);
	SafeDelete(m_pDebugNumShadowCastingSpotLightsBuffer);
	SafeDelete(m_pDebugDrawSpotLightShadowCasterCommandBuffer);
	SafeDelete(m_pDebugNumDrawSpotLightShadowCastersBuffer);
	SafeDelete(m_pDebugSpotLightShadowMapTileBuffer);
	SafeDelete(m_pDebugSpotLightViewTileProjMatrixBuffer);
#endif // FOR_DEBUG_ONLY
}

void DXApplication::OnInit()
{
	Scene* pScene = SceneLoader::LoadCornellBox(CornellBoxSettings_Test1);

	GraphicsFactory factory;
	m_pDevice = new GraphicsDevice(&factory, D3D_FEATURE_LEVEL_11_0);

#ifdef ENABLE_INDIRECT_LIGHTING
	D3D12_FEATURE_DATA_D3D12_OPTIONS supportedOptions;
	m_pDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &supportedOptions, sizeof(supportedOptions));
	assert(supportedOptions.ConservativeRasterizationTier != D3D12_CONSERVATIVE_RASTERIZATION_TIER_NOT_SUPPORTED);
	assert(supportedOptions.ROVsSupported == TRUE);
#endif // ENABLE_INDIRECT_LIGHTING

	CommandQueueDesc commandQueueDesc(D3D12_COMMAND_LIST_TYPE_DIRECT);
	m_pCommandQueue = new CommandQueue(m_pDevice, &commandQueueDesc, L"m_pCommandQueue");

	m_pCommandListPool = new CommandListPool(m_pDevice, D3D12_COMMAND_LIST_TYPE_DIRECT);

	DescriptorHeapDesc rtvHeapDesc(D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 9, false);
	m_pShaderInvisibleRTVHeap = new DescriptorHeap(m_pDevice, &rtvHeapDesc, L"m_pShaderInvisibleRTVHeap");

	DescriptorHeapDesc shaderVisibleSRVHeapDesc(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 80, true);
	m_pShaderVisibleSRVHeap = new DescriptorHeap(m_pDevice, &shaderVisibleSRVHeapDesc, L"m_pShaderVisibleSRVHeap");

	DescriptorHeapDesc shaderInvisibleSRVHeapDesc(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 70, false);
	m_pShaderInvisibleSRVHeap = new DescriptorHeap(m_pDevice, &shaderInvisibleSRVHeapDesc, L"m_pShaderInvisibleSRVHeap");

	DescriptorHeapDesc dsvHeapDesc(D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 2, false);
	m_pShaderInvisibleDSVHeap = new DescriptorHeap(m_pDevice, &dsvHeapDesc, L"m_pShaderInvisibleDSVHeap");
				
	m_pRenderEnv->m_pDevice = m_pDevice;
	m_pRenderEnv->m_pCommandListPool = m_pCommandListPool;
	m_pRenderEnv->m_pUploadHeapProps = m_pUploadHeapProps;
	m_pRenderEnv->m_pDefaultHeapProps = m_pDefaultHeapProps;
	m_pRenderEnv->m_pReadbackHeapProps = m_pReadbackHeapProps;
	m_pRenderEnv->m_pShaderInvisibleRTVHeap = m_pShaderInvisibleRTVHeap;
	m_pRenderEnv->m_pShaderInvisibleSRVHeap = m_pShaderInvisibleSRVHeap;
	m_pRenderEnv->m_pShaderInvisibleDSVHeap = m_pShaderInvisibleDSVHeap;
	m_pRenderEnv->m_pShaderVisibleSRVHeap = m_pShaderVisibleSRVHeap;
	
	const RECT backBufferRect = m_pWindow->GetClientRect();
	const UINT backBufferWidth = backBufferRect.right - backBufferRect.left;
	const UINT backBufferHeight = backBufferRect.bottom - backBufferRect.top;

	m_pBackBufferViewport = new Viewport(0.0f, 0.0f, (FLOAT)backBufferWidth, (FLOAT)backBufferHeight);
	m_pSpotLightTiledShadowMapViewport = new Viewport(0.0f, 0.0f, (FLOAT)kTiledShadowMapSize, (FLOAT)kTiledShadowMapSize);

	SpotLight* pSpotLight = *pScene->GetSpotLights();
	m_pCamera = new Camera(Camera::ProjType_Perspective, 0.001f, pSpotLight->GetAttenEndRange(), 1.0f);
	m_pCamera->GetTransform().SetPosition(pSpotLight->GetTransform().GetPosition());
	m_pCamera->GetTransform().SetRotation(pSpotLight->GetTransform().GetRotation());
	m_pCamera->SetFovY(pSpotLight->GetOuterConeAngle());

	//m_pCamera = new Camera(Camera::ProjType_Perspective, 0.1f, 1300.0f, FLOAT(backBufferWidth) / FLOAT(backBufferHeight));
	//m_pCamera->GetTransform().SetPosition(Vector3f(278.0f, 274.0f, 700.0f));
	//m_pCamera->GetTransform().SetRotation(CreateRotationYQuaternion(Radian(PI)));
	
	m_pCamera->SetClearFlags(Camera::ClearFlag_Color | Camera::ClearFlag_Depth);
	m_pCamera->SetBackgroundColor(Color::GRAY);
	
	SwapChainDesc swapChainDesc(kNumBackBuffers, m_pWindow->GetHWND(), backBufferWidth, backBufferHeight);
	m_pSwapChain = new SwapChain(&factory, m_pRenderEnv, &swapChainDesc, m_pCommandQueue);
	m_BackBufferIndex = m_pSwapChain->GetCurrentBackBufferIndex();
	
	DepthStencilValue optimizedClearDepth(1.0f);

	DepthTexture2DDesc depthTexDesc(DXGI_FORMAT_R32_TYPELESS, backBufferWidth, backBufferHeight, true, true);
	m_pDepthTexture = new DepthTexture(m_pRenderEnv, m_pRenderEnv->m_pDefaultHeapProps, &depthTexDesc, D3D12_RESOURCE_STATE_DEPTH_WRITE, &optimizedClearDepth, L"m_pDepthTexture");
	
	DepthTexture2DDesc spotLightTiledShadowMapDesc(DXGI_FORMAT_R32_TYPELESS, kTiledShadowMapSize, kTiledShadowMapSize, true, true);
	m_pSpotLightTiledShadowMap = new DepthTexture(m_pRenderEnv, m_pRenderEnv->m_pDefaultHeapProps, &spotLightTiledShadowMapDesc, D3D12_RESOURCE_STATE_DEPTH_WRITE, &optimizedClearDepth, L"m_pSpotLightTiledShadowMap");

	const FLOAT optimizedClearColor[4] = {0.0f, 0.0f, 0.0f, 0.0f};
	ColorTexture2DDesc diffuseTexDesc(DXGI_FORMAT_R10G10B10A2_UNORM, backBufferWidth, backBufferHeight, true, true, false);
	m_pDiffuseTexture = new ColorTexture(m_pRenderEnv, m_pRenderEnv->m_pDefaultHeapProps, &diffuseTexDesc, D3D12_RESOURCE_STATE_RENDER_TARGET, optimizedClearColor, L"m_pDiffuseTexture");

	ColorTexture2DDesc normalTexDesc(DXGI_FORMAT_R8G8B8A8_SNORM, backBufferWidth, backBufferHeight, true, true, false);
	m_pNormalTexture = new ColorTexture(m_pRenderEnv, m_pRenderEnv->m_pDefaultHeapProps, &normalTexDesc, D3D12_RESOURCE_STATE_RENDER_TARGET, optimizedClearColor, L"m_pNormalTexture");

	ColorTexture2DDesc specularTexDesc(DXGI_FORMAT_R8G8B8A8_UNORM, backBufferWidth, backBufferHeight, true, true, false);
	m_pSpecularTexture = new ColorTexture(m_pRenderEnv, m_pRenderEnv->m_pDefaultHeapProps, &specularTexDesc, D3D12_RESOURCE_STATE_RENDER_TARGET, optimizedClearColor, L"m_pSpecularTexture");

	ColorTexture2DDesc accumLightTexDesc(DXGI_FORMAT_R10G10B10A2_UNORM, backBufferWidth, backBufferHeight, false, true, true);
	m_pAccumLightTexture = new ColorTexture(m_pRenderEnv, m_pRenderEnv->m_pDefaultHeapProps, &accumLightTexDesc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, optimizedClearColor, L"m_pAccumLightTexture");

	ConstantBufferDesc objectTransformBufferDesc(sizeof(ObjectTransform));
	m_pObjectTransformBuffer = new Buffer(m_pRenderEnv, m_pRenderEnv->m_pUploadHeapProps, &objectTransformBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, L"m_pObjectTransformBuffer");

	ConstantBufferDesc cameraTransformBufferDesc(sizeof(CameraTransform));
	m_pCameraTransformBuffer = new Buffer(m_pRenderEnv, m_pRenderEnv->m_pUploadHeapProps, &cameraTransformBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, L"m_pCameraTransformBuffer");

	ConstantBufferDesc gridConfigBufferDesc(sizeof(GridConfig));
	m_pGridConfigBuffer = new Buffer(m_pRenderEnv, m_pRenderEnv->m_pUploadHeapProps, &gridConfigBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, L"m_pGridConfigBuffer");

	ConstantBufferDesc viewFrustumCullingDataBufferDesc(sizeof(ViewFrustumCullingData));
	m_pViewFrustumMeshCullingDataBuffer = new Buffer(m_pRenderEnv, m_pRenderEnv->m_pUploadHeapProps, &viewFrustumCullingDataBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, L"m_pViewFrustumMeshCullingDataBuffer");
	m_pViewFrustumPointLightCullingDataBuffer = new Buffer(m_pRenderEnv, m_pRenderEnv->m_pUploadHeapProps, &viewFrustumCullingDataBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, L"m_pViewFrustumPointLightCullingDataBuffer");
	m_pViewFrustumSpotLightCullingDataBuffer = new Buffer(m_pRenderEnv, m_pRenderEnv->m_pUploadHeapProps, &viewFrustumCullingDataBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, L"m_pViewFrustumSpotLightCullingDataBuffer");

	ConstantBufferDesc tiledLightCullingDataBufferDesc(sizeof(TiledLightCullingData));
	m_pTiledLightCullingDataBuffer = new Buffer(m_pRenderEnv, m_pRenderEnv->m_pUploadHeapProps, &tiledLightCullingDataBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, L"m_pTiledLightCullingDataBuffer");

	ConstantBufferDesc tiledShadingDataBufferDesc(sizeof(TiledShadingData));
	m_pTiledShadingDataBuffer = new Buffer(m_pRenderEnv, m_pRenderEnv->m_pUploadHeapProps, &tiledShadingDataBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, L"m_pTiledShadingDataBuffer");

	const u32 numGridElements = kNumGridCellsX * kNumGridCellsY * kNumGridCellsZ;
	StructuredBufferDesc gridBufferDesc(numGridElements, sizeof(Voxel), true, true);
	m_pGridBuffer = new Buffer(m_pRenderEnv, m_pRenderEnv->m_pDefaultHeapProps, &gridBufferDesc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, L"m_pGridBuffer");

	m_pFence = new Fence(m_pDevice, m_LastSubmissionFenceValue, L"m_pFence");
	
	assert(pScene->GetNumMeshBatches() == 1);
	MeshBatchData* pMeshBatchData = *pScene->GetMeshBatches();
	
	StructuredBufferDesc drawCommandBufferDesc(pMeshBatchData->GetNumMeshes(), sizeof(DrawMeshCommand), true, true);
	m_pDrawMeshCommandBuffer = new Buffer(m_pRenderEnv, m_pRenderEnv->m_pDefaultHeapProps, &drawCommandBufferDesc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, L"m_pDrawMeshCommandBuffer");
	
	CommandList* pCommandList = m_pCommandListPool->Create(L"uploadDataCommandList");
	pCommandList->Begin();

	m_pMeshBatch = new MeshBatch(m_pRenderEnv, pMeshBatchData);
	m_pMeshBatch->RecordDataForUpload(pCommandList);

	if (pScene->GetNumPointLights() > 0)
	{
		m_pPointLightBuffer = new LightBuffer(m_pRenderEnv, pScene->GetNumPointLights(), pScene->GetPointLights());
		m_pPointLightBuffer->RecordDataForUpload(pCommandList);

		FormattedBufferDesc numVisibleLightsBufferDesc(1, DXGI_FORMAT_R32_UINT, true, true);
		m_pNumVisiblePointLightsBuffer = new Buffer(m_pRenderEnv, m_pRenderEnv->m_pDefaultHeapProps, &numVisibleLightsBufferDesc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, L"m_pNumVisiblePointLightsBuffer");

		FormattedBufferDesc visibleLightIndexBufferDesc(pScene->GetNumPointLights(), DXGI_FORMAT_R32_UINT, true, true);
		m_pVisiblePointLightIndexBuffer = new Buffer(m_pRenderEnv, m_pRenderEnv->m_pDefaultHeapProps, &visibleLightIndexBufferDesc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, L"m_pVisiblePointLightIndexBuffer");

		FormattedBufferDesc numLightsPerTileBufferDesc(1, DXGI_FORMAT_R32_UINT, true, true);
		m_pNumPointLightsPerTileBuffer = new Buffer(m_pRenderEnv, m_pRenderEnv->m_pDefaultHeapProps, &numLightsPerTileBufferDesc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, L"m_pNumPointLightsPerTileBuffer");

		FormattedBufferDesc lightIndexPerTileBufferDesc(kNumTilesX * kNumTilesY * pScene->GetNumPointLights(), DXGI_FORMAT_R32_UINT, true, true);
		m_pPointLightIndexPerTileBuffer = new Buffer(m_pRenderEnv, m_pRenderEnv->m_pDefaultHeapProps, &lightIndexPerTileBufferDesc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, L"m_pPointLightIndexPerTileBuffer");

		StructuredBufferDesc lightRangePerTileBufferDesc(kNumTilesX * kNumTilesY, sizeof(Range), true, true);
		m_pPointLightRangePerTileBuffer = new Buffer(m_pRenderEnv, m_pRenderEnv->m_pDefaultHeapProps, &lightRangePerTileBufferDesc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, L"m_pPointLightRangePerTileBuffer");
	}

	if (pScene->GetNumSpotLights() > 0)
	{
		m_pSpotLightBuffer = new LightBuffer(m_pRenderEnv, pScene->GetNumSpotLights(), pScene->GetSpotLights());
		m_pSpotLightBuffer->RecordDataForUpload(pCommandList);

		FormattedBufferDesc numVisibleLightsBufferDesc(1, DXGI_FORMAT_R32_UINT, true, true);
		m_pNumVisibleSpotLightsBuffer = new Buffer(m_pRenderEnv, m_pRenderEnv->m_pDefaultHeapProps, &numVisibleLightsBufferDesc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, L"m_pNumVisibleSpotLightsBuffer");

		FormattedBufferDesc visibleLightIndexBufferDesc(pScene->GetNumSpotLights(), DXGI_FORMAT_R32_UINT, true, true);
		m_pVisibleSpotLightIndexBuffer = new Buffer(m_pRenderEnv, m_pRenderEnv->m_pDefaultHeapProps, &visibleLightIndexBufferDesc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, L"m_pVisibleSpotLightIndexBuffer");

		FormattedBufferDesc numLightsPerTileBufferDesc(1, DXGI_FORMAT_R32_UINT, true, true);
		m_pNumSpotLightsPerTileBuffer = new Buffer(m_pRenderEnv, m_pRenderEnv->m_pDefaultHeapProps, &numLightsPerTileBufferDesc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, L"m_pNumSpotLightsPerTileBuffer");

		FormattedBufferDesc lightIndexPerTileBufferDesc(kNumTilesX * kNumTilesY * pScene->GetNumSpotLights(), DXGI_FORMAT_R32_UINT, true, true);
		m_pSpotLightIndexPerTileBuffer = new Buffer(m_pRenderEnv, m_pRenderEnv->m_pDefaultHeapProps, &lightIndexPerTileBufferDesc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, L"m_pSpotLightIndexPerTileBuffer");

		StructuredBufferDesc lightRangePerTileBufferDesc(kNumTilesX * kNumTilesY, sizeof(Range), true, true);
		m_pSpotLightRangePerTileBuffer = new Buffer(m_pRenderEnv, m_pRenderEnv->m_pDefaultHeapProps, &lightRangePerTileBufferDesc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, L"m_pSpotLightRangePerTileBuffer");

		ConstantBufferDesc shadowMapDataBufferDesc(sizeof(ShadowMapData));
		m_pSpotLightShadowMapDataBuffer = new Buffer(m_pRenderEnv, m_pRenderEnv->m_pUploadHeapProps, &shadowMapDataBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, L"m_pSpotLightShadowMapDataBuffer");

		ConstantBufferDesc visualizeGBufferDataBufferDesc(sizeof(VisualizeGBufferData));
		m_pVisualizeGBufferDataBuffer = new Buffer(m_pRenderEnv, m_pRenderEnv->m_pUploadHeapProps, &visualizeGBufferDataBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, L"m_pVisualizeGBufferDataBuffer");

		StructuredBufferDesc shadowMapTileBufferDesc(kNumShadowMapTiles * kNumShadowMapTiles, sizeof(ShadowMapTile), true, true);
		m_pSpotLightShadowMapTileBuffer = new Buffer(m_pRenderEnv, m_pRenderEnv->m_pDefaultHeapProps, &shadowMapTileBufferDesc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, L"m_pSpotLightShadowMapTileBuffer");

		StructuredBufferDesc lightViewProjTileMatrixBufferDesc(pScene->GetNumSpotLights(), sizeof(Matrix4f), true, true);
		m_pSpotLightViewTileProjMatrixBuffer = new Buffer(m_pRenderEnv, m_pRenderEnv->m_pDefaultHeapProps, &lightViewProjTileMatrixBufferDesc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, L"m_pSpotLightShadowMapTileBuffer");

#ifdef FOR_DEBUG_ONLY
		StructuredBufferDesc debugShadowMapTileBufferDesc(kNumShadowMapTiles * kNumShadowMapTiles, sizeof(ShadowMapTile), false, false);
		m_pDebugSpotLightShadowMapTileBuffer = new Buffer(m_pRenderEnv, m_pRenderEnv->m_pReadbackHeapProps, &debugShadowMapTileBufferDesc, D3D12_RESOURCE_STATE_COPY_DEST, L"pDebugSpotLightShadowMapTileBuffer");
		
		StructuredBufferDesc debugLightViewProjTileMatrixBufferDesc(pScene->GetNumSpotLights(), sizeof(Matrix4f), false, false);
		m_pDebugSpotLightViewTileProjMatrixBuffer = new Buffer(m_pRenderEnv, m_pRenderEnv->m_pReadbackHeapProps, &debugLightViewProjTileMatrixBufferDesc, D3D12_RESOURCE_STATE_COPY_DEST, L"pDebugSpotLightViewTileProjMatrixBuffer");
#endif // FOR_DEBUG_ONLY
	}
	
	StructuredBufferDesc shadowMapCommandsArgumentBufferDesc(1, sizeof(Vector3u), false, false);
	m_pCreateRenderShadowMapCommandsArgumentBuffer = new Buffer(m_pRenderEnv, m_pRenderEnv->m_pDefaultHeapProps, &shadowMapCommandsArgumentBufferDesc, D3D12_RESOURCE_STATE_COPY_DEST, L"m_pCreateRenderShadowMapCommandsArgumentBuffer");

	Vector3u shadowMapCommandsArgumentBufferInitValues(0, 1, 1);
	Buffer uploadRenderShadowMapCommandsArgumentBuffer(m_pRenderEnv, m_pRenderEnv->m_pUploadHeapProps, &shadowMapCommandsArgumentBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, L"m_pCreateRenderShadowMapCommandsArgumentBuffer");
	uploadRenderShadowMapCommandsArgumentBuffer.Write(&shadowMapCommandsArgumentBufferInitValues, sizeof(Vector3u));

	pCommandList->CopyResource(m_pCreateRenderShadowMapCommandsArgumentBuffer, &uploadRenderShadowMapCommandsArgumentBuffer);
	pCommandList->End();

	++m_LastSubmissionFenceValue;
	m_pCommandQueue->ExecuteCommandLists(m_pRenderEnv, 1, &pCommandList, m_pFence, m_LastSubmissionFenceValue);
	m_pFence->WaitForSignalOnCPU(m_LastSubmissionFenceValue);

	m_pMeshBatch->RemoveDataForUpload();
	
	if (m_pPointLightBuffer != nullptr)
		m_pPointLightBuffer->RemoveDataForUpload();

	if (m_pSpotLightBuffer != nullptr)
		m_pSpotLightBuffer->RemoveDataForUpload();

	FormattedBufferDesc numVisibleMeshesBufferDesc(1, DXGI_FORMAT_R32_UINT, true, true);
	m_pNumVisibleMeshesBuffer = new Buffer(m_pRenderEnv, m_pRenderEnv->m_pDefaultHeapProps, &numVisibleMeshesBufferDesc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, L"m_pNumVisibleMeshesBuffer");

	FormattedBufferDesc visibleMeshIndexBufferDesc(m_pMeshBatch->GetNumMeshes(), DXGI_FORMAT_R32_UINT, true, true);
	m_pVisibleMeshIndexBuffer = new Buffer(m_pRenderEnv, m_pRenderEnv->m_pDefaultHeapProps, &visibleMeshIndexBufferDesc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, L"m_pVisibleMeshIndexBuffer");
	
#ifdef FOR_DEBUG_ONLY
	FormattedBufferDesc debugNumVisibleMeshesBufferDesc(1, DXGI_FORMAT_R32_UINT, false, false);
	m_pDebugNumVisibleMeshesBuffer = new Buffer(m_pRenderEnv, m_pRenderEnv->m_pReadbackHeapProps, &debugNumVisibleMeshesBufferDesc, D3D12_RESOURCE_STATE_COPY_DEST, L"m_pDebugNumVisibleMeshesBuffer");

	FormattedBufferDesc debugVisibleMeshIndexBufferDesc(m_pMeshBatch->GetNumMeshes(), DXGI_FORMAT_R32_UINT, false, false);
	m_pDebugVisibleMeshIndexBuffer = new Buffer(m_pRenderEnv, m_pRenderEnv->m_pReadbackHeapProps, &debugVisibleMeshIndexBufferDesc, D3D12_RESOURCE_STATE_COPY_DEST, L"m_pDebugVisibleMeshIndexBuffer");
#endif // FOR_DEBUG_ONLY

	Buffer* pMeshBoundsBuffer = m_pMeshBatch->GetMeshBoundsBuffer();
	Buffer* pMeshDescBuffer = m_pMeshBatch->GetMeshDescBuffer();
	Buffer* pMaterialBuffer = m_pMeshBatch->GetMaterialBuffer();

	ViewFrustumCullingPass::InitParams detectVisibleMeshesParams;
	detectVisibleMeshesParams.m_pRenderEnv = m_pRenderEnv;
	detectVisibleMeshesParams.m_ObjectBoundsType = ObjectBoundsType_AABB;
	detectVisibleMeshesParams.m_NumObjects = m_pMeshBatch->GetNumMeshes();
	
	m_pDetectVisibleMeshesPass = new ViewFrustumCullingPass(&detectVisibleMeshesParams);
	
	m_pDetectVisibleMeshesResources->m_RequiredResourceStates.emplace_back(m_pNumVisibleMeshesBuffer, m_pNumVisibleMeshesBuffer->GetWriteState());
	m_pDetectVisibleMeshesResources->m_RequiredResourceStates.emplace_back(m_pVisibleMeshIndexBuffer, m_pVisibleMeshIndexBuffer->GetWriteState());
	m_pDetectVisibleMeshesResources->m_RequiredResourceStates.emplace_back(pMeshBoundsBuffer, pMeshBoundsBuffer->GetReadState());
	
	m_pDetectVisibleMeshesResources->m_SRVHeapStart = m_pShaderVisibleSRVHeap->Allocate();
	m_pDevice->CopyDescriptor(m_pDetectVisibleMeshesResources->m_SRVHeapStart, m_pNumVisibleMeshesBuffer->GetUAVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), m_pVisibleMeshIndexBuffer->GetUAVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), pMeshBoundsBuffer->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), m_pViewFrustumMeshCullingDataBuffer->GetCBVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	if (m_pPointLightBuffer != nullptr)
	{
		ViewFrustumCullingPass::InitParams detectVisibleLightsParams;
		detectVisibleLightsParams.m_pRenderEnv = m_pRenderEnv;
		detectVisibleLightsParams.m_ObjectBoundsType = ObjectBoundsType_Sphere;
		detectVisibleLightsParams.m_NumObjects = m_pPointLightBuffer->GetNumLights();

		m_pDetectVisiblePointLightsPass = new ViewFrustumCullingPass(&detectVisibleLightsParams);
		Buffer* pLightBoundsBuffer = m_pPointLightBuffer->GetLightBoundsBuffer();
		
		m_pDetectVisiblePointLightsResources->m_RequiredResourceStates.emplace_back(m_pNumVisiblePointLightsBuffer, m_pNumVisiblePointLightsBuffer->GetWriteState());
		m_pDetectVisiblePointLightsResources->m_RequiredResourceStates.emplace_back(m_pVisiblePointLightIndexBuffer, m_pVisiblePointLightIndexBuffer->GetWriteState());
		m_pDetectVisiblePointLightsResources->m_RequiredResourceStates.emplace_back(pLightBoundsBuffer, pLightBoundsBuffer->GetReadState());

		m_pDetectVisiblePointLightsResources->m_SRVHeapStart = m_pShaderVisibleSRVHeap->Allocate();
		m_pDevice->CopyDescriptor(m_pDetectVisiblePointLightsResources->m_SRVHeapStart, m_pNumVisiblePointLightsBuffer->GetUAVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), m_pVisiblePointLightIndexBuffer->GetUAVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), pLightBoundsBuffer->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), m_pViewFrustumPointLightCullingDataBuffer->GetCBVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	}

	if (m_pSpotLightBuffer != nullptr)
	{
		ViewFrustumCullingPass::InitParams detectVisibleLightsParams;
		detectVisibleLightsParams.m_pRenderEnv = m_pRenderEnv;
		detectVisibleLightsParams.m_ObjectBoundsType = ObjectBoundsType_Sphere;
		detectVisibleLightsParams.m_NumObjects = m_pSpotLightBuffer->GetNumLights();

		m_pDetectVisibleSpotLightsPass = new ViewFrustumCullingPass(&detectVisibleLightsParams);
		Buffer* pLightBoundsBuffer = m_pSpotLightBuffer->GetLightBoundsBuffer();

		m_pDetectVisibleSpotLightsResources->m_RequiredResourceStates.emplace_back(m_pNumVisibleSpotLightsBuffer, m_pNumVisibleSpotLightsBuffer->GetWriteState());
		m_pDetectVisibleSpotLightsResources->m_RequiredResourceStates.emplace_back(m_pVisibleSpotLightIndexBuffer, m_pVisibleSpotLightIndexBuffer->GetWriteState());
		m_pDetectVisibleSpotLightsResources->m_RequiredResourceStates.emplace_back(pLightBoundsBuffer, pLightBoundsBuffer->GetReadState());

		m_pDetectVisibleSpotLightsResources->m_SRVHeapStart = m_pShaderVisibleSRVHeap->Allocate();
		m_pDevice->CopyDescriptor(m_pDetectVisibleSpotLightsResources->m_SRVHeapStart, m_pNumVisibleSpotLightsBuffer->GetUAVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), m_pVisibleSpotLightIndexBuffer->GetUAVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), pLightBoundsBuffer->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), m_pViewFrustumSpotLightCullingDataBuffer->GetCBVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	}
	
	TiledLightCullingPass::InitParams tiledLightCullingParams;
	tiledLightCullingParams.m_pRenderEnv = m_pRenderEnv;
	tiledLightCullingParams.m_TileSize = kTileSize;
	tiledLightCullingParams.m_NumTilesX = kNumTilesX;
	tiledLightCullingParams.m_NumTilesY = kNumTilesY;
	tiledLightCullingParams.m_MaxNumPointLights = (m_pPointLightBuffer != nullptr) ? m_pPointLightBuffer->GetNumLights() : 0;
	tiledLightCullingParams.m_MaxNumSpotLights = (m_pSpotLightBuffer != nullptr) ? m_pSpotLightBuffer->GetNumLights() : 0;

	m_pTiledLightCullingPass = new TiledLightCullingPass(&tiledLightCullingParams);
	
	m_pTiledLightCullingResources->m_RequiredResourceStates.emplace_back(m_pDepthTexture, m_pDepthTexture->GetReadState());
	m_pTiledLightCullingResources->m_SRVHeapStart = m_pShaderVisibleSRVHeap->Allocate();
	
	m_pDevice->CopyDescriptor(m_pTiledLightCullingResources->m_SRVHeapStart, m_pTiledLightCullingDataBuffer->GetCBVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), m_pDepthTexture->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	if (m_pPointLightBuffer != nullptr)
	{
		Buffer* pLightBoundsBuffer = m_pPointLightBuffer->GetLightBoundsBuffer();

		m_pTiledLightCullingResources->m_RequiredResourceStates.emplace_back(m_pNumVisiblePointLightsBuffer, m_pNumVisiblePointLightsBuffer->GetReadState());
		m_pTiledLightCullingResources->m_RequiredResourceStates.emplace_back(m_pVisiblePointLightIndexBuffer, m_pVisiblePointLightIndexBuffer->GetReadState());
		m_pTiledLightCullingResources->m_RequiredResourceStates.emplace_back(pLightBoundsBuffer, pLightBoundsBuffer->GetReadState());
		m_pTiledLightCullingResources->m_RequiredResourceStates.emplace_back(m_pNumPointLightsPerTileBuffer, m_pNumPointLightsPerTileBuffer->GetWriteState());
		m_pTiledLightCullingResources->m_RequiredResourceStates.emplace_back(m_pPointLightIndexPerTileBuffer, m_pPointLightIndexPerTileBuffer->GetWriteState());
		m_pTiledLightCullingResources->m_RequiredResourceStates.emplace_back(m_pPointLightRangePerTileBuffer, m_pPointLightRangePerTileBuffer->GetWriteState());

		m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), m_pNumVisiblePointLightsBuffer->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), m_pVisiblePointLightIndexBuffer->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), pLightBoundsBuffer->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), m_pNumPointLightsPerTileBuffer->GetUAVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), m_pPointLightIndexPerTileBuffer->GetUAVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), m_pPointLightRangePerTileBuffer->GetUAVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	}

	if (m_pSpotLightBuffer != nullptr)
	{
		Buffer* pLightBoundsBuffer = m_pSpotLightBuffer->GetLightBoundsBuffer();

		m_pTiledLightCullingResources->m_RequiredResourceStates.emplace_back(m_pNumVisibleSpotLightsBuffer, m_pNumVisibleSpotLightsBuffer->GetReadState());
		m_pTiledLightCullingResources->m_RequiredResourceStates.emplace_back(m_pVisibleSpotLightIndexBuffer, m_pVisibleSpotLightIndexBuffer->GetReadState());
		m_pTiledLightCullingResources->m_RequiredResourceStates.emplace_back(pLightBoundsBuffer, pLightBoundsBuffer->GetReadState());
		m_pTiledLightCullingResources->m_RequiredResourceStates.emplace_back(m_pNumSpotLightsPerTileBuffer, m_pNumSpotLightsPerTileBuffer->GetWriteState());
		m_pTiledLightCullingResources->m_RequiredResourceStates.emplace_back(m_pSpotLightIndexPerTileBuffer, m_pSpotLightIndexPerTileBuffer->GetWriteState());
		m_pTiledLightCullingResources->m_RequiredResourceStates.emplace_back(m_pSpotLightRangePerTileBuffer, m_pSpotLightRangePerTileBuffer->GetWriteState());

		m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), m_pNumVisibleSpotLightsBuffer->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), m_pVisibleSpotLightIndexBuffer->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), pLightBoundsBuffer->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), m_pNumSpotLightsPerTileBuffer->GetUAVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), m_pSpotLightIndexPerTileBuffer->GetUAVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), m_pSpotLightRangePerTileBuffer->GetUAVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	}

	CreateRenderGBufferCommandsPass::InitParams createRenderGBufferCommandsParams;
	createRenderGBufferCommandsParams.m_pRenderEnv = m_pRenderEnv;
	createRenderGBufferCommandsParams.m_NumMeshesInBatch = m_pMeshBatch->GetNumMeshes();
	
	m_pCreateRenderGBufferCommandsPass = new CreateRenderGBufferCommandsPass(&createRenderGBufferCommandsParams);

	m_pCreateRenderGBufferCommandsResources->m_RequiredResourceStates.emplace_back(m_pNumVisibleMeshesBuffer, m_pNumVisibleMeshesBuffer->GetReadState());
	m_pCreateRenderGBufferCommandsResources->m_RequiredResourceStates.emplace_back(m_pVisibleMeshIndexBuffer, m_pVisibleMeshIndexBuffer->GetReadState());
	m_pCreateRenderGBufferCommandsResources->m_RequiredResourceStates.emplace_back(pMeshDescBuffer, pMeshDescBuffer->GetReadState());
	m_pCreateRenderGBufferCommandsResources->m_RequiredResourceStates.emplace_back(m_pDrawMeshCommandBuffer, m_pDrawMeshCommandBuffer->GetWriteState());
	
	m_pCreateRenderGBufferCommandsResources->m_SRVHeapStart = m_pShaderVisibleSRVHeap->Allocate();
	m_pDevice->CopyDescriptor(m_pCreateRenderGBufferCommandsResources->m_SRVHeapStart, m_pNumVisibleMeshesBuffer->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), m_pVisibleMeshIndexBuffer->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), pMeshDescBuffer->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), m_pDrawMeshCommandBuffer->GetUAVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	RenderGBufferPass::InitParams renderGBufferParams;
	renderGBufferParams.m_pRenderEnv = m_pRenderEnv;
	renderGBufferParams.m_NormalRTVFormat = GetRenderTargetViewFormat(m_pNormalTexture->GetFormat());
	renderGBufferParams.m_DiffuseRTVFormat = GetRenderTargetViewFormat(m_pDiffuseTexture->GetFormat());
	renderGBufferParams.m_SpecularRTVFormat = GetRenderTargetViewFormat(m_pSpecularTexture->GetFormat());
	renderGBufferParams.m_DSVFormat = GetDepthStencilViewFormat(m_pDepthTexture->GetFormat());
	renderGBufferParams.m_pMeshBatch = m_pMeshBatch;
	
	m_pRenderGBufferPass = new RenderGBufferPass(&renderGBufferParams);
	
	m_pRenderGBufferResources->m_RequiredResourceStates.emplace_back(m_pNormalTexture, m_pNormalTexture->GetWriteState());
	m_pRenderGBufferResources->m_RequiredResourceStates.emplace_back(m_pDiffuseTexture, m_pDiffuseTexture->GetWriteState());
	m_pRenderGBufferResources->m_RequiredResourceStates.emplace_back(m_pSpecularTexture, m_pSpecularTexture->GetWriteState());
	m_pRenderGBufferResources->m_RequiredResourceStates.emplace_back(m_pDepthTexture, m_pDepthTexture->GetWriteState());
	m_pRenderGBufferResources->m_RequiredResourceStates.emplace_back(pMaterialBuffer, pMaterialBuffer->GetReadState());
	m_pRenderGBufferResources->m_RequiredResourceStates.emplace_back(m_pDrawMeshCommandBuffer, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
	m_pRenderGBufferResources->m_RequiredResourceStates.emplace_back(m_pNumVisibleMeshesBuffer, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);

	m_pRenderGBufferResources->m_RTVHeapStart = m_pShaderInvisibleRTVHeap->Allocate();
	m_pRenderGBufferResources->m_DSVHeapStart = m_pDepthTexture->GetDSVHandle();
	m_pRenderGBufferResources->m_SRVHeapStart = m_pShaderVisibleSRVHeap->Allocate();
	m_pDevice->CopyDescriptor(m_pRenderGBufferResources->m_RTVHeapStart, m_pNormalTexture->GetRTVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	m_pDevice->CopyDescriptor(m_pShaderInvisibleRTVHeap->Allocate(), m_pDiffuseTexture->GetRTVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	m_pDevice->CopyDescriptor(m_pShaderInvisibleRTVHeap->Allocate(), m_pSpecularTexture->GetRTVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	m_pDevice->CopyDescriptor(m_pRenderGBufferResources->m_SRVHeapStart, m_pObjectTransformBuffer->GetCBVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), pMaterialBuffer->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	m_pCreateRenderShadowMapCommandsArgumentBufferResources->m_RequiredResourceStates.emplace_back(m_pCreateRenderShadowMapCommandsArgumentBuffer, D3D12_RESOURCE_STATE_COPY_DEST);
	m_pCreateRenderShadowMapCommandsArgumentBufferResources->m_RequiredResourceStates.emplace_back(m_pNumVisibleMeshesBuffer, D3D12_RESOURCE_STATE_COPY_SOURCE);

	CreateRenderShadowMapCommandsPass::InitParams createRenderShadowMapCommandsParams;
	createRenderShadowMapCommandsParams.m_pRenderEnv = m_pRenderEnv;
	createRenderShadowMapCommandsParams.m_EnablePointLights = pScene->GetNumPointLights() > 0;
	createRenderShadowMapCommandsParams.m_MaxNumPointLightsPerShadowCaster = pScene->GetNumPointLights();
	createRenderShadowMapCommandsParams.m_EnableSpotLights = pScene->GetNumSpotLights() > 0;
	createRenderShadowMapCommandsParams.m_MaxNumSpotLightsPerShadowCaster = pScene->GetNumSpotLights();

	m_pCreateRenderShadowMapCommandsPass = new CreateRenderShadowMapCommandsPass(&createRenderShadowMapCommandsParams);

	if (pScene->GetNumPointLights() > 0)
	{
		FormattedBufferDesc shadowCastingLightIndexBufferDesc(m_pMeshBatch->GetNumMeshes() * pScene->GetNumPointLights(), DXGI_FORMAT_R32_UINT, true, true);
		m_pShadowCastingPointLightIndexBuffer = new Buffer(m_pRenderEnv, m_pRenderEnv->m_pDefaultHeapProps, &shadowCastingLightIndexBufferDesc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, L"m_pShadowCastingPointLightIndexBuffer");
		
		FormattedBufferDesc numShadowCastingLightsBufferDesc(1, DXGI_FORMAT_R32_UINT, false, true);
		m_pNumShadowCastingPointLightsBuffer = new Buffer(m_pRenderEnv, m_pRenderEnv->m_pDefaultHeapProps, &numShadowCastingLightsBufferDesc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, L"m_pNumShadowCastingPointLightsBuffer");

		StructuredBufferDesc drawCommandBufferDesc(m_pMeshBatch->GetNumMeshes(), sizeof(DrawMeshCommand), false, true);
		m_pDrawPointLightShadowCasterCommandBuffer = new Buffer(m_pRenderEnv, m_pRenderEnv->m_pDefaultHeapProps, &drawCommandBufferDesc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, L"m_pDrawPointLightShadowCasterCommandBuffer");
		
		FormattedBufferDesc numShadowCastersBufferDesc(1, DXGI_FORMAT_R32_UINT, false, true);
		m_pNumDrawPointLightShadowCastersBuffer = new Buffer(m_pRenderEnv, m_pRenderEnv->m_pDefaultHeapProps, &numShadowCastersBufferDesc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, L"m_pNumDrawPointLightShadowCastersBuffer");
	}

	if (pScene->GetNumSpotLights() > 0)
	{
		FormattedBufferDesc shadowCastingLightIndexBufferDesc(m_pMeshBatch->GetNumMeshes() * pScene->GetNumSpotLights(), DXGI_FORMAT_R32_UINT, true, true);
		m_pShadowCastingSpotLightIndexBuffer = new Buffer(m_pRenderEnv, m_pRenderEnv->m_pDefaultHeapProps, &shadowCastingLightIndexBufferDesc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, L"m_pShadowCastingSpotLightIndexBuffer");

		FormattedBufferDesc numShadowCastingLightsBufferDesc(1, DXGI_FORMAT_R32_UINT, false, true);
		m_pNumShadowCastingSpotLightsBuffer = new Buffer(m_pRenderEnv, m_pRenderEnv->m_pDefaultHeapProps, &numShadowCastingLightsBufferDesc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, L"m_pNumShadowCastingSpotLightsBuffer");

		StructuredBufferDesc drawCommandBufferDesc(m_pMeshBatch->GetNumMeshes(), sizeof(DrawMeshCommand), false, true);
		m_pDrawSpotLightShadowCasterCommandBuffer = new Buffer(m_pRenderEnv, m_pRenderEnv->m_pDefaultHeapProps, &drawCommandBufferDesc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, L"m_pDrawSpotLightShadowCasterCommandBuffer");

		FormattedBufferDesc numShadowCastersBufferDesc(1, DXGI_FORMAT_R32_UINT, false, true);
		m_pNumDrawSpotLightShadowCastersBuffer = new Buffer(m_pRenderEnv, m_pRenderEnv->m_pDefaultHeapProps, &numShadowCastersBufferDesc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, L"m_pNumDrawSpotLightShadowCastersBuffer");

#ifdef FOR_DEBUG_ONLY
		FormattedBufferDesc debugShadowCastingLightIndexBufferDesc(m_pMeshBatch->GetNumMeshes() * pScene->GetNumSpotLights(), DXGI_FORMAT_R32_UINT, false, false);
		m_pDebugShadowCastingSpotLightIndexBuffer = new Buffer(m_pRenderEnv, m_pRenderEnv->m_pReadbackHeapProps, &debugShadowCastingLightIndexBufferDesc, D3D12_RESOURCE_STATE_COPY_DEST, L"m_pDebugShadowCastingSpotLightIndexBuffer");

		FormattedBufferDesc debugNumShadowCastingLightsBufferDesc(1, DXGI_FORMAT_R32_UINT, false, false);
		m_pDebugNumShadowCastingSpotLightsBuffer = new Buffer(m_pRenderEnv, m_pRenderEnv->m_pReadbackHeapProps, &debugNumShadowCastingLightsBufferDesc, D3D12_RESOURCE_STATE_COPY_DEST, L"m_pDebugNumShadowCastingSpotLightsBuffer");
		
		StructuredBufferDesc debugDrawCommandBufferDesc(m_pMeshBatch->GetNumMeshes(), sizeof(DrawMeshCommand), false, false);
		m_pDebugDrawSpotLightShadowCasterCommandBuffer = new Buffer(m_pRenderEnv, m_pRenderEnv->m_pReadbackHeapProps, &debugDrawCommandBufferDesc, D3D12_RESOURCE_STATE_COPY_DEST, L"m_pDebugDrawSpotLightShadowCasterCommandBuffer");
		
		FormattedBufferDesc debugNumShadowCastersBufferDesc(1, DXGI_FORMAT_R32_UINT, false, false);
		m_pDebugNumDrawSpotLightShadowCastersBuffer = new Buffer(m_pRenderEnv, m_pRenderEnv->m_pReadbackHeapProps, &debugNumShadowCastersBufferDesc, D3D12_RESOURCE_STATE_COPY_DEST, L"m_pDebugNumDrawSpotLightShadowCastersBuffer");
#endif // FOR_DEBUG_ONLY
	}

	m_pCreateRenderShadowMapCommandsResources->m_RequiredResourceStates.emplace_back(m_pVisibleMeshIndexBuffer, m_pVisibleMeshIndexBuffer->GetReadState());
	m_pCreateRenderShadowMapCommandsResources->m_RequiredResourceStates.emplace_back(pMeshBoundsBuffer, pMeshBoundsBuffer->GetReadState());
	m_pCreateRenderShadowMapCommandsResources->m_RequiredResourceStates.emplace_back(pMeshDescBuffer, pMeshDescBuffer->GetReadState());
	m_pCreateRenderShadowMapCommandsResources->m_RequiredResourceStates.emplace_back(m_pCreateRenderShadowMapCommandsArgumentBuffer, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);

	m_pCreateRenderShadowMapCommandsResources->m_SRVHeapStart = m_pShaderVisibleSRVHeap->Allocate();
	m_pDevice->CopyDescriptor(m_pCreateRenderShadowMapCommandsResources->m_SRVHeapStart, m_pVisibleMeshIndexBuffer->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), pMeshBoundsBuffer->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), pMeshDescBuffer->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	if (m_pPointLightBuffer != nullptr)
	{
		Buffer* pLightBoundsBuffer = m_pPointLightBuffer->GetLightBoundsBuffer();

		m_pCreateRenderShadowMapCommandsResources->m_RequiredResourceStates.emplace_back(pLightBoundsBuffer, pLightBoundsBuffer->GetReadState());
		m_pCreateRenderShadowMapCommandsResources->m_RequiredResourceStates.emplace_back(m_pNumVisiblePointLightsBuffer, m_pNumVisiblePointLightsBuffer->GetReadState());
		m_pCreateRenderShadowMapCommandsResources->m_RequiredResourceStates.emplace_back(m_pVisiblePointLightIndexBuffer, m_pVisiblePointLightIndexBuffer->GetReadState());
		m_pCreateRenderShadowMapCommandsResources->m_RequiredResourceStates.emplace_back(m_pShadowCastingPointLightIndexBuffer, m_pShadowCastingPointLightIndexBuffer->GetWriteState());
		m_pCreateRenderShadowMapCommandsResources->m_RequiredResourceStates.emplace_back(m_pNumShadowCastingPointLightsBuffer, m_pNumShadowCastingPointLightsBuffer->GetWriteState());
		m_pCreateRenderShadowMapCommandsResources->m_RequiredResourceStates.emplace_back(m_pDrawPointLightShadowCasterCommandBuffer, m_pDrawPointLightShadowCasterCommandBuffer->GetWriteState());
		m_pCreateRenderShadowMapCommandsResources->m_RequiredResourceStates.emplace_back(m_pNumDrawPointLightShadowCastersBuffer, m_pNumDrawPointLightShadowCastersBuffer->GetWriteState());

		m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), pLightBoundsBuffer->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), m_pNumVisiblePointLightsBuffer->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), m_pVisiblePointLightIndexBuffer->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), m_pShadowCastingPointLightIndexBuffer->GetUAVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), m_pNumShadowCastingPointLightsBuffer->GetUAVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), m_pDrawPointLightShadowCasterCommandBuffer->GetUAVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), m_pNumDrawPointLightShadowCastersBuffer->GetUAVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	}

	if (m_pSpotLightBuffer != nullptr)
	{
		Buffer* pLightBoundsBuffer = m_pSpotLightBuffer->GetLightBoundsBuffer();

		m_pCreateRenderShadowMapCommandsResources->m_RequiredResourceStates.emplace_back(pLightBoundsBuffer, pLightBoundsBuffer->GetReadState());
		m_pCreateRenderShadowMapCommandsResources->m_RequiredResourceStates.emplace_back(m_pNumVisibleSpotLightsBuffer, m_pNumVisibleSpotLightsBuffer->GetReadState());
		m_pCreateRenderShadowMapCommandsResources->m_RequiredResourceStates.emplace_back(m_pVisibleSpotLightIndexBuffer, m_pVisibleSpotLightIndexBuffer->GetReadState());
		m_pCreateRenderShadowMapCommandsResources->m_RequiredResourceStates.emplace_back(m_pShadowCastingSpotLightIndexBuffer, m_pShadowCastingSpotLightIndexBuffer->GetWriteState());
		m_pCreateRenderShadowMapCommandsResources->m_RequiredResourceStates.emplace_back(m_pNumShadowCastingSpotLightsBuffer, m_pNumShadowCastingSpotLightsBuffer->GetWriteState());
		m_pCreateRenderShadowMapCommandsResources->m_RequiredResourceStates.emplace_back(m_pDrawSpotLightShadowCasterCommandBuffer, m_pDrawSpotLightShadowCasterCommandBuffer->GetWriteState());
		m_pCreateRenderShadowMapCommandsResources->m_RequiredResourceStates.emplace_back(m_pNumDrawSpotLightShadowCastersBuffer, m_pNumDrawSpotLightShadowCastersBuffer->GetWriteState());

		m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), pLightBoundsBuffer->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), m_pNumVisibleSpotLightsBuffer->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), m_pVisibleSpotLightIndexBuffer->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), m_pShadowCastingSpotLightIndexBuffer->GetUAVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), m_pNumShadowCastingSpotLightsBuffer->GetUAVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), m_pDrawSpotLightShadowCasterCommandBuffer->GetUAVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), m_pNumDrawSpotLightShadowCastersBuffer->GetUAVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	}

	TiledShadingPass::InitParams tiledShadingParams;
	tiledShadingParams.m_pRenderEnv = m_pRenderEnv;
	tiledShadingParams.m_ShadingMode = ShadingMode_Phong;
	tiledShadingParams.m_TileSize = kTileSize;
	tiledShadingParams.m_NumTilesX = kNumTilesX;
	tiledShadingParams.m_NumTilesY = kNumTilesY;
	tiledShadingParams.m_EnablePointLights = (m_pPointLightBuffer != nullptr);
	tiledShadingParams.m_EnableSpotLights = (m_pSpotLightBuffer != nullptr);
	tiledShadingParams.m_EnableDirectionalLight = (pScene->GetDirectionalLight() != nullptr);

	m_pTiledShadingPass = new TiledShadingPass(&tiledShadingParams);

	m_pTiledShadingResources->m_RequiredResourceStates.emplace_back(m_pAccumLightTexture, m_pAccumLightTexture->GetWriteState());
	m_pTiledShadingResources->m_RequiredResourceStates.emplace_back(m_pDepthTexture, m_pDepthTexture->GetReadState());
	m_pTiledShadingResources->m_RequiredResourceStates.emplace_back(m_pNormalTexture, m_pNormalTexture->GetReadState());
	m_pTiledShadingResources->m_RequiredResourceStates.emplace_back(m_pDiffuseTexture, m_pDiffuseTexture->GetReadState());
	m_pTiledShadingResources->m_RequiredResourceStates.emplace_back(m_pSpecularTexture, m_pSpecularTexture->GetReadState());

	m_pTiledShadingResources->m_SRVHeapStart = m_pShaderVisibleSRVHeap->Allocate();
	m_pDevice->CopyDescriptor(m_pTiledShadingResources->m_SRVHeapStart, m_pAccumLightTexture->GetUAVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), m_pTiledShadingDataBuffer->GetCBVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), m_pDepthTexture->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), m_pNormalTexture->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), m_pDiffuseTexture->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), m_pSpecularTexture->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	if (m_pPointLightBuffer != nullptr)
	{
		Buffer* pLightBoundsBuffer = m_pPointLightBuffer->GetLightBoundsBuffer();
		Buffer* pLightPropsBuffer = m_pPointLightBuffer->GetLightPropsBuffer();

		m_pTiledShadingResources->m_RequiredResourceStates.emplace_back(pLightBoundsBuffer, pLightBoundsBuffer->GetReadState());
		m_pTiledShadingResources->m_RequiredResourceStates.emplace_back(pLightPropsBuffer, pLightPropsBuffer->GetReadState());
		m_pTiledShadingResources->m_RequiredResourceStates.emplace_back(m_pPointLightIndexPerTileBuffer, m_pPointLightIndexPerTileBuffer->GetReadState());
		m_pTiledShadingResources->m_RequiredResourceStates.emplace_back(m_pPointLightRangePerTileBuffer, m_pPointLightRangePerTileBuffer->GetReadState());

		m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), pLightBoundsBuffer->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), pLightPropsBuffer->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), m_pPointLightIndexPerTileBuffer->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), m_pPointLightRangePerTileBuffer->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	}

	if (m_pSpotLightBuffer != nullptr)
	{
		Buffer* pLightBoundsBuffer = m_pSpotLightBuffer->GetLightBoundsBuffer();
		Buffer* pLightPropsBuffer = m_pSpotLightBuffer->GetLightPropsBuffer();
		Buffer* pLightFrustumBuffer = m_pSpotLightBuffer->GetLightFrustumBuffer();
		Buffer* pLightViewProjMatrixBuffer = m_pSpotLightBuffer->GetLightViewProjMatrixBuffer();

		m_pTiledShadingResources->m_RequiredResourceStates.emplace_back(pLightBoundsBuffer, pLightBoundsBuffer->GetReadState());
		m_pTiledShadingResources->m_RequiredResourceStates.emplace_back(pLightPropsBuffer, pLightPropsBuffer->GetReadState());
		m_pTiledShadingResources->m_RequiredResourceStates.emplace_back(m_pSpotLightIndexPerTileBuffer, m_pSpotLightIndexPerTileBuffer->GetReadState());
		m_pTiledShadingResources->m_RequiredResourceStates.emplace_back(m_pSpotLightRangePerTileBuffer, m_pSpotLightRangePerTileBuffer->GetReadState());

		m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), pLightBoundsBuffer->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), pLightPropsBuffer->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), m_pSpotLightIndexPerTileBuffer->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), m_pSpotLightRangePerTileBuffer->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		SetupTiledShadowMapPass::InitParams setupTiledShadowMapParams;
		setupTiledShadowMapParams.m_pRenderEnv = m_pRenderEnv;
		setupTiledShadowMapParams.m_NumTilesX = kNumShadowMapTiles;
		setupTiledShadowMapParams.m_NumTilesY = kNumShadowMapTiles;

		m_pSetupSpotLightTiledShadowMapPass = new SetupTiledShadowMapPass(&setupTiledShadowMapParams);
		
		m_pSetupSpotLightTiledShadowMapResources->m_RequiredResourceStates.emplace_back(m_pNumVisibleSpotLightsBuffer, m_pNumVisibleSpotLightsBuffer->GetReadState());
		m_pSetupSpotLightTiledShadowMapResources->m_RequiredResourceStates.emplace_back(m_pVisibleSpotLightIndexBuffer, m_pVisibleSpotLightIndexBuffer->GetReadState());
		m_pSetupSpotLightTiledShadowMapResources->m_RequiredResourceStates.emplace_back(pLightViewProjMatrixBuffer, pLightViewProjMatrixBuffer->GetReadState());
		m_pSetupSpotLightTiledShadowMapResources->m_RequiredResourceStates.emplace_back(m_pSpotLightShadowMapTileBuffer, m_pSpotLightShadowMapTileBuffer->GetWriteState());
		m_pSetupSpotLightTiledShadowMapResources->m_RequiredResourceStates.emplace_back(m_pSpotLightViewTileProjMatrixBuffer, m_pSpotLightViewTileProjMatrixBuffer->GetWriteState());
		
		m_pSetupSpotLightTiledShadowMapResources->m_SRVHeapStart = m_pShaderVisibleSRVHeap->Allocate();
		m_pDevice->CopyDescriptor(m_pSetupSpotLightTiledShadowMapResources->m_SRVHeapStart, m_pSpotLightShadowMapDataBuffer->GetCBVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), m_pNumVisibleSpotLightsBuffer->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), m_pVisibleSpotLightIndexBuffer->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), pLightViewProjMatrixBuffer->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), m_pSpotLightShadowMapTileBuffer->GetUAVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), m_pSpotLightViewTileProjMatrixBuffer->GetUAVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		
		RenderTiledShadowMapPass::InitParams renderTiledShadowMapParams;
		renderTiledShadowMapParams.m_pRenderEnv = m_pRenderEnv;
		renderTiledShadowMapParams.m_DSVFormat = GetDepthStencilViewFormat(m_pSpotLightTiledShadowMap->GetFormat());
		renderTiledShadowMapParams.m_pMeshBatch = m_pMeshBatch;
		renderTiledShadowMapParams.m_LightType = LightType_Spot;

		m_pRenderSpotLightTiledShadowMapPass = new RenderTiledShadowMapPass(&renderTiledShadowMapParams);
		
		m_pRenderSpotLightTiledShadowMapResources->m_RequiredResourceStates.emplace_back(m_pSpotLightTiledShadowMap, m_pSpotLightTiledShadowMap->GetWriteState());
		m_pRenderSpotLightTiledShadowMapResources->m_RequiredResourceStates.emplace_back(pLightPropsBuffer, pLightPropsBuffer->GetReadState());
		m_pRenderSpotLightTiledShadowMapResources->m_RequiredResourceStates.emplace_back(m_pSpotLightViewTileProjMatrixBuffer, m_pSpotLightViewTileProjMatrixBuffer->GetReadState());
		m_pRenderSpotLightTiledShadowMapResources->m_RequiredResourceStates.emplace_back(pLightFrustumBuffer, pLightFrustumBuffer->GetReadState());
		m_pRenderSpotLightTiledShadowMapResources->m_RequiredResourceStates.emplace_back(m_pDrawSpotLightShadowCasterCommandBuffer, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
		m_pRenderSpotLightTiledShadowMapResources->m_RequiredResourceStates.emplace_back(m_pNumDrawSpotLightShadowCastersBuffer, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
		
		m_pRenderSpotLightTiledShadowMapResources->m_DSVHeapStart = m_pSpotLightTiledShadowMap->GetDSVHandle();
		m_pRenderSpotLightTiledShadowMapResources->m_SRVHeapStart = m_pShaderVisibleSRVHeap->Allocate();

		m_pDevice->CopyDescriptor(m_pRenderSpotLightTiledShadowMapResources->m_SRVHeapStart, m_pObjectTransformBuffer->GetCBVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), pLightPropsBuffer->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), m_pSpotLightViewTileProjMatrixBuffer->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), pLightFrustumBuffer->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	}
	
	ClearVoxelGridPass::InitParams clearGridParams;
	clearGridParams.m_pRenderEnv = m_pRenderEnv;
	clearGridParams.m_NumGridCellsX = kNumGridCellsX;
	clearGridParams.m_NumGridCellsY = kNumGridCellsY;
	clearGridParams.m_NumGridCellsZ = kNumGridCellsZ;

	m_pClearVoxelGridPass = new ClearVoxelGridPass(&clearGridParams);

	m_pClearVoxelGridResources->m_RequiredResourceStates.emplace_back(m_pGridBuffer, m_pGridBuffer->GetWriteState());
	m_pClearVoxelGridResources->m_SRVHeapStart = m_pShaderVisibleSRVHeap->Allocate();
	
	m_pDevice->CopyDescriptor(m_pClearVoxelGridResources->m_SRVHeapStart, m_pGridConfigBuffer->GetCBVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), m_pGridBuffer->GetUAVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

#ifdef ENABLE_INDIRECT_LIGHTING
	CreateVoxelGridPass::InitParams createGridParams;
	createGridParams.m_pRenderEnv = m_pRenderEnv;
	createGridParams.m_pMeshBatch = m_pMeshBatch;

	m_pCreateVoxelGridPass = new CreateVoxelGridPass(&createGridParams);

	m_pCreateVoxelGridResources->m_RequiredResourceStates.emplace_back(pMaterialBuffer, pMaterialBuffer->GetReadState());
	m_pCreateVoxelGridResources->m_RequiredResourceStates.emplace_back(m_pGridBuffer, m_pGridBuffer->GetWriteState());	
	m_pCreateVoxelGridResources->m_RequiredResourceStates.emplace_back(m_pDrawMeshCommandBuffer, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
	m_pCreateVoxelGridResources->m_RequiredResourceStates.emplace_back(m_pNumVisibleMeshesBuffer, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
	
	m_pCreateVoxelGridResources->m_SRVHeapStart = m_pShaderVisibleSRVHeap->Allocate();
	m_pDevice->CopyDescriptor(m_pCreateVoxelGridResources->m_SRVHeapStart, m_pObjectTransformBuffer->GetCBVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), m_pCameraTransformBuffer->GetCBVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), m_pGridConfigBuffer->GetCBVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), pMaterialBuffer->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), m_pGridBuffer->GetUAVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	InjectVPLsIntoVoxelGridPass::InitPrams injectVPLsParams;
	injectVPLsParams.m_pRenderEnv = m_pRenderEnv;
	injectVPLsParams.m_NumGridCellsX = kNumGridCellsX;
	injectVPLsParams.m_NumGridCellsY = kNumGridCellsY;
	injectVPLsParams.m_NumGridCellsZ = kNumGridCellsZ;

	//m_pInjectVPLsIntoVoxelGridPass = new InjectVPLsIntoVoxelGridPass(&injectVPLsParams);

	VisualizeVoxelGridPass::InitParams visualizeGridParams;
	visualizeGridParams.m_pRenderEnv = m_pRenderEnv;
	visualizeGridParams.m_RTVFormat = GetRenderTargetViewFormat(m_pSwapChain->GetBackBuffer(m_BackBufferIndex)->GetFormat());
	
	m_pVisualizeVoxelGridPass = new VisualizeVoxelGridPass(&visualizeGridParams);

	for (u8 index = 0; index < kNumBackBuffers; ++index)
	{
		ColorTexture* pRenderTarget = m_pSwapChain->GetBackBuffer(index);

		m_VisualizeVoxelGridResources[index]->m_RequiredResourceStates.emplace_back(pRenderTarget, pRenderTarget->GetWriteState());
		m_VisualizeVoxelGridResources[index]->m_RequiredResourceStates.emplace_back(m_pDepthTexture, m_pDepthTexture->GetReadState());

		m_VisualizeVoxelGridResources[index]->m_RTVHeapStart = pRenderTarget->GetRTVHandle();
		m_VisualizeVoxelGridResources[index]->m_SRVHeapStart = m_pShaderVisibleSRVHeap->Allocate();

		m_pDevice->CopyDescriptor(m_VisualizeVoxelGridResources[index]->m_SRVHeapStart, m_pGridConfigBuffer->GetCBVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), m_pCameraTransformBuffer->GetCBVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), m_pDepthTexture->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), m_pGridBuffer->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	}
#endif // ENABLE_INDIRECT_LIGHTING

	VisualizeTexturePass::InitParams visualizeTextureParams;
	visualizeTextureParams.m_pRenderEnv = m_pRenderEnv;
	visualizeTextureParams.m_RTVFormat = GetRenderTargetViewFormat(m_pSwapChain->GetBackBuffer(m_BackBufferIndex)->GetFormat());

	switch (kOutputResult)
	{
		case OutputResult_GBufferDiffuse:
		{
			visualizeTextureParams.m_TextureType = VisualizeTexturePass::TextureType_GBufferDiffuse;
			break;
		}
		case OutputResult_GBufferSpecular:
		{
			visualizeTextureParams.m_TextureType = VisualizeTexturePass::TextureType_GBufferSpecular;
			break;
		}
		case OutputResult_GBufferNormal:
		{
			visualizeTextureParams.m_TextureType = VisualizeTexturePass::TextureType_GBufferNormal;
			break;
		}
		case OutputResult_Depth:
		case OutputResult_SpotLightTiledShadowMap:
		{
			visualizeTextureParams.m_TextureType = VisualizeTexturePass::TextureType_Depth;
			break;
		}
		case OutputResult_AccumLight:
		{
			visualizeTextureParams.m_TextureType = VisualizeTexturePass::TextureType_Other;
			break;
		}
		default:
		{
			assert(false);
		}
	}

	m_pVisualizeTexturePass = new VisualizeTexturePass(&visualizeTextureParams);

	for (u8 index = 0; index < kNumBackBuffers; ++index)
	{
		ColorTexture* pRenderTarget = m_pSwapChain->GetBackBuffer(index);
		m_VisualizeTextureResources[index]->m_RequiredResourceStates.emplace_back(pRenderTarget, pRenderTarget->GetWriteState());
		m_VisualizeTextureResources[index]->m_RTVHeapStart = pRenderTarget->GetRTVHandle();
		
		m_VisualizeTextureResources[index]->m_SRVHeapStart = m_pShaderVisibleSRVHeap->Allocate();
		m_pDevice->CopyDescriptor(m_VisualizeTextureResources[index]->m_SRVHeapStart, m_pVisualizeGBufferDataBuffer->GetCBVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		switch (kOutputResult)
		{
			case OutputResult_GBufferDiffuse:
			{
				m_VisualizeTextureResources[index]->m_RequiredResourceStates.emplace_back(m_pDiffuseTexture, m_pDiffuseTexture->GetReadState());
				m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), m_pDiffuseTexture->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
				break;
			}
			case OutputResult_GBufferSpecular:
			{
				m_VisualizeTextureResources[index]->m_RequiredResourceStates.emplace_back(m_pSpecularTexture, m_pSpecularTexture->GetReadState());
				m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), m_pSpecularTexture->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
				break;
			}
			case OutputResult_GBufferNormal:
			{
				m_VisualizeTextureResources[index]->m_RequiredResourceStates.emplace_back(m_pNormalTexture, m_pNormalTexture->GetReadState());
				m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), m_pNormalTexture->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
				break;
			}
			case OutputResult_Depth:
			{
				m_VisualizeTextureResources[index]->m_RequiredResourceStates.emplace_back(m_pDepthTexture, m_pDepthTexture->GetReadState());
				m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), m_pDepthTexture->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
				break;
			}
			case OutputResult_SpotLightTiledShadowMap:
			{
				m_VisualizeTextureResources[index]->m_RequiredResourceStates.emplace_back(m_pSpotLightTiledShadowMap, m_pSpotLightTiledShadowMap->GetReadState());
				m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), m_pSpotLightTiledShadowMap->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
				break;
			}
			case OutputResult_AccumLight:
			{
				m_VisualizeTextureResources[index]->m_RequiredResourceStates.emplace_back(m_pAccumLightTexture, m_pAccumLightTexture->GetReadState());
				m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), m_pAccumLightTexture->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
				break;
			}
		}
	}

#ifdef FOR_DEBUG_ONLY
	m_pDebugResources->m_RequiredResourceStates.emplace_back(m_pNumVisibleMeshesBuffer, D3D12_RESOURCE_STATE_COPY_SOURCE);
	m_pDebugResources->m_RequiredResourceStates.emplace_back(m_pVisibleMeshIndexBuffer, D3D12_RESOURCE_STATE_COPY_SOURCE);
	m_pDebugResources->m_RequiredResourceStates.emplace_back(m_pShadowCastingSpotLightIndexBuffer, D3D12_RESOURCE_STATE_COPY_SOURCE);
	m_pDebugResources->m_RequiredResourceStates.emplace_back(m_pNumShadowCastingSpotLightsBuffer, D3D12_RESOURCE_STATE_COPY_SOURCE);
	m_pDebugResources->m_RequiredResourceStates.emplace_back(m_pDrawSpotLightShadowCasterCommandBuffer, D3D12_RESOURCE_STATE_COPY_SOURCE);
	m_pDebugResources->m_RequiredResourceStates.emplace_back(m_pNumDrawSpotLightShadowCastersBuffer, D3D12_RESOURCE_STATE_COPY_SOURCE);
	m_pDebugResources->m_RequiredResourceStates.emplace_back(m_pSpotLightShadowMapTileBuffer, D3D12_RESOURCE_STATE_COPY_SOURCE);
	m_pDebugResources->m_RequiredResourceStates.emplace_back(m_pSpotLightViewTileProjMatrixBuffer, D3D12_RESOURCE_STATE_COPY_SOURCE);
	m_pDebugResources->m_RequiredResourceStates.emplace_back(m_pSpotLightShadowMapTileBuffer, D3D12_RESOURCE_STATE_COPY_SOURCE);
#endif // FOR_DEBUG_ONLY

	// Kolya: Should be moved to OnUpdate
	// Temporarily moved constant buffer update here to overcome frame capture crash on AMD R9 290
	const Vector3f& mainCameraPos = m_pCamera->GetTransform().GetPosition();
	const Quaternion& mainCameraRotation = m_pCamera->GetTransform().GetRotation();
	const Matrix4f mainViewProjMatrix = m_pCamera->GetViewMatrix() * m_pCamera->GetProjMatrix();
	const Frustum mainCameraFrustum = ExtractWorldFrustum(*m_pCamera);

	ObjectTransform objectTransform;
	objectTransform.m_WorldPositionMatrix = Matrix4f::IDENTITY;
	objectTransform.m_WorldNormalMatrix = Matrix4f::IDENTITY;
	objectTransform.m_WorldViewProjMatrix = mainViewProjMatrix;

	m_pObjectTransformBuffer->Write(&objectTransform, sizeof(objectTransform));

	const BasisAxes mainCameraBasis = ExtractBasisAxes(mainCameraRotation);
	//assert(IsNormalized(mainCameraBasis.m_XAxis));
	//assert(IsNormalized(mainCameraBasis.m_YAxis));
	//assert(IsNormalized(mainCameraBasis.m_ZAxis));

	const Vector3f gridSize(kGridSizeX, kGridSizeY, kGridSizeZ);
	const Vector3f gridHalfSize(0.5f * gridSize);
	const Vector3f gridNumCells(kNumGridCellsX, kNumGridCellsY, kNumGridCellsZ);
	const Vector3f gridRcpCellSize(Rcp(gridSize / gridNumCells));
	// Kolya: Hard-coding grid center for now
	//const Vector3f gridCenter(mainCameraPos + (0.25f * gridSize.m_Z) * mainCameraBasis.m_ZAxis);
	const Vector3f gridCenter(278.0f, 274.0f, -279.0f);
	const Vector3f gridMinPoint(gridCenter - gridHalfSize);

	GridConfig gridConfig;
	gridConfig.m_WorldSpaceOrigin = Vector4f(gridMinPoint.m_X, gridMinPoint.m_Y, gridMinPoint.m_Z, 0.0f);
	gridConfig.m_RcpCellSize = Vector4f(gridRcpCellSize.m_X, gridRcpCellSize.m_Y, gridRcpCellSize.m_Z, 0.0f);
	gridConfig.m_NumCells = Vector4i(kNumGridCellsX, kNumGridCellsY, kNumGridCellsZ, 0);

	m_pGridConfigBuffer->Write(&gridConfig, sizeof(gridConfig));

	Camera xAxisCamera(Camera::ProjType_Ortho, 0.0f, gridSize.m_X, gridSize.m_Z / gridSize.m_Y);
	xAxisCamera.SetSizeY(gridSize.m_Y);
	xAxisCamera.GetTransform().SetPosition(gridCenter - gridHalfSize.m_X * mainCameraBasis.m_XAxis);
	xAxisCamera.GetTransform().SetRotation(mainCameraRotation * CreateRotationYQuaternion(Radian(PI_DIV_TWO)));

	Camera yAxisCamera(Camera::ProjType_Ortho, 0.0f, gridSize.m_Y, gridSize.m_X / gridSize.m_Z);
	yAxisCamera.SetSizeY(gridSize.m_Z);
	yAxisCamera.GetTransform().SetPosition(gridCenter - gridHalfSize.m_Y * mainCameraBasis.m_YAxis);
	yAxisCamera.GetTransform().SetRotation(mainCameraRotation * CreateRotationXQuaternion(Radian(-PI_DIV_TWO)));

	Camera zAxisCamera(Camera::ProjType_Ortho, 0.0f, gridSize.m_Z, gridSize.m_X / gridSize.m_Y);
	zAxisCamera.SetSizeY(gridSize.m_Y);
	zAxisCamera.GetTransform().SetPosition(gridCenter - gridHalfSize.m_Z * mainCameraBasis.m_ZAxis);
	zAxisCamera.GetTransform().SetRotation(mainCameraRotation);

	CameraTransform cameraTransform;
	cameraTransform.m_ViewProjInvMatrix = Inverse(mainViewProjMatrix);
	cameraTransform.m_ViewProjMatrices[0] = xAxisCamera.GetViewMatrix() * xAxisCamera.GetProjMatrix();
	cameraTransform.m_ViewProjMatrices[1] = yAxisCamera.GetViewMatrix() * yAxisCamera.GetProjMatrix();
	cameraTransform.m_ViewProjMatrices[2] = zAxisCamera.GetViewMatrix() * zAxisCamera.GetProjMatrix();

	m_pCameraTransformBuffer->Write(&cameraTransform, sizeof(cameraTransform));
			
	ViewFrustumCullingData viewFrustumCullingData;
	for (u8 planeIndex = 0; planeIndex < Frustum::NumPlanes; ++planeIndex)
		viewFrustumCullingData.m_ViewFrustumPlanes[planeIndex] = mainCameraFrustum.m_Planes[planeIndex];

	viewFrustumCullingData.m_NumObjects = m_pMeshBatch->GetNumMeshes();
	m_pViewFrustumMeshCullingDataBuffer->Write(&viewFrustumCullingData, sizeof(viewFrustumCullingData));

	viewFrustumCullingData.m_NumObjects = pScene->GetNumPointLights();
	m_pViewFrustumPointLightCullingDataBuffer->Write(&viewFrustumCullingData, sizeof(viewFrustumCullingData));

	viewFrustumCullingData.m_NumObjects = pScene->GetNumSpotLights();
	m_pViewFrustumSpotLightCullingDataBuffer->Write(&viewFrustumCullingData, sizeof(viewFrustumCullingData));

	TiledLightCullingData tiledLightCullingData;
	tiledLightCullingData.m_RcpScreenSize = Rcp(Vector2f((f32)backBufferWidth, (f32)backBufferHeight));
	tiledLightCullingData.m_ViewMatrix = m_pCamera->GetViewMatrix();
	tiledLightCullingData.m_ProjMatrix = m_pCamera->GetProjMatrix();
	tiledLightCullingData.m_ProjInvMatrix = Inverse(m_pCamera->GetProjMatrix());

	m_pTiledLightCullingDataBuffer->Write(&tiledLightCullingData, sizeof(tiledLightCullingData));

	TiledShadingData tiledShadingData;
	tiledShadingData.m_RcpScreenSize = Rcp(Vector2f((f32)backBufferWidth, (f32)backBufferHeight));
	tiledShadingData.m_WorldSpaceCameraPos = mainCameraPos;
	tiledShadingData.m_ViewProjInvMatrix = Inverse(m_pCamera->GetViewMatrix() * m_pCamera->GetProjMatrix());
	
	const DirectionalLight* pDirectionalLight = pScene->GetDirectionalLight();
	if (pDirectionalLight != nullptr)
	{
		const BasisAxes lightBasis = ExtractBasisAxes(pDirectionalLight->GetTransform().GetRotation());
		
		tiledShadingData.m_WorldSpaceLightDir = Normalize(lightBasis.m_ZAxis);
		tiledShadingData.m_LightColor = pDirectionalLight->GetColor();
	}
	m_pTiledShadingDataBuffer->Write(&tiledShadingData, sizeof(tiledShadingData));

	ShadowMapData spotLightShadowMapData;
	spotLightShadowMapData.m_SizeInPixels = f32(kTiledShadowMapSize);
	spotLightShadowMapData.m_RcpSizeInPixels = Rcp(spotLightShadowMapData.m_SizeInPixels);
	spotLightShadowMapData.m_TileSizeInPixels = f32(kShadowMapTileSize);
	spotLightShadowMapData.m_RcpTileSizeInPixels = Rcp(spotLightShadowMapData.m_TileSizeInPixels);

	m_pSpotLightShadowMapDataBuffer->Write(&spotLightShadowMapData, sizeof(spotLightShadowMapData));
	
	VisualizeGBufferData visualizeGBufferData;
	visualizeGBufferData.m_CameraProjMatrix = m_pCamera->GetProjMatrix();
	visualizeGBufferData.m_CameraNearPlane = m_pCamera->GetNearClipPlane();
	visualizeGBufferData.m_CameraFarPlane = m_pCamera->GetFarClipPlane();
	
	m_pVisualizeGBufferDataBuffer->Write(&visualizeGBufferData, sizeof(visualizeGBufferData));

	SafeDelete(pScene);
}

void DXApplication::OnUpdate()
{
}

void DXApplication::OnRender()
{
	std::vector<CommandList*> submissionBatch;

	ColorTexture* pRenderTarget = m_pSwapChain->GetBackBuffer(m_BackBufferIndex);
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = pRenderTarget->GetRTVHandle();
	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = m_pDepthTexture->GetDSVHandle();
	
	const u8 clearFlags = m_pCamera->GetClearFlags();
	if (clearFlags != 0)
	{
		CommandList* pClearCommandList = m_pCommandListPool->Create(L"pClearCommandList");

		std::vector<ResourceTransitionBarrier> resourceTransitions;
		if (pRenderTarget->GetState() != pRenderTarget->GetWriteState())
		{
			resourceTransitions.emplace_back(pRenderTarget, pRenderTarget->GetState(), pRenderTarget->GetWriteState());
			pRenderTarget->SetState(pRenderTarget->GetWriteState());
		}
		if (m_pDepthTexture->GetState() != m_pDepthTexture->GetWriteState())
		{
			resourceTransitions.emplace_back(m_pDepthTexture, m_pDepthTexture->GetState(), m_pDepthTexture->GetWriteState());
			m_pDepthTexture->SetState(m_pDepthTexture->GetWriteState());
		}
		
		const Vector4f& clearColor = m_pCamera->GetBackgroundColor();

		pClearCommandList->Begin();
		pClearCommandList->ResourceBarrier(resourceTransitions.size(), &resourceTransitions[0]);
		pClearCommandList->ClearRenderTargetView(rtvHandle, &clearColor.m_X);
		pClearCommandList->ClearDepthView(dsvHandle);
		pClearCommandList->End();

		submissionBatch.emplace_back(pClearCommandList);
	}

	ViewFrustumCullingPass::RenderParams detectVisibleMeshesParams;
	detectVisibleMeshesParams.m_pRenderEnv = m_pRenderEnv;
	detectVisibleMeshesParams.m_pCommandList = m_pCommandListPool->Create(L"pDetectVisibleMeshesCommandList");
	detectVisibleMeshesParams.m_pResources = m_pDetectVisibleMeshesResources;
	detectVisibleMeshesParams.m_pNumVisibleObjectsBuffer = m_pNumVisibleMeshesBuffer;

	m_pDetectVisibleMeshesPass->Record(&detectVisibleMeshesParams);
	submissionBatch.emplace_back(detectVisibleMeshesParams.m_pCommandList);
	
	if (m_pPointLightBuffer != nullptr)
	{
		ViewFrustumCullingPass::RenderParams detectVisibleLightsParams;
		detectVisibleLightsParams.m_pRenderEnv = m_pRenderEnv;
		detectVisibleLightsParams.m_pCommandList = m_pCommandListPool->Create(L"pDetectVisiblePointLightsCommandList");
		detectVisibleLightsParams.m_pResources = m_pDetectVisiblePointLightsResources;
		detectVisibleLightsParams.m_pNumVisibleObjectsBuffer = m_pNumVisiblePointLightsBuffer;

		m_pDetectVisiblePointLightsPass->Record(&detectVisibleLightsParams);
		submissionBatch.emplace_back(detectVisibleLightsParams.m_pCommandList);
	}

	if (m_pSpotLightBuffer != nullptr)
	{
		ViewFrustumCullingPass::RenderParams detectVisibleLightsParams;
		detectVisibleLightsParams.m_pRenderEnv = m_pRenderEnv;
		detectVisibleLightsParams.m_pCommandList = m_pCommandListPool->Create(L"pDetectVisibleSpotLightsCommandList");
		detectVisibleLightsParams.m_pResources = m_pDetectVisibleSpotLightsResources;
		detectVisibleLightsParams.m_pNumVisibleObjectsBuffer = m_pNumVisibleSpotLightsBuffer;

		m_pDetectVisibleSpotLightsPass->Record(&detectVisibleLightsParams);
		submissionBatch.emplace_back(detectVisibleLightsParams.m_pCommandList);
	}
	
	CreateRenderGBufferCommandsPass::RenderParams createRenderGBufferCommandsParams;
	createRenderGBufferCommandsParams.m_pRenderEnv = m_pRenderEnv;
	createRenderGBufferCommandsParams.m_pCommandList = m_pCommandListPool->Create(L"pCreateRenderGBufferCommandsCommandList");
	createRenderGBufferCommandsParams.m_pResources = m_pCreateRenderGBufferCommandsResources;

	m_pCreateRenderGBufferCommandsPass->Record(&createRenderGBufferCommandsParams);
	submissionBatch.emplace_back(createRenderGBufferCommandsParams.m_pCommandList);

	RenderGBufferPass::RenderParams renderGBufferParams;
	renderGBufferParams.m_pRenderEnv = m_pRenderEnv;
	renderGBufferParams.m_pCommandList = m_pCommandListPool->Create(L"pRenderGBufferCommandList");
	renderGBufferParams.m_pResources = m_pRenderGBufferResources;
	renderGBufferParams.m_pViewport = m_pBackBufferViewport;
	renderGBufferParams.m_pMeshBatch = m_pMeshBatch;
	renderGBufferParams.m_pDrawMeshCommandBuffer = m_pDrawMeshCommandBuffer;
	renderGBufferParams.m_pNumDrawMeshesBuffer = m_pNumVisibleMeshesBuffer;
	
	m_pRenderGBufferPass->Record(&renderGBufferParams);
	submissionBatch.emplace_back(renderGBufferParams.m_pCommandList);

	TiledLightCullingPass::RenderParams tiledLightCullingParams;
	tiledLightCullingParams.m_pRenderEnv = m_pRenderEnv;
	tiledLightCullingParams.m_pCommandList = m_pCommandListPool->Create(L"pTiledLightCullingCommandList");
	tiledLightCullingParams.m_pResources = m_pTiledLightCullingResources;
	tiledLightCullingParams.m_pNumPointLightsPerTileBuffer = m_pNumPointLightsPerTileBuffer;
	tiledLightCullingParams.m_pNumSpotLightsPerTileBuffer = m_pNumSpotLightsPerTileBuffer;

	m_pTiledLightCullingPass->Record(&tiledLightCullingParams);
	submissionBatch.emplace_back(tiledLightCullingParams.m_pCommandList);
	
	CommandList* pUpdateCreateRenderShadowMapCommandsArgumentBufferCommandList = m_pCommandListPool->Create(L"pUpdateCreateRenderShadowMapCommandsArgumentBufferCommandList");
	pUpdateCreateRenderShadowMapCommandsArgumentBufferCommandList->Begin();
	pUpdateCreateRenderShadowMapCommandsArgumentBufferCommandList->SetRequiredResourceStates(&m_pCreateRenderShadowMapCommandsArgumentBufferResources->m_RequiredResourceStates);
	pUpdateCreateRenderShadowMapCommandsArgumentBufferCommandList->CopyBufferRegion(m_pCreateRenderShadowMapCommandsArgumentBuffer, 0, m_pNumVisibleMeshesBuffer, 0, sizeof(u32));
	pUpdateCreateRenderShadowMapCommandsArgumentBufferCommandList->End();

	submissionBatch.emplace_back(pUpdateCreateRenderShadowMapCommandsArgumentBufferCommandList);

	CreateRenderShadowMapCommandsPass::RenderParams createRenderShadowMapCommandsParams;
	createRenderShadowMapCommandsParams.m_pRenderEnv = m_pRenderEnv;
	createRenderShadowMapCommandsParams.m_pCommandList = m_pCommandListPool->Create(L"pCreateRenderShadowMapCommandsCommandList");
	createRenderShadowMapCommandsParams.m_pResources = m_pCreateRenderShadowMapCommandsResources;
	createRenderShadowMapCommandsParams.m_pIndirectArgumentBuffer = m_pCreateRenderShadowMapCommandsArgumentBuffer;
	createRenderShadowMapCommandsParams.m_pNumShadowCastingPointLightsBuffer = m_pNumShadowCastingPointLightsBuffer;
	createRenderShadowMapCommandsParams.m_pNumDrawPointLightShadowCastersBuffer = m_pNumDrawPointLightShadowCastersBuffer;
	createRenderShadowMapCommandsParams.m_pNumShadowCastingSpotLightsBuffer = m_pNumShadowCastingSpotLightsBuffer;
	createRenderShadowMapCommandsParams.m_pNumDrawSpotLightShadowCastersBuffer = m_pNumDrawSpotLightShadowCastersBuffer;

	m_pCreateRenderShadowMapCommandsPass->Record(&createRenderShadowMapCommandsParams);
	submissionBatch.emplace_back(createRenderShadowMapCommandsParams.m_pCommandList);

	if (m_pPointLightBuffer != nullptr)
	{
	}
	
	if (m_pSpotLightBuffer != nullptr)
	{
		SetupTiledShadowMapPass::RenderParams setupTiledShadowMapParams;
		setupTiledShadowMapParams.m_pRenderEnv = m_pRenderEnv;
		setupTiledShadowMapParams.m_pCommandList = m_pCommandListPool->Create(L"pSetupSpotLightTiledShadowMapCommandList");
		setupTiledShadowMapParams.m_pResources = m_pSetupSpotLightTiledShadowMapResources;

		m_pSetupSpotLightTiledShadowMapPass->Record(&setupTiledShadowMapParams);
		submissionBatch.emplace_back(setupTiledShadowMapParams.m_pCommandList);

		RenderTiledShadowMapPass::RenderParams renderTiledShadowMapParams;
		renderTiledShadowMapParams.m_pRenderEnv = m_pRenderEnv;
		renderTiledShadowMapParams.m_pCommandList = m_pCommandListPool->Create(L"pRenderSpotLightTiledShadowMapCommandList");
		renderTiledShadowMapParams.m_pResources = m_pRenderSpotLightTiledShadowMapResources;
		renderTiledShadowMapParams.m_pViewport = m_pSpotLightTiledShadowMapViewport;
		renderTiledShadowMapParams.m_pMeshBatch = m_pMeshBatch;
		renderTiledShadowMapParams.m_pDrawShadowCasterCommandBuffer = m_pDrawSpotLightShadowCasterCommandBuffer;
		renderTiledShadowMapParams.m_pNumDrawShadowCastersBuffer = m_pNumDrawSpotLightShadowCastersBuffer;

		m_pRenderSpotLightTiledShadowMapPass->Record(&renderTiledShadowMapParams);
		submissionBatch.emplace_back(renderTiledShadowMapParams.m_pCommandList);
	}

	TiledShadingPass::RenderParams tiledShadingParams;
	tiledShadingParams.m_pRenderEnv = m_pRenderEnv;
	tiledShadingParams.m_pCommandList = m_pCommandListPool->Create(L"pTiledShadingCommandList");
	tiledShadingParams.m_pResources = m_pTiledShadingResources;
	tiledShadingParams.m_pAccumLightTexture = m_pAccumLightTexture;
	
	m_pTiledShadingPass->Record(&tiledShadingParams);
	submissionBatch.emplace_back(tiledShadingParams.m_pCommandList);
	
#ifdef ENABLE_INDIRECT_LIGHTING
	ClearVoxelGridPass::RenderParams clearGridParams;
	clearGridParams.m_pRenderEnv = m_pRenderEnv;
	clearGridParams.m_pCommandList = m_pCommandListPool->Create(L"pClearGridCommandList");
	clearGridParams.m_pResources = m_pClearVoxelGridResources;
	
	m_pClearVoxelGridPass->Record(&clearGridParams);
	submissionBatch.emplace_back(clearGridParams.m_pCommandList);

	CreateVoxelGridPass::RenderParams createGridParams;
	createGridParams.m_pRenderEnv = m_pRenderEnv;
	createGridParams.m_pCommandList = m_pCommandListPool->Create(L"pCreateGridCommandList");
	createGridParams.m_pResources = m_pCreateVoxelGridResources;
	createGridParams.m_pBackBufferViewport = m_pBackBufferViewport;
	createGridParams.m_pMeshBatch = m_pMeshBatch;
	createGridParams.m_pDrawMeshCommandBuffer = m_pDrawMeshCommandBuffer;
	createGridParams.m_pNumDrawMeshesBuffer = m_pNumVisibleMeshesBuffer;
	
	m_pCreateVoxelGridPass->Record(&createGridParams);
	submissionBatch.emplace_back(createGridParams.m_pCommandList);
	
	VisualizeVoxelGridPass::RenderParams visualizeGridParams;
	visualizeGridParams.m_pRenderEnv = m_pRenderEnv;
	visualizeGridParams.m_pCommandList = m_pCommandListPool->Create(L"pVisualizeGridCommandList");
	visualizeGridParams.m_pResources = m_VisualizeVoxelGridResources[m_BackBufferIndex];
	visualizeGridParams.m_pBackBufferViewport = m_pBackBufferViewport;
	
	m_pVisualizeVoxelGridPass->Record(&visualizeGridParams);
	submissionBatch.emplace_back(visualizeGridParams.m_pCommandList);
#endif // ENABLE_INDIRECT_LIGHTING

	VisualizeTexturePass::RenderParams visualizeTextureParams;
	visualizeTextureParams.m_pRenderEnv = m_pRenderEnv;
	visualizeTextureParams.m_pCommandList = m_pCommandListPool->Create(L"pVisualizeTextureCommandList");
	visualizeTextureParams.m_pResources = m_VisualizeTextureResources[m_BackBufferIndex];
	visualizeTextureParams.m_pViewport = m_pBackBufferViewport;

	m_pVisualizeTexturePass->Record(&visualizeTextureParams);
	submissionBatch.emplace_back(visualizeTextureParams.m_pCommandList);
	
	ResourceTransitionBarrier presentBarrier(pRenderTarget, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
	CommandList* pPresentBarrierCommandList = m_pCommandListPool->Create(L"pPresentBarrierCommandList");
	pPresentBarrierCommandList->Begin();
	pPresentBarrierCommandList->ResourceBarrier(1, &presentBarrier);
	pPresentBarrierCommandList->End();
	submissionBatch.emplace_back(pPresentBarrierCommandList);

#ifdef FOR_DEBUG_ONLY
	CommandList* pDebugCommandList = m_pCommandListPool->Create(L"pDebugCommandList");
	
	pDebugCommandList->Begin();
	pDebugCommandList->SetRequiredResourceStates(&m_pDebugResources->m_RequiredResourceStates);
	pDebugCommandList->CopyResource(m_pDebugNumVisibleMeshesBuffer, m_pNumVisibleMeshesBuffer);
	pDebugCommandList->CopyResource(m_pDebugVisibleMeshIndexBuffer, m_pVisibleMeshIndexBuffer);
	pDebugCommandList->CopyResource(m_pDebugShadowCastingSpotLightIndexBuffer, m_pShadowCastingSpotLightIndexBuffer);
	pDebugCommandList->CopyResource(m_pDebugNumShadowCastingSpotLightsBuffer, m_pNumShadowCastingSpotLightsBuffer);
	pDebugCommandList->CopyResource(m_pDebugDrawSpotLightShadowCasterCommandBuffer, m_pDrawSpotLightShadowCasterCommandBuffer);
	pDebugCommandList->CopyResource(m_pDebugNumDrawSpotLightShadowCastersBuffer, m_pNumDrawSpotLightShadowCastersBuffer);
	pDebugCommandList->CopyResource(m_pDebugSpotLightShadowMapTileBuffer, m_pSpotLightShadowMapTileBuffer);
	pDebugCommandList->CopyResource(m_pDebugSpotLightViewTileProjMatrixBuffer, m_pSpotLightViewTileProjMatrixBuffer);
	pDebugCommandList->CopyResource(m_pDebugSpotLightShadowMapTileBuffer, m_pSpotLightShadowMapTileBuffer);
	pDebugCommandList->End();

	submissionBatch.emplace_back(pDebugCommandList);
#endif // FOR_DEBUG_ONLY

	++m_LastSubmissionFenceValue;
	m_pCommandQueue->ExecuteCommandLists(m_pRenderEnv, submissionBatch.size(), submissionBatch.data(), m_pFence, m_LastSubmissionFenceValue);
	pRenderTarget->SetState(D3D12_RESOURCE_STATE_PRESENT);

#ifdef FOR_DEBUG_ONLY
	m_pFence->WaitForSignalOnCPU(m_LastSubmissionFenceValue);
	OutputDebugStringA("1.Debug =========================\n");

	u32 debugNumVisibleMeshes = 0;
	m_pDebugNumVisibleMeshesBuffer->Read(&debugNumVisibleMeshes, sizeof(u32));
	OutputDebugStringA(("NumVisibleMeshes: " + std::to_string(debugNumVisibleMeshes)).c_str());
	OutputDebugStringA("\n");

	std::vector<u32> debugVisibleMeshIndices(debugNumVisibleMeshes);
	m_pDebugVisibleMeshIndexBuffer->Read(debugVisibleMeshIndices.data(), debugNumVisibleMeshes * sizeof(u32));
	for (u32 i = 0; i < debugNumVisibleMeshes; ++i)
		OutputDebugStringA(("\t" + std::to_string(i) + ".mesh index: " + std::to_string(debugVisibleMeshIndices[i]) + "\n").c_str());
	OutputDebugStringA("\n");

	u32 debugNumShadowCastingSpotLights = 0;
	m_pDebugNumShadowCastingSpotLightsBuffer->Read(&debugNumShadowCastingSpotLights, sizeof(u32));
	OutputDebugStringA(("NumShadowCastingSpotLights: " + std::to_string(debugNumShadowCastingSpotLights)).c_str());
	OutputDebugStringA("\n");

	std::vector<u32> debugShadowCastingSpotLightIndices(debugNumShadowCastingSpotLights);
	m_pDebugShadowCastingSpotLightIndexBuffer->Read(debugShadowCastingSpotLightIndices.data(), debugNumShadowCastingSpotLights * sizeof(u32));
	for (u32 i = 0; i < debugNumShadowCastingSpotLights; ++i)
		OutputDebugStringA(("\t" + std::to_string(i) + ".light index: " + std::to_string(debugShadowCastingSpotLightIndices[i]) + "\n").c_str());
	OutputDebugStringA("\n");
	
	u32 debugNumDrawSpotLightShadowCasters = 0;
	m_pDebugNumDrawSpotLightShadowCastersBuffer->Read(&debugNumDrawSpotLightShadowCasters, sizeof(u32));
	OutputDebugStringA(("NumDrawSpotLightShadowCasters: " + std::to_string(debugNumDrawSpotLightShadowCasters)).c_str());
	OutputDebugStringA("\n");

	std::vector<DrawMeshCommand> debugDrawSpotLightShadowCasterCommands(debugNumDrawSpotLightShadowCasters);
	m_pDebugDrawSpotLightShadowCasterCommandBuffer->Read(debugDrawSpotLightShadowCasterCommands.data(), debugNumDrawSpotLightShadowCasters * sizeof(DrawMeshCommand));
	for (u32 i = 0; i < debugNumDrawSpotLightShadowCasters; ++i)
	{
		const DrawMeshCommand& drawCommand = debugDrawSpotLightShadowCasterCommands[i];
		OutputDebugStringA((std::to_string(i) + ".Command:\n").c_str());
		OutputDebugStringA(("\tlightIndexStart: " + std::to_string(drawCommand.m_Root32BitConstant) + "\n").c_str());
		OutputDebugStringA(("\tIndexCountPerInstance: " + std::to_string(drawCommand.m_DrawArgs.IndexCountPerInstance) + "\n").c_str());
		OutputDebugStringA(("\tinstanceCount: " + std::to_string(drawCommand.m_DrawArgs.InstanceCount) + "\n").c_str());
		OutputDebugStringA(("\tstartIndexLocation: " + std::to_string(drawCommand.m_DrawArgs.StartIndexLocation) + "\n").c_str());
		OutputDebugStringA(("\tbaseVertexLocation: " + std::to_string(drawCommand.m_DrawArgs.BaseVertexLocation) + "\n").c_str());
		OutputDebugStringA(("\tstartInstanceLocation: " + std::to_string(drawCommand.m_DrawArgs.StartInstanceLocation) + "\n").c_str());
	}

	u16 debugNumSpotLightShadowMapTiles = kNumShadowMapTiles * kNumShadowMapTiles;
	std::vector<ShadowMapTile> debugSpotLightShadowMapTiles(debugNumSpotLightShadowMapTiles);
	m_pDebugSpotLightShadowMapTileBuffer->Read(debugSpotLightShadowMapTiles.data(), debugNumSpotLightShadowMapTiles * sizeof(ShadowMapTile));

	for (u16 row = 0; row < kNumShadowMapTiles; ++row)
	{
		for (u16 col = 0; col < kNumShadowMapTiles; ++col)
		{
			const ShadowMapTile& tile = debugSpotLightShadowMapTiles[row * kNumShadowMapTiles + col];
			OutputDebugStringA(("Tile (" + std::to_string(row) + ", " + std::to_string(col) + "):\n").c_str());
			OutputDebugStringA(("\ttexSpaceTopLeftPos: (" + std::to_string(tile.m_TexSpaceTopLeftPos.m_X) + ", " + std::to_string(tile.m_TexSpaceTopLeftPos.m_Y) + ")\n").c_str());
			OutputDebugStringA(("\ttexSpaceSize: (" + std::to_string(tile.m_TexSpaceSize.m_X) + ", " + std::to_string(tile.m_TexSpaceSize.m_Y) + ")\n").c_str());
		}
	}

	OutputDebugStringA("2.Debug =========================\n");
#endif // FOR_DEBUG_ONLY

	++m_LastSubmissionFenceValue;
	m_pSwapChain->Present(1, 0);
	m_pCommandQueue->Signal(m_pFence, m_LastSubmissionFenceValue);

	m_FrameCompletionFenceValues[m_BackBufferIndex] = m_LastSubmissionFenceValue;
	m_BackBufferIndex = m_pSwapChain->GetCurrentBackBufferIndex();
	m_pFence->WaitForSignalOnCPU(m_FrameCompletionFenceValues[m_BackBufferIndex]);
}

void DXApplication::OnDestroy()
{
	m_pCommandQueue->Signal(m_pFence, m_LastSubmissionFenceValue);
	m_pFence->WaitForSignalOnCPU(m_LastSubmissionFenceValue);
}

void DXApplication::OnKeyDown(UINT8 key)
{
}

void DXApplication::OnKeyUp(UINT8 key)
{
}
