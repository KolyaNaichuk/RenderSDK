#include "DXApplication.h"
#include "D3DWrapper/D3DFactory.h"
#include "D3DWrapper/D3DDevice.h"
#include "D3DWrapper/D3DSwapChain.h"
#include "D3DWrapper/D3DCommandQueue.h"
#include "D3DWrapper/D3DCommandAllocator.h"
#include "D3DWrapper/D3DCommandList.h"
#include "D3DWrapper/D3DDescriptorHeap.h"
#include "D3DWrapper/D3DResource.h"
#include "D3DWrapper/D3DFence.h"
#include "D3DWrapper/D3DRenderEnv.h"
#include "D3DWrapper/D3DUtils.h"
#include "D3DWrapper/D3DResourceList.h"
#include "D3DWrapper/D3DCommandSignature.h"
#include "RenderPasses/RenderGBufferPass.h"
#include "RenderPasses/TiledLightCullingPass.h"
#include "RenderPasses/TiledShadingPass.h"
#include "RenderPasses/ClearVoxelGridPass.h"
#include "RenderPasses/CreateVoxelGridPass.h"
#include "RenderPasses/InjectVPLsIntoVoxelGridPass.h"
#include "RenderPasses/VisualizeVoxelGridPass.h"
#include "RenderPasses/VisualizeMeshPass.h"
#include "RenderPasses/ViewFrustumCullingPass.h"
#include "RenderPasses/RenderGBufferCommandsPass.h"
#include "RenderPasses/RenderShadowMapCommandsPass.h"
#include "RenderPasses/RenderTiledShadowMapPass.h"
#include "RenderPasses/CopyTexturePass.h"
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
2.Generate render G-Buffer indirect draw commands (GPU side)
3.Render G-Buffer with visible meshes (GPU side)

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

enum
{
	kTileSize = 16,
	kNumTilesX = 58,
	kNumTilesY = 48,

	kGridSizeX = 640,
	kGridSizeY = 640,
	kGridSizeZ = 640,
		
	kNumGridCellsX = 64,
	kNumGridCellsY = 64,
	kNumGridCellsZ = 64,

	kTiledShadowMapSize = 4 * 1024
};

struct ObjectTransform
{
	Matrix4f m_WorldPosMatrix;
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
	u32 m_NumMeshes;
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

DXApplication::DXApplication(HINSTANCE hApp)
	: Application(hApp, L"Scene Voxelization", 0, 0, kTileSize * kNumTilesX, kTileSize * kNumTilesY)
	, m_pDevice(nullptr)
	, m_pSwapChain(nullptr)
	, m_pCommandQueue(nullptr)
	, m_pCommandList(nullptr)
	, m_pCommandListPool(nullptr)
	, m_pDefaultHeapProps(new D3DHeapProperties(D3D12_HEAP_TYPE_DEFAULT))
	, m_pUploadHeapProps(new D3DHeapProperties(D3D12_HEAP_TYPE_UPLOAD))
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
	, m_pViewport(nullptr)
	, m_pObjectTransformBuffer(nullptr)
	, m_pCameraTransformBuffer(nullptr)
	, m_pGridBuffer(nullptr)
	, m_pGridConfigBuffer(nullptr)
	, m_pViewFrustumCullingDataBuffer(nullptr)
	, m_pTiledLightCullingDataBuffer(nullptr)
	, m_pTiledShadingDataBuffer(nullptr)
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
	, m_pRenderEnv(new D3DRenderEnv())
	, m_pFence(nullptr)
	, m_BackBufferIndex(0)
	, m_pRenderGBufferPass(nullptr)
	, m_pRenderGBufferResources(new D3DResourceList())
	, m_pTiledLightCullingPass(nullptr)
	, m_pTiledLightCullingResources(new D3DResourceList())
	, m_pTiledShadingPass(nullptr)
	, m_pTiledShadingResources(new D3DResourceList())
	, m_pClearVoxelGridPass(nullptr)
	, m_pClearVoxelGridResources(new D3DResourceList())
	, m_pCreateVoxelGridPass(nullptr)
	, m_pCreateVoxelGridResources(new D3DResourceList())
	, m_pInjectVPLsIntoVoxelGridPass(nullptr)
	, m_pVisualizeVoxelGridPass(nullptr)
	, m_pVisualizeMeshPass(nullptr)
	, m_pDetectVisibleMeshesPass(nullptr)
	, m_pDetectVisibleMeshesResources(new D3DResourceList())
	, m_pDetectVisiblePointLightsPass(nullptr)
	, m_pDetectVisiblePointLightsResources(new D3DResourceList())
	, m_pDetectVisibleSpotLightsPass(nullptr)
	, m_pDetectVisibleSpotLightsResources(new D3DResourceList())
	, m_pRenderGBufferCommandsPass(nullptr)
	, m_pRenderGBufferCommandsResources(new D3DResourceList())
	, m_pRenderShadowMapCommandsPass(nullptr)
	, m_pRenderShadowMapCommandsResources(new D3DResourceList())
	, m_pRenderShadowMapCommandsArgumentBuffer(nullptr)
	, m_pRenderSpotLightTiledShadowMapPass(nullptr)
	, m_pRenderSpotLightTiledShadowMapResources(new D3DResourceList())
	, m_pCopyTexturePass(nullptr)
	, m_pMeshBatch(nullptr)
	, m_pPointLightBuffer(nullptr)
	, m_pNumVisiblePointLightsBuffer(nullptr)
	, m_pVisiblePointLightIndexBuffer(nullptr)
	, m_pSpotLightBuffer(nullptr)
	, m_pNumVisibleSpotLightsBuffer(nullptr)
	, m_pVisibleSpotLightIndexBuffer(nullptr)
	, m_pCamera(nullptr)
{
	for (u8 index = 0; index < kBackBufferCount; ++index)
	{
		m_FenceValues[index] = 0;
		m_CommandAllocators[index] = nullptr;
		m_VisualizeMeshResources[index] = new D3DResourceList();
		m_VisualizeVoxelGridResources[index] = new D3DResourceList();
		m_CopyTextureResources[index] = new D3DResourceList();
	}
}

DXApplication::~DXApplication()
{
	for (u8 index = 0; index < kBackBufferCount; ++index)
	{
		SafeDelete(m_CommandAllocators[index]);
		SafeDelete(m_VisualizeMeshResources[index]);
		SafeDelete(m_VisualizeVoxelGridResources[index]);
		SafeDelete(m_CopyTextureResources[index]);
	}

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
	SafeDelete(m_pCopyTexturePass);
	SafeDelete(m_pClearVoxelGridPass);
	SafeDelete(m_pClearVoxelGridResources);
	SafeDelete(m_pCreateVoxelGridPass);
	SafeDelete(m_pCreateVoxelGridResources);
	SafeDelete(m_pInjectVPLsIntoVoxelGridPass);
	SafeDelete(m_pVisualizeVoxelGridPass);
	SafeDelete(m_pVisualizeMeshPass);
	SafeDelete(m_pTiledLightCullingPass);
	SafeDelete(m_pTiledLightCullingResources);
	SafeDelete(m_pTiledShadingPass);
	SafeDelete(m_pTiledShadingResources);
	SafeDelete(m_pRenderGBufferPass);
	SafeDelete(m_pRenderGBufferResources);
	SafeDelete(m_pRenderGBufferCommandsPass);
	SafeDelete(m_pRenderGBufferCommandsResources);
	SafeDelete(m_pRenderShadowMapCommandsPass);
	SafeDelete(m_pRenderShadowMapCommandsResources);
	SafeDelete(m_pRenderShadowMapCommandsArgumentBuffer);
	SafeDelete(m_pRenderSpotLightTiledShadowMapPass);
	SafeDelete(m_pRenderSpotLightTiledShadowMapResources);
	SafeDelete(m_pDetectVisibleMeshesPass);
	SafeDelete(m_pDetectVisibleMeshesResources);
	SafeDelete(m_pDetectVisiblePointLightsPass);
	SafeDelete(m_pDetectVisiblePointLightsResources);
	SafeDelete(m_pDetectVisibleSpotLightsPass);
	SafeDelete(m_pDetectVisibleSpotLightsResources);
	SafeDelete(m_pFence);
	SafeDelete(m_pDefaultHeapProps);
	SafeDelete(m_pUploadHeapProps);
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
	SafeDelete(m_pViewFrustumCullingDataBuffer);
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
	SafeDelete(m_pCommandList);
	SafeDelete(m_pCommandListPool);
	SafeDelete(m_pSwapChain);
	SafeDelete(m_pDevice);
	SafeDelete(m_pViewport);
}

void DXApplication::OnInit()
{
	D3DFactory factory;
	m_pDevice = new D3DDevice(&factory, D3D_FEATURE_LEVEL_12_0, true);

	D3D12_FEATURE_DATA_D3D12_OPTIONS supportedOptions;
	m_pDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &supportedOptions, sizeof(supportedOptions));
	//assert(supportedOptions.ConservativeRasterizationTier != D3D12_CONSERVATIVE_RASTERIZATION_TIER_NOT_SUPPORTED);
	assert(supportedOptions.ROVsSupported == TRUE);

	D3DCommandQueueDesc commandQueueDesc(D3D12_COMMAND_LIST_TYPE_DIRECT);
	m_pCommandQueue = new D3DCommandQueue(m_pDevice, &commandQueueDesc, L"m_pCommandQueue");

	D3DDescriptorHeapDesc rtvHeapDesc(D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 9, false);
	m_pShaderInvisibleRTVHeap = new D3DDescriptorHeap(m_pDevice, &rtvHeapDesc, L"m_pShaderInvisibleRTVHeap");

	D3DDescriptorHeapDesc shaderVisibleSRVHeapDesc(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 70, true);
	m_pShaderVisibleSRVHeap = new D3DDescriptorHeap(m_pDevice, &shaderVisibleSRVHeapDesc, L"m_pShaderVisibleSRVHeap");

	D3DDescriptorHeapDesc shaderInvisibleSRVHeapDesc(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 60, false);
	m_pShaderInvisibleSRVHeap = new D3DDescriptorHeap(m_pDevice, &shaderInvisibleSRVHeapDesc, L"m_pShaderInvisibleSRVHeap");

	D3DDescriptorHeapDesc dsvHeapDesc(D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 2, false);
	m_pShaderInvisibleDSVHeap = new D3DDescriptorHeap(m_pDevice, &dsvHeapDesc, L"m_pShaderInvisibleDSVHeap");

	for (UINT index = 0; index < kBackBufferCount; ++index)
		m_CommandAllocators[index] = new D3DCommandAllocator(m_pDevice, D3D12_COMMAND_LIST_TYPE_DIRECT, L"m_CommandAllocators");

	m_pCommandListPool = new D3DCommandListPool(m_pDevice, D3D12_COMMAND_LIST_TYPE_DIRECT);
	m_pCommandList = new D3DCommandList(m_pDevice, m_CommandAllocators[m_BackBufferIndex], nullptr, L"m_pCommandList");

	m_pRenderEnv->m_pDevice = m_pDevice;
	m_pRenderEnv->m_pCommandListPool = m_pCommandListPool;
	m_pRenderEnv->m_pDefaultHeapProps = m_pDefaultHeapProps;
	m_pRenderEnv->m_pUploadHeapProps = m_pUploadHeapProps;
	m_pRenderEnv->m_pShaderInvisibleRTVHeap = m_pShaderInvisibleRTVHeap;
	m_pRenderEnv->m_pShaderInvisibleSRVHeap = m_pShaderInvisibleSRVHeap;
	m_pRenderEnv->m_pShaderInvisibleDSVHeap = m_pShaderInvisibleDSVHeap;
	m_pRenderEnv->m_pShaderVisibleSRVHeap = m_pShaderVisibleSRVHeap;
	
	const RECT bufferRect = m_pWindow->GetClientRect();
	const UINT bufferWidth = bufferRect.right - bufferRect.left;
	const UINT bufferHeight = bufferRect.bottom - bufferRect.top;

	m_pViewport = new D3DViewport(0.0f, 0.0f, (FLOAT)bufferWidth, (FLOAT)bufferHeight);

	m_pCamera = new Camera(Camera::ProjType_Perspective, 0.1f, 1300.0f, FLOAT(bufferWidth) / FLOAT(bufferHeight));
	m_pCamera->SetClearFlags(Camera::ClearFlag_Color | Camera::ClearFlag_Depth);
	m_pCamera->SetBackgroundColor(Color::GRAY);

	Transform& transform = m_pCamera->GetTransform();
	transform.SetPosition(Vector3f(278.0f, 274.0f, 700.0f));
	transform.SetRotation(CreateRotationYQuaternion(Radian(PI)));

	D3DSwapChainDesc swapChainDesc(kBackBufferCount, m_pWindow->GetHWND(), bufferWidth, bufferHeight);
	m_pSwapChain = new D3DSwapChain(&factory, m_pRenderEnv, &swapChainDesc, m_pCommandQueue);
	m_BackBufferIndex = m_pSwapChain->GetCurrentBackBufferIndex();
	
	D3DDepthStencilValue optimizedClearDepth(1.0f);

	D3DDepthTexture2DDesc depthTexDesc(DXGI_FORMAT_R32_TYPELESS, bufferWidth, bufferHeight, true, true);
	m_pDepthTexture = new D3DDepthTexture(m_pRenderEnv, m_pRenderEnv->m_pDefaultHeapProps, &depthTexDesc, D3D12_RESOURCE_STATE_DEPTH_WRITE, &optimizedClearDepth, L"m_pDepthTexture");
	
	D3DDepthTexture2DDesc tiledShadowMapDesc(DXGI_FORMAT_R32_TYPELESS, kTiledShadowMapSize, kTiledShadowMapSize, true, true);
	m_pSpotLightTiledShadowMap = new D3DDepthTexture(m_pRenderEnv, m_pRenderEnv->m_pDefaultHeapProps, &tiledShadowMapDesc, D3D12_RESOURCE_STATE_DEPTH_WRITE, &optimizedClearDepth, L"m_pSpotLightTiledShadowMap");

	const FLOAT optimizedClearColor[4] = {0.0f, 0.0f, 0.0f, 0.0f};
	D3DColorTexture2DDesc diffuseTexDesc(DXGI_FORMAT_R10G10B10A2_UNORM, bufferWidth, bufferHeight, true, true, false);
	m_pDiffuseTexture = new D3DColorTexture(m_pRenderEnv, m_pRenderEnv->m_pDefaultHeapProps, &diffuseTexDesc, D3D12_RESOURCE_STATE_RENDER_TARGET, optimizedClearColor, L"m_pDiffuseTexture");

	D3DColorTexture2DDesc normalTexDesc(DXGI_FORMAT_R8G8B8A8_SNORM, bufferWidth, bufferHeight, true, true, false);
	m_pNormalTexture = new D3DColorTexture(m_pRenderEnv, m_pRenderEnv->m_pDefaultHeapProps, &normalTexDesc, D3D12_RESOURCE_STATE_RENDER_TARGET, optimizedClearColor, L"m_pNormalTexture");

	D3DColorTexture2DDesc specularTexDesc(DXGI_FORMAT_R8G8B8A8_UNORM, bufferWidth, bufferHeight, true, true, false);
	m_pSpecularTexture = new D3DColorTexture(m_pRenderEnv, m_pRenderEnv->m_pDefaultHeapProps, &specularTexDesc, D3D12_RESOURCE_STATE_RENDER_TARGET, optimizedClearColor, L"m_pSpecularTexture");

	D3DColorTexture2DDesc accumLightTexDesc(DXGI_FORMAT_R10G10B10A2_UNORM, bufferWidth, bufferHeight, false, true, true);
	m_pAccumLightTexture = new D3DColorTexture(m_pRenderEnv, m_pRenderEnv->m_pDefaultHeapProps, &accumLightTexDesc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, optimizedClearColor, L"m_pAccumLightTexture");

	D3DConstantBufferDesc objectTransformBufferDesc(sizeof(ObjectTransform));
	m_pObjectTransformBuffer = new D3DBuffer(m_pRenderEnv, m_pRenderEnv->m_pUploadHeapProps, &objectTransformBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, L"m_pObjectTransformBuffer");

	D3DConstantBufferDesc cameraTransformBufferDesc(sizeof(CameraTransform));
	m_pCameraTransformBuffer = new D3DBuffer(m_pRenderEnv, m_pRenderEnv->m_pUploadHeapProps, &cameraTransformBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, L"m_pCameraTransformBuffer");

	D3DConstantBufferDesc gridConfigBufferDesc(sizeof(GridConfig));
	m_pGridConfigBuffer = new D3DBuffer(m_pRenderEnv, m_pRenderEnv->m_pUploadHeapProps, &gridConfigBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, L"m_pGridConfigBuffer");

	D3DConstantBufferDesc viewFrustumCullingDataBufferDesc(sizeof(ViewFrustumCullingData));
	m_pViewFrustumCullingDataBuffer = new D3DBuffer(m_pRenderEnv, m_pRenderEnv->m_pUploadHeapProps, &viewFrustumCullingDataBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, L"m_pViewFrustumCullingDataBuffer");

	D3DConstantBufferDesc tiledLightCullingDataBufferDesc(sizeof(TiledLightCullingData));
	m_pTiledLightCullingDataBuffer = new D3DBuffer(m_pRenderEnv, m_pRenderEnv->m_pUploadHeapProps, &tiledLightCullingDataBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, L"m_pTiledLightCullingDataBuffer");

	D3DConstantBufferDesc tiledShadingDataBufferDesc(sizeof(TiledShadingData));
	m_pTiledShadingDataBuffer = new D3DBuffer(m_pRenderEnv, m_pRenderEnv->m_pUploadHeapProps, &tiledShadingDataBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, L"m_pTiledShadingDataBuffer");

	const u32 numGridElements = kNumGridCellsX * kNumGridCellsY * kNumGridCellsZ;
	D3DStructuredBufferDesc gridBufferDesc(numGridElements, sizeof(Voxel), true, true);
	m_pGridBuffer = new D3DBuffer(m_pRenderEnv, m_pRenderEnv->m_pDefaultHeapProps, &gridBufferDesc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, L"m_pGridBuffer");

	m_pFence = new D3DFence(m_pDevice, m_FenceValues[m_BackBufferIndex]);
	++m_FenceValues[m_BackBufferIndex];

	Scene* pScene = SceneLoader::LoadCornellBox(CornellBoxSettings_Test1);
	
	assert(pScene->GetNumMeshBatches() == 1);
	MeshBatchData* pMeshBatchData = *pScene->GetMeshBatches();
	
	D3DStructuredBufferDesc drawCommandBufferDesc(pMeshBatchData->GetNumMeshes(), sizeof(DrawMeshCommand), true, true);
	m_pDrawMeshCommandBuffer = new D3DBuffer(m_pRenderEnv, m_pRenderEnv->m_pDefaultHeapProps, &drawCommandBufferDesc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, L"m_pDrawMeshCommandBuffer");
	
	m_pMeshBatch = new MeshBatch(m_pRenderEnv, pMeshBatchData);
	m_pMeshBatch->RecordDataForUpload(m_pCommandList);

	if (pScene->GetNumPointLights() > 0)
	{
		m_pPointLightBuffer = new LightBuffer(m_pRenderEnv, pScene->GetNumPointLights(), pScene->GetPointLights());
		m_pPointLightBuffer->RecordDataForUpload(m_pCommandList);

		D3DFormattedBufferDesc numVisibleLightsBufferDesc(1, DXGI_FORMAT_R32_UINT, true, true);
		m_pNumVisiblePointLightsBuffer = new D3DBuffer(m_pRenderEnv, m_pRenderEnv->m_pDefaultHeapProps, &numVisibleLightsBufferDesc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, L"m_pNumVisiblePointLightsBuffer");

		D3DFormattedBufferDesc visibleLightIndexBufferDesc(pScene->GetNumPointLights(), DXGI_FORMAT_R32_UINT, true, true);
		m_pVisiblePointLightIndexBuffer = new D3DBuffer(m_pRenderEnv, m_pRenderEnv->m_pDefaultHeapProps, &visibleLightIndexBufferDesc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, L"m_pVisiblePointLightIndexBuffer");

		D3DFormattedBufferDesc numLightsPerTileBufferDesc(1, DXGI_FORMAT_R32_UINT, true, true);
		m_pNumPointLightsPerTileBuffer = new D3DBuffer(m_pRenderEnv, m_pRenderEnv->m_pDefaultHeapProps, &numLightsPerTileBufferDesc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, L"m_pNumPointLightsPerTileBuffer");

		D3DFormattedBufferDesc lightIndexPerTileBufferDesc(kNumTilesX * kNumTilesY * pScene->GetNumPointLights(), DXGI_FORMAT_R32_UINT, true, true);
		m_pPointLightIndexPerTileBuffer = new D3DBuffer(m_pRenderEnv, m_pRenderEnv->m_pDefaultHeapProps, &lightIndexPerTileBufferDesc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, L"m_pPointLightIndexPerTileBuffer");

		D3DStructuredBufferDesc lightRangePerTileBufferDesc(kNumTilesX * kNumTilesY, sizeof(Range), true, true);
		m_pPointLightRangePerTileBuffer = new D3DBuffer(m_pRenderEnv, m_pRenderEnv->m_pDefaultHeapProps, &lightRangePerTileBufferDesc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, L"m_pPointLightRangePerTileBuffer");
	}

	if (pScene->GetNumSpotLights() > 0)
	{
		m_pSpotLightBuffer = new LightBuffer(m_pRenderEnv, pScene->GetNumSpotLights(), pScene->GetSpotLights());
		m_pSpotLightBuffer->RecordDataForUpload(m_pCommandList);

		D3DFormattedBufferDesc numVisibleLightsBufferDesc(1, DXGI_FORMAT_R32_UINT, true, true);
		m_pNumVisibleSpotLightsBuffer = new D3DBuffer(m_pRenderEnv, m_pRenderEnv->m_pDefaultHeapProps, &numVisibleLightsBufferDesc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, L"m_pNumVisibleSpotLightsBuffer");

		D3DFormattedBufferDesc visibleLightIndexBufferDesc(pScene->GetNumSpotLights(), DXGI_FORMAT_R32_UINT, true, true);
		m_pVisibleSpotLightIndexBuffer = new D3DBuffer(m_pRenderEnv, m_pRenderEnv->m_pDefaultHeapProps, &visibleLightIndexBufferDesc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, L"m_pVisibleSpotLightIndexBuffer");

		D3DFormattedBufferDesc numLightsPerTileBufferDesc(1, DXGI_FORMAT_R32_UINT, true, true);
		m_pNumSpotLightsPerTileBuffer = new D3DBuffer(m_pRenderEnv, m_pRenderEnv->m_pDefaultHeapProps, &numLightsPerTileBufferDesc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, L"m_pNumSpotLightsPerTileBuffer");

		D3DFormattedBufferDesc lightIndexPerTileBufferDesc(kNumTilesX * kNumTilesY * pScene->GetNumSpotLights(), DXGI_FORMAT_R32_UINT, true, true);
		m_pSpotLightIndexPerTileBuffer = new D3DBuffer(m_pRenderEnv, m_pRenderEnv->m_pDefaultHeapProps, &lightIndexPerTileBufferDesc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, L"m_pSpotLightIndexPerTileBuffer");

		D3DStructuredBufferDesc lightRangePerTileBufferDesc(kNumTilesX * kNumTilesY, sizeof(Range), true, true);
		m_pSpotLightRangePerTileBuffer = new D3DBuffer(m_pRenderEnv, m_pRenderEnv->m_pDefaultHeapProps, &lightRangePerTileBufferDesc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, L"m_pSpotLightRangePerTileBuffer");
	}
	
	D3DStructuredBufferDesc shadowMapCommandsArgumentBufferDesc(1, sizeof(Vector3u), false, false);
	m_pRenderShadowMapCommandsArgumentBuffer = new D3DBuffer(m_pRenderEnv, m_pRenderEnv->m_pDefaultHeapProps, &shadowMapCommandsArgumentBufferDesc, D3D12_RESOURCE_STATE_COPY_DEST, L"m_pRenderShadowMapCommandsArgumentBuffer");

	Vector3u shadowMapCommandsArgumentBufferInitValues(0, 1, 1);
	D3DBuffer uploadRenderShadowMapCommandsArgumentBuffer(m_pRenderEnv, m_pRenderEnv->m_pUploadHeapProps, &shadowMapCommandsArgumentBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, L"m_pRenderShadowMapCommandsArgumentBuffer");
	uploadRenderShadowMapCommandsArgumentBuffer.Write(&shadowMapCommandsArgumentBufferInitValues, sizeof(Vector3u));

	m_pCommandList->CopyResource(m_pRenderShadowMapCommandsArgumentBuffer, &uploadRenderShadowMapCommandsArgumentBuffer);

	m_pCommandList->Close();
	m_pCommandQueue->ExecuteCommandLists(m_pRenderEnv, 1, &m_pCommandList, nullptr);
	WaitForGPU();

	m_pMeshBatch->RemoveDataForUpload();
	
	if (m_pPointLightBuffer != nullptr)
		m_pPointLightBuffer->RemoveDataForUpload();

	if (m_pSpotLightBuffer != nullptr)
		m_pSpotLightBuffer->RemoveDataForUpload();

	D3DFormattedBufferDesc numVisibleMeshesBufferDesc(1, DXGI_FORMAT_R32_UINT, true, true);
	m_pNumVisibleMeshesBuffer = new D3DBuffer(m_pRenderEnv, m_pRenderEnv->m_pDefaultHeapProps, &numVisibleMeshesBufferDesc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, L"m_pNumVisibleMeshesBuffer");

	D3DFormattedBufferDesc visibleMeshIndexBufferDesc(m_pMeshBatch->GetNumMeshes(), DXGI_FORMAT_R32_UINT, true, true);
	m_pVisibleMeshIndexBuffer = new D3DBuffer(m_pRenderEnv, m_pRenderEnv->m_pDefaultHeapProps, &visibleMeshIndexBufferDesc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, L"m_pVisibleMeshIndexBuffer");
	
	D3DBuffer* pMeshBoundsBuffer = m_pMeshBatch->GetMeshBoundsBuffer();
	D3DBuffer* pMeshDescBuffer = m_pMeshBatch->GetMeshDescBuffer();
	D3DBuffer* pMaterialBuffer = m_pMeshBatch->GetMaterialBuffer();

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
	m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), m_pViewFrustumCullingDataBuffer->GetCBVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	if (m_pPointLightBuffer != nullptr)
	{
		ViewFrustumCullingPass::InitParams detectVisibleLightsParams;
		detectVisibleLightsParams.m_pRenderEnv = m_pRenderEnv;
		detectVisibleLightsParams.m_ObjectBoundsType = ObjectBoundsType_Sphere;
		detectVisibleLightsParams.m_NumObjects = m_pPointLightBuffer->GetNumLights();

		m_pDetectVisiblePointLightsPass = new ViewFrustumCullingPass(&detectVisibleLightsParams);
		D3DBuffer* pLightBoundsBuffer = m_pPointLightBuffer->GetLightBoundsBuffer();
		
		m_pDetectVisiblePointLightsResources->m_RequiredResourceStates.emplace_back(m_pNumVisiblePointLightsBuffer, m_pNumVisiblePointLightsBuffer->GetWriteState());
		m_pDetectVisiblePointLightsResources->m_RequiredResourceStates.emplace_back(m_pVisiblePointLightIndexBuffer, m_pVisiblePointLightIndexBuffer->GetWriteState());
		m_pDetectVisiblePointLightsResources->m_RequiredResourceStates.emplace_back(pLightBoundsBuffer, pLightBoundsBuffer->GetReadState());

		m_pDetectVisiblePointLightsResources->m_SRVHeapStart = m_pShaderVisibleSRVHeap->Allocate();
		m_pDevice->CopyDescriptor(m_pDetectVisiblePointLightsResources->m_SRVHeapStart, m_pNumVisiblePointLightsBuffer->GetUAVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), m_pVisiblePointLightIndexBuffer->GetUAVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), pLightBoundsBuffer->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), m_pViewFrustumCullingDataBuffer->GetCBVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	}

	if (m_pSpotLightBuffer != nullptr)
	{
		ViewFrustumCullingPass::InitParams detectVisibleLightsParams;
		detectVisibleLightsParams.m_pRenderEnv = m_pRenderEnv;
		detectVisibleLightsParams.m_ObjectBoundsType = ObjectBoundsType_Sphere;
		detectVisibleLightsParams.m_NumObjects = m_pSpotLightBuffer->GetNumLights();

		m_pDetectVisibleSpotLightsPass = new ViewFrustumCullingPass(&detectVisibleLightsParams);
		D3DBuffer* pLightBoundsBuffer = m_pSpotLightBuffer->GetLightBoundsBuffer();

		m_pDetectVisibleSpotLightsResources->m_RequiredResourceStates.emplace_back(m_pNumVisibleSpotLightsBuffer, m_pNumVisibleSpotLightsBuffer->GetWriteState());
		m_pDetectVisibleSpotLightsResources->m_RequiredResourceStates.emplace_back(m_pVisibleSpotLightIndexBuffer, m_pVisibleSpotLightIndexBuffer->GetWriteState());
		m_pDetectVisibleSpotLightsResources->m_RequiredResourceStates.emplace_back(pLightBoundsBuffer, pLightBoundsBuffer->GetReadState());

		m_pDetectVisibleSpotLightsResources->m_SRVHeapStart = m_pShaderVisibleSRVHeap->Allocate();
		m_pDevice->CopyDescriptor(m_pDetectVisibleSpotLightsResources->m_SRVHeapStart, m_pNumVisibleSpotLightsBuffer->GetUAVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), m_pVisibleSpotLightIndexBuffer->GetUAVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), pLightBoundsBuffer->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), m_pViewFrustumCullingDataBuffer->GetCBVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
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
		D3DBuffer* pLightBoundsBuffer = m_pPointLightBuffer->GetLightBoundsBuffer();

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
		D3DBuffer* pLightBoundsBuffer = m_pSpotLightBuffer->GetLightBoundsBuffer();

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

	RenderGBufferCommandsPass::InitParams fillGBufferCommandsParams;
	fillGBufferCommandsParams.m_pRenderEnv = m_pRenderEnv;
	fillGBufferCommandsParams.m_NumMeshesInBatch = m_pMeshBatch->GetNumMeshes();
	
	m_pRenderGBufferCommandsPass = new RenderGBufferCommandsPass(&fillGBufferCommandsParams);

	m_pRenderGBufferCommandsResources->m_RequiredResourceStates.emplace_back(m_pNumVisibleMeshesBuffer, m_pNumVisibleMeshesBuffer->GetReadState());
	m_pRenderGBufferCommandsResources->m_RequiredResourceStates.emplace_back(m_pVisibleMeshIndexBuffer, m_pVisibleMeshIndexBuffer->GetReadState());
	m_pRenderGBufferCommandsResources->m_RequiredResourceStates.emplace_back(pMeshDescBuffer, pMeshDescBuffer->GetReadState());
	m_pRenderGBufferCommandsResources->m_RequiredResourceStates.emplace_back(m_pDrawMeshCommandBuffer, m_pDrawMeshCommandBuffer->GetWriteState());
	
	m_pRenderGBufferCommandsResources->m_SRVHeapStart = m_pShaderVisibleSRVHeap->Allocate();
	m_pDevice->CopyDescriptor(m_pRenderGBufferCommandsResources->m_SRVHeapStart, m_pNumVisibleMeshesBuffer->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), m_pVisibleMeshIndexBuffer->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), pMeshDescBuffer->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), m_pDrawMeshCommandBuffer->GetUAVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	RenderGBufferPass::InitParams fillGBufferParams;
	fillGBufferParams.m_pRenderEnv = m_pRenderEnv;
	fillGBufferParams.m_NormalRTVFormat = GetRenderTargetViewFormat(m_pNormalTexture->GetFormat());
	fillGBufferParams.m_DiffuseRTVFormat = GetRenderTargetViewFormat(m_pDiffuseTexture->GetFormat());
	fillGBufferParams.m_SpecularRTVFormat = GetRenderTargetViewFormat(m_pSpecularTexture->GetFormat());
	fillGBufferParams.m_DSVFormat = GetDepthStencilViewFormat(m_pDepthTexture->GetFormat());
	fillGBufferParams.m_pMeshBatch = m_pMeshBatch;
	
	m_pRenderGBufferPass = new RenderGBufferPass(&fillGBufferParams);
	
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

	RenderShadowMapCommandsPass::InitParams renderShadowMapCommandsParams;
	renderShadowMapCommandsParams.m_pRenderEnv = m_pRenderEnv;
	renderShadowMapCommandsParams.m_EnablePointLights = pScene->GetNumPointLights() > 0;
	renderShadowMapCommandsParams.m_MaxNumPointLightsPerShadowCaster = pScene->GetNumPointLights();
	renderShadowMapCommandsParams.m_EnableSpotLights = pScene->GetNumSpotLights() > 0;
	renderShadowMapCommandsParams.m_MaxNumSpotLightsPerShadowCaster = pScene->GetNumSpotLights();

	m_pRenderShadowMapCommandsPass = new RenderShadowMapCommandsPass(&renderShadowMapCommandsParams);

	if (pScene->GetNumPointLights() > 0)
	{
		D3DFormattedBufferDesc shadowCastingLightIndexBufferDesc(m_pMeshBatch->GetNumMeshes() * pScene->GetNumPointLights(), DXGI_FORMAT_R32_UINT, true, true);
		m_pShadowCastingPointLightIndexBuffer = new D3DBuffer(m_pRenderEnv, m_pRenderEnv->m_pDefaultHeapProps, &shadowCastingLightIndexBufferDesc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, L"m_pShadowCastingPointLightIndexBuffer");
		
		D3DFormattedBufferDesc numShadowCastingLightsBufferDesc(1, DXGI_FORMAT_R32_UINT, false, true);
		m_pNumShadowCastingPointLightsBuffer = new D3DBuffer(m_pRenderEnv, m_pRenderEnv->m_pDefaultHeapProps, &numShadowCastingLightsBufferDesc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, L"m_pNumShadowCastingPointLightsBuffer");

		D3DStructuredBufferDesc drawCommandBufferDesc(m_pMeshBatch->GetNumMeshes(), sizeof(DrawMeshCommand), false, true);
		m_pDrawPointLightShadowCasterCommandBuffer = new D3DBuffer(m_pRenderEnv, m_pRenderEnv->m_pDefaultHeapProps, &drawCommandBufferDesc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, L"m_pDrawPointLightShadowCasterCommandBuffer");
		
		D3DFormattedBufferDesc numShadowCastersBufferDesc(1, DXGI_FORMAT_R32_UINT, false, true);
		m_pNumDrawPointLightShadowCastersBuffer = new D3DBuffer(m_pRenderEnv, m_pRenderEnv->m_pDefaultHeapProps, &numShadowCastersBufferDesc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, L"m_pNumDrawPointLightShadowCastersBuffer");
	}

	if (pScene->GetNumSpotLights() > 0)
	{
		D3DFormattedBufferDesc shadowCastingLightIndexBufferDesc(m_pMeshBatch->GetNumMeshes() * pScene->GetNumSpotLights(), DXGI_FORMAT_R32_UINT, true, true);
		m_pShadowCastingSpotLightIndexBuffer = new D3DBuffer(m_pRenderEnv, m_pRenderEnv->m_pDefaultHeapProps, &shadowCastingLightIndexBufferDesc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, L"m_pShadowCastingSpotLightIndexBuffer");

		D3DFormattedBufferDesc numShadowCastingLightsBufferDesc(1, DXGI_FORMAT_R32_UINT, false, true);
		m_pNumShadowCastingSpotLightsBuffer = new D3DBuffer(m_pRenderEnv, m_pRenderEnv->m_pDefaultHeapProps, &numShadowCastingLightsBufferDesc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, L"m_pNumShadowCastingSpotLightsBuffer");

		D3DStructuredBufferDesc drawCommandBufferDesc(m_pMeshBatch->GetNumMeshes(), sizeof(DrawMeshCommand), false, true);
		m_pDrawSpotLightShadowCasterCommandBuffer = new D3DBuffer(m_pRenderEnv, m_pRenderEnv->m_pDefaultHeapProps, &drawCommandBufferDesc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, L"m_pDrawSpotLightShadowCasterCommandBuffer");

		D3DFormattedBufferDesc numShadowCastersBufferDesc(1, DXGI_FORMAT_R32_UINT, false, true);
		m_pNumDrawSpotLightShadowCastersBuffer = new D3DBuffer(m_pRenderEnv, m_pRenderEnv->m_pDefaultHeapProps, &numShadowCastersBufferDesc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, L"m_pNumDrawSpotLightShadowCastersBuffer");
	}

	m_pRenderShadowMapCommandsResources->m_RequiredResourceStates.emplace_back(m_pVisibleMeshIndexBuffer, m_pVisibleMeshIndexBuffer->GetReadState());
	m_pRenderShadowMapCommandsResources->m_RequiredResourceStates.emplace_back(pMeshBoundsBuffer, pMeshBoundsBuffer->GetReadState());
	m_pRenderShadowMapCommandsResources->m_RequiredResourceStates.emplace_back(pMeshDescBuffer, pMeshDescBuffer->GetReadState());
	m_pRenderShadowMapCommandsResources->m_RequiredResourceStates.emplace_back(m_pRenderShadowMapCommandsArgumentBuffer, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);

	m_pRenderShadowMapCommandsResources->m_SRVHeapStart = m_pShaderVisibleSRVHeap->Allocate();
	m_pDevice->CopyDescriptor(m_pRenderShadowMapCommandsResources->m_SRVHeapStart, m_pVisibleMeshIndexBuffer->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), pMeshBoundsBuffer->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), pMeshDescBuffer->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	if (m_pPointLightBuffer != nullptr)
	{
		D3DBuffer* pPointLightBoundsBuffer = m_pPointLightBuffer->GetLightBoundsBuffer();

		m_pRenderShadowMapCommandsResources->m_RequiredResourceStates.emplace_back(pPointLightBoundsBuffer, pPointLightBoundsBuffer->GetReadState());
		m_pRenderShadowMapCommandsResources->m_RequiredResourceStates.emplace_back(m_pNumVisiblePointLightsBuffer, m_pNumVisiblePointLightsBuffer->GetReadState());
		m_pRenderShadowMapCommandsResources->m_RequiredResourceStates.emplace_back(m_pVisiblePointLightIndexBuffer, m_pVisiblePointLightIndexBuffer->GetReadState());
		m_pRenderShadowMapCommandsResources->m_RequiredResourceStates.emplace_back(m_pShadowCastingPointLightIndexBuffer, m_pShadowCastingPointLightIndexBuffer->GetWriteState());
		m_pRenderShadowMapCommandsResources->m_RequiredResourceStates.emplace_back(m_pNumShadowCastingPointLightsBuffer, m_pNumShadowCastingPointLightsBuffer->GetWriteState());
		m_pRenderShadowMapCommandsResources->m_RequiredResourceStates.emplace_back(m_pDrawPointLightShadowCasterCommandBuffer, m_pDrawPointLightShadowCasterCommandBuffer->GetWriteState());
		m_pRenderShadowMapCommandsResources->m_RequiredResourceStates.emplace_back(m_pNumDrawPointLightShadowCastersBuffer, m_pNumDrawPointLightShadowCastersBuffer->GetWriteState());

		m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), pPointLightBoundsBuffer->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), m_pNumVisiblePointLightsBuffer->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), m_pVisiblePointLightIndexBuffer->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), m_pShadowCastingPointLightIndexBuffer->GetUAVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), m_pNumShadowCastingPointLightsBuffer->GetUAVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), m_pDrawPointLightShadowCasterCommandBuffer->GetUAVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), m_pNumDrawPointLightShadowCastersBuffer->GetUAVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	}

	if (m_pSpotLightBuffer != nullptr)
	{
		D3DBuffer* pSpotLightBoundsBuffer = m_pSpotLightBuffer->GetLightBoundsBuffer();

		m_pRenderShadowMapCommandsResources->m_RequiredResourceStates.emplace_back(pSpotLightBoundsBuffer, pSpotLightBoundsBuffer->GetReadState());
		m_pRenderShadowMapCommandsResources->m_RequiredResourceStates.emplace_back(m_pNumVisibleSpotLightsBuffer, m_pNumVisibleSpotLightsBuffer->GetReadState());
		m_pRenderShadowMapCommandsResources->m_RequiredResourceStates.emplace_back(m_pVisibleSpotLightIndexBuffer, m_pVisibleSpotLightIndexBuffer->GetReadState());
		m_pRenderShadowMapCommandsResources->m_RequiredResourceStates.emplace_back(m_pShadowCastingSpotLightIndexBuffer, m_pShadowCastingSpotLightIndexBuffer->GetWriteState());
		m_pRenderShadowMapCommandsResources->m_RequiredResourceStates.emplace_back(m_pNumShadowCastingSpotLightsBuffer, m_pNumShadowCastingSpotLightsBuffer->GetWriteState());
		m_pRenderShadowMapCommandsResources->m_RequiredResourceStates.emplace_back(m_pDrawSpotLightShadowCasterCommandBuffer, m_pDrawSpotLightShadowCasterCommandBuffer->GetWriteState());
		m_pRenderShadowMapCommandsResources->m_RequiredResourceStates.emplace_back(m_pNumDrawSpotLightShadowCastersBuffer, m_pNumDrawSpotLightShadowCastersBuffer->GetWriteState());

		m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), pSpotLightBoundsBuffer->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
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
		D3DBuffer* pLightBoundsBuffer = m_pPointLightBuffer->GetLightBoundsBuffer();
		D3DBuffer* pLightPropsBuffer = m_pPointLightBuffer->GetLightPropsBuffer();

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
		D3DBuffer* pLightBoundsBuffer = m_pSpotLightBuffer->GetLightBoundsBuffer();
		D3DBuffer* pLightPropsBuffer = m_pSpotLightBuffer->GetLightPropsBuffer();

		m_pTiledShadingResources->m_RequiredResourceStates.emplace_back(pLightBoundsBuffer, pLightBoundsBuffer->GetReadState());
		m_pTiledShadingResources->m_RequiredResourceStates.emplace_back(pLightPropsBuffer, pLightPropsBuffer->GetReadState());
		m_pTiledShadingResources->m_RequiredResourceStates.emplace_back(m_pSpotLightIndexPerTileBuffer, m_pSpotLightIndexPerTileBuffer->GetReadState());
		m_pTiledShadingResources->m_RequiredResourceStates.emplace_back(m_pSpotLightRangePerTileBuffer, m_pSpotLightRangePerTileBuffer->GetReadState());

		m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), pLightBoundsBuffer->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), pLightPropsBuffer->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), m_pSpotLightIndexPerTileBuffer->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), m_pSpotLightRangePerTileBuffer->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	}

	RenderTiledShadowMapPass::InitParams renderSpotLightTiledShadowMapParams;
	renderSpotLightTiledShadowMapParams.m_pRenderEnv = m_pRenderEnv;
	renderSpotLightTiledShadowMapParams.m_DSVFormat = GetDepthStencilViewFormat(m_pSpotLightTiledShadowMap->GetFormat());
	renderSpotLightTiledShadowMapParams.m_pMeshBatch = m_pMeshBatch;
	renderSpotLightTiledShadowMapParams.m_LightType = LightType_Spot;

	m_pRenderSpotLightTiledShadowMapPass = new RenderTiledShadowMapPass(&renderSpotLightTiledShadowMapParams);
		
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

	for (u8 index = 0; index < kBackBufferCount; ++index)
	{
		D3DColorTexture* pRenderTarget = m_pSwapChain->GetBackBuffer(index);

		m_VisualizeVoxelGridResources[index]->m_RequiredResourceStates.emplace_back(pRenderTarget, pRenderTarget->GetWriteState());
		m_VisualizeVoxelGridResources[index]->m_RequiredResourceStates.emplace_back(m_pDepthTexture, m_pDepthTexture->GetReadState());

		m_VisualizeVoxelGridResources[index]->m_RTVHeapStart = pRenderTarget->GetRTVHandle();
		m_VisualizeVoxelGridResources[index]->m_SRVHeapStart = m_pShaderVisibleSRVHeap->Allocate();

		m_pDevice->CopyDescriptor(m_VisualizeVoxelGridResources[index]->m_SRVHeapStart, m_pGridConfigBuffer->GetCBVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), m_pCameraTransformBuffer->GetCBVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), m_pDepthTexture->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), m_pGridBuffer->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	}
	
	VisualizeMeshPass::InitParams visualizeMeshParams;
	visualizeMeshParams.m_pRenderEnv = m_pRenderEnv;
	visualizeMeshParams.m_MeshDataElement = MeshDataElement_Normal;
	visualizeMeshParams.m_pMeshBatch = m_pMeshBatch;
	visualizeMeshParams.m_RTVFormat = GetRenderTargetViewFormat(m_pSwapChain->GetBackBuffer(m_BackBufferIndex)->GetFormat());
	visualizeMeshParams.m_DSVFormat = GetDepthStencilViewFormat(m_pDepthTexture->GetFormat());
		
	//m_pVisualizeMeshPass = new VisualizeMeshPass(&visualizeMeshParams);

	for (u8 index = 0; index < kBackBufferCount; ++index)
	{
		D3DColorTexture* pRenderTarget = m_pSwapChain->GetBackBuffer(index);

		m_VisualizeMeshResources[index]->m_RequiredResourceStates.emplace_back(pRenderTarget, pRenderTarget->GetWriteState());
		m_VisualizeMeshResources[index]->m_RequiredResourceStates.emplace_back(m_pDepthTexture, m_pDepthTexture->GetWriteState());
		
		m_VisualizeMeshResources[index]->m_RTVHeapStart = pRenderTarget->GetRTVHandle();
		m_VisualizeMeshResources[index]->m_DSVHeapStart = m_pDepthTexture->GetDSVHandle();
		m_VisualizeMeshResources[index]->m_SRVHeapStart = m_pShaderVisibleSRVHeap->Allocate();
		
		m_pDevice->CopyDescriptor(m_VisualizeMeshResources[index]->m_SRVHeapStart, m_pObjectTransformBuffer->GetCBVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	}

	D3DColorTexture* pCopyTexture = m_pAccumLightTexture;

	CopyTexturePass::InitParams copyTextureParams;
	copyTextureParams.m_pRenderEnv = m_pRenderEnv;
	copyTextureParams.m_RTVFormat = GetRenderTargetViewFormat(m_pSwapChain->GetBackBuffer(0)->GetFormat());
	
	m_pCopyTexturePass = new CopyTexturePass(&copyTextureParams);

	for (u8 index = 0; index < kBackBufferCount; ++index)
	{
		D3DColorTexture* pRenderTarget = m_pSwapChain->GetBackBuffer(index);

		m_CopyTextureResources[index]->m_RequiredResourceStates.emplace_back(pRenderTarget, pRenderTarget->GetWriteState());
		m_CopyTextureResources[index]->m_RequiredResourceStates.emplace_back(pCopyTexture, pCopyTexture->GetReadState());

		m_CopyTextureResources[index]->m_SRVHeapStart = m_pShaderVisibleSRVHeap->Allocate();
		m_CopyTextureResources[index]->m_RTVHeapStart = pRenderTarget->GetRTVHandle();

		m_pDevice->CopyDescriptor(m_CopyTextureResources[index]->m_SRVHeapStart, pCopyTexture->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	}

	// Kolya: Should be moved to OnUpdate
	// Temporarily moved constant buffer update here to overcome frame capture crash on AMD R9 290
	const Vector3f& mainCameraPos = m_pCamera->GetTransform().GetPosition();
	const Quaternion& mainCameraRotation = m_pCamera->GetTransform().GetRotation();
	const Matrix4f mainViewProjMatrix = m_pCamera->GetViewMatrix() * m_pCamera->GetProjMatrix();
	const Frustum mainCameraFrustum = ExtractWorldFrustum(*m_pCamera);

	ObjectTransform objectTransform;
	objectTransform.m_WorldPosMatrix = Matrix4f::IDENTITY;
	objectTransform.m_WorldNormalMatrix = Matrix4f::IDENTITY;
	objectTransform.m_WorldViewProjMatrix = mainViewProjMatrix;

	m_pObjectTransformBuffer->Write(&objectTransform, sizeof(objectTransform));

	const BasisAxes mainCameraBasis = ExtractBasisAxes(mainCameraRotation);
	assert(IsNormalized(mainCameraBasis.m_XAxis));
	assert(IsNormalized(mainCameraBasis.m_YAxis));
	assert(IsNormalized(mainCameraBasis.m_ZAxis));

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
	viewFrustumCullingData.m_NumMeshes = m_pMeshBatch->GetNumMeshes();

	m_pViewFrustumCullingDataBuffer->Write(&viewFrustumCullingData, sizeof(viewFrustumCullingData));

	TiledLightCullingData tiledLightCullingData;
	tiledLightCullingData.m_RcpScreenSize = Rcp(Vector2f((f32)bufferWidth, (f32)bufferHeight));
	tiledLightCullingData.m_ViewMatrix = m_pCamera->GetViewMatrix();
	tiledLightCullingData.m_ProjMatrix = m_pCamera->GetProjMatrix();
	tiledLightCullingData.m_ProjInvMatrix = Inverse(m_pCamera->GetProjMatrix());

	m_pTiledLightCullingDataBuffer->Write(&tiledLightCullingData, sizeof(tiledLightCullingData));

	TiledShadingData tiledShadingData;
	tiledShadingData.m_RcpScreenSize = Rcp(Vector2f((f32)bufferWidth, (f32)bufferHeight));
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

	SafeDelete(pScene);
}

void DXApplication::OnUpdate()
{
}

void DXApplication::OnRender()
{
	D3DCommandAllocator* pCommandAllocator = m_CommandAllocators[m_BackBufferIndex];
	pCommandAllocator->Reset();

	D3DColorTexture* pRenderTarget = m_pSwapChain->GetBackBuffer(m_BackBufferIndex);
	
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = pRenderTarget->GetRTVHandle();
	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = m_pDepthTexture->GetDSVHandle();
	
	const u8 clearFlags = m_pCamera->GetClearFlags();
	if (clearFlags != 0)
	{
		std::vector<D3DResourceTransitionBarrier> resourceTransitions;
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

		m_pCommandList->Reset(pCommandAllocator);
		m_pCommandList->ResourceBarrier(resourceTransitions.size(), &resourceTransitions[0]);
		m_pCommandList->ClearRenderTargetView(rtvHandle, &clearColor.m_X);
		m_pCommandList->ClearDepthView(dsvHandle);				
		m_pCommandList->Close();

		m_pCommandQueue->ExecuteCommandLists(m_pRenderEnv, 1, &m_pCommandList, pCommandAllocator);
		WaitForGPU();
	}

	ViewFrustumCullingPass::RenderParams detectVisibleMeshesParams;
	detectVisibleMeshesParams.m_pRenderEnv = m_pRenderEnv;
	detectVisibleMeshesParams.m_pCommandList = m_pCommandList;
	detectVisibleMeshesParams.m_pCommandAllocator = pCommandAllocator;
	detectVisibleMeshesParams.m_pResources = m_pDetectVisibleMeshesResources;
	detectVisibleMeshesParams.m_pNumVisibleObjectsBuffer = m_pNumVisibleMeshesBuffer;

	m_pDetectVisibleMeshesPass->Record(&detectVisibleMeshesParams);
	m_pCommandQueue->ExecuteCommandLists(m_pRenderEnv, 1, &m_pCommandList, pCommandAllocator);
	WaitForGPU();

	if (m_pPointLightBuffer != nullptr)
	{
		ViewFrustumCullingPass::RenderParams detectVisibleLightsParams;
		detectVisibleLightsParams.m_pRenderEnv = m_pRenderEnv;
		detectVisibleLightsParams.m_pCommandList = m_pCommandList;
		detectVisibleLightsParams.m_pCommandAllocator = pCommandAllocator;
		detectVisibleLightsParams.m_pResources = m_pDetectVisiblePointLightsResources;
		detectVisibleLightsParams.m_pNumVisibleObjectsBuffer = m_pNumVisiblePointLightsBuffer;

		m_pDetectVisiblePointLightsPass->Record(&detectVisibleLightsParams);
		m_pCommandQueue->ExecuteCommandLists(m_pRenderEnv, 1, &m_pCommandList, pCommandAllocator);
		WaitForGPU();
	}

	if (m_pSpotLightBuffer != nullptr)
	{
		ViewFrustumCullingPass::RenderParams detectVisibleLightsParams;
		detectVisibleLightsParams.m_pRenderEnv = m_pRenderEnv;
		detectVisibleLightsParams.m_pCommandList = m_pCommandList;
		detectVisibleLightsParams.m_pCommandAllocator = pCommandAllocator;
		detectVisibleLightsParams.m_pResources = m_pDetectVisibleSpotLightsResources;
		detectVisibleLightsParams.m_pNumVisibleObjectsBuffer = m_pNumVisibleSpotLightsBuffer;

		m_pDetectVisibleSpotLightsPass->Record(&detectVisibleLightsParams);
		m_pCommandQueue->ExecuteCommandLists(m_pRenderEnv, 1, &m_pCommandList, pCommandAllocator);
		WaitForGPU();
	}
	
	RenderGBufferCommandsPass::RenderParams fillGBufferCommandsParams;
	fillGBufferCommandsParams.m_pRenderEnv = m_pRenderEnv;
	fillGBufferCommandsParams.m_pCommandList = m_pCommandList;
	fillGBufferCommandsParams.m_pCommandAllocator = pCommandAllocator;
	fillGBufferCommandsParams.m_pResources = m_pRenderGBufferCommandsResources;

	m_pRenderGBufferCommandsPass->Record(&fillGBufferCommandsParams);
	m_pCommandQueue->ExecuteCommandLists(m_pRenderEnv, 1, &m_pCommandList, pCommandAllocator);
	WaitForGPU();

	RenderGBufferPass::RenderParams fillGBufferParams;
	fillGBufferParams.m_pRenderEnv = m_pRenderEnv;
	fillGBufferParams.m_pCommandList = m_pCommandList;
	fillGBufferParams.m_pCommandAllocator = pCommandAllocator;
	fillGBufferParams.m_pResources = m_pRenderGBufferResources;
	fillGBufferParams.m_pViewport = m_pViewport;
	fillGBufferParams.m_pMeshBatch = m_pMeshBatch;
	fillGBufferParams.m_pDrawMeshCommandBuffer = m_pDrawMeshCommandBuffer;
	fillGBufferParams.m_pNumDrawMeshesBuffer = m_pNumVisibleMeshesBuffer;
	
	m_pRenderGBufferPass->Record(&fillGBufferParams);
	m_pCommandQueue->ExecuteCommandLists(m_pRenderEnv, 1, &m_pCommandList, pCommandAllocator);
	WaitForGPU();

	TiledLightCullingPass::RenderParams tiledLightCullingParams;
	tiledLightCullingParams.m_pRenderEnv = m_pRenderEnv;
	tiledLightCullingParams.m_pCommandList = m_pCommandList;
	tiledLightCullingParams.m_pCommandAllocator = pCommandAllocator;
	tiledLightCullingParams.m_pResources = m_pTiledLightCullingResources;
	tiledLightCullingParams.m_pNumPointLightsPerTileBuffer = m_pNumPointLightsPerTileBuffer;
	tiledLightCullingParams.m_pNumSpotLightsPerTileBuffer = m_pNumSpotLightsPerTileBuffer;

	m_pTiledLightCullingPass->Record(&tiledLightCullingParams);
	m_pCommandQueue->ExecuteCommandLists(m_pRenderEnv, 1, &m_pCommandList, pCommandAllocator);
	WaitForGPU();

	{
		std::vector<D3DResourceTransitionBarrier> resourceTransitions;
		if (m_pRenderShadowMapCommandsArgumentBuffer->GetState() != D3D12_RESOURCE_STATE_COPY_DEST)
		{
			resourceTransitions.emplace_back(m_pRenderShadowMapCommandsArgumentBuffer, m_pRenderShadowMapCommandsArgumentBuffer->GetState(), D3D12_RESOURCE_STATE_COPY_DEST);
			m_pRenderShadowMapCommandsArgumentBuffer->SetState(D3D12_RESOURCE_STATE_COPY_DEST);
		}
		if (m_pNumVisibleMeshesBuffer->GetState() != D3D12_RESOURCE_STATE_COPY_SOURCE)
		{
			resourceTransitions.emplace_back(m_pNumVisibleMeshesBuffer, m_pNumVisibleMeshesBuffer->GetState(), D3D12_RESOURCE_STATE_COPY_SOURCE);
			m_pNumVisibleMeshesBuffer->SetState(D3D12_RESOURCE_STATE_COPY_SOURCE);
		}

		m_pCommandList->Reset(pCommandAllocator, nullptr);
		m_pCommandList->ResourceBarrier(resourceTransitions.size(), &resourceTransitions[0]);
		m_pCommandList->CopyBufferRegion(m_pRenderShadowMapCommandsArgumentBuffer, 0, m_pNumVisibleMeshesBuffer, 0, sizeof(u32));
		m_pCommandList->Close();

		m_pCommandQueue->ExecuteCommandLists(m_pRenderEnv, 1, &m_pCommandList, pCommandAllocator);
		WaitForGPU();
	}

	RenderShadowMapCommandsPass::RenderParams renderShadowMapCommandsParams;
	renderShadowMapCommandsParams.m_pRenderEnv = m_pRenderEnv;
	renderShadowMapCommandsParams.m_pCommandList = m_pCommandList;
	renderShadowMapCommandsParams.m_pCommandAllocator = pCommandAllocator;
	renderShadowMapCommandsParams.m_pResources = m_pRenderShadowMapCommandsResources;
	renderShadowMapCommandsParams.m_pIndirectArgumentBuffer = m_pRenderShadowMapCommandsArgumentBuffer;
	renderShadowMapCommandsParams.m_pNumShadowCastingPointLightsBuffer = m_pNumShadowCastingPointLightsBuffer;
	renderShadowMapCommandsParams.m_pNumDrawPointLightShadowCastersBuffer = m_pNumDrawPointLightShadowCastersBuffer;
	renderShadowMapCommandsParams.m_pNumShadowCastingSpotLightsBuffer = m_pNumShadowCastingSpotLightsBuffer;
	renderShadowMapCommandsParams.m_pNumDrawSpotLightShadowCastersBuffer = m_pNumDrawSpotLightShadowCastersBuffer;

	//m_pRenderShadowMapCommandsPass->Record(&renderShadowMapCommandsParams);
	//m_pCommandQueue->ExecuteCommandLists(m_pRenderEnv, 1, &m_pCommandList, pCommandAllocator);
	//WaitForGPU();

	TiledShadingPass::RenderParams tiledShadingParams;
	tiledShadingParams.m_pRenderEnv = m_pRenderEnv;
	tiledShadingParams.m_pCommandList = m_pCommandList;
	tiledShadingParams.m_pCommandAllocator = pCommandAllocator;
	tiledShadingParams.m_pResources = m_pTiledShadingResources;
	tiledShadingParams.m_pAccumLightTexture = m_pAccumLightTexture;
	
	m_pTiledShadingPass->Record(&tiledShadingParams);
	m_pCommandQueue->ExecuteCommandLists(m_pRenderEnv, 1, &m_pCommandList, pCommandAllocator);
	WaitForGPU();

	CopyTexturePass::RenderParams copyTextureParams;
	copyTextureParams.m_pRenderEnv = m_pRenderEnv;
	copyTextureParams.m_pCommandList = m_pCommandList;
	copyTextureParams.m_pCommandAllocator = pCommandAllocator;
	copyTextureParams.m_pResources = m_CopyTextureResources[m_BackBufferIndex];
	copyTextureParams.m_pViewport = m_pViewport;

	m_pCopyTexturePass->Record(&copyTextureParams);
	m_pCommandQueue->ExecuteCommandLists(m_pRenderEnv, 1, &m_pCommandList, pCommandAllocator);
	WaitForGPU();

	/*
	VisualizeMeshPass::RenderParams visualizeMeshParams;
	visualizeMeshParams.m_pRenderEnv = m_pRenderEnv;
	visualizeMeshParams.m_pCommandList = m_pCommandList;
	visualizeMeshParams.m_pCommandAllocator = pCommandAllocator;
	visualizeMeshParams.m_pResources = m_VisualizeMeshResources[m_BackBufferIndex];
	visualizeMeshParams.m_pViewport = m_pViewport;
	visualizeMeshParams.m_pMeshBatch = m_pMeshBatch;

	m_pVisualizeMeshPass->Record(&visualizeMeshParams);
	m_pCommandQueue->ExecuteCommandLists(m_pRenderEnv, 1, &m_pCommandList, pCommandAllocator);
	WaitForGPU();
	*/

	ClearVoxelGridPass::RenderParams clearGridParams;
	clearGridParams.m_pRenderEnv = m_pRenderEnv;
	clearGridParams.m_pCommandAllocator = pCommandAllocator;
	clearGridParams.m_pCommandList = m_pCommandList;
	clearGridParams.m_pResources = m_pClearVoxelGridResources;
	
	m_pClearVoxelGridPass->Record(&clearGridParams);
	m_pCommandQueue->ExecuteCommandLists(m_pRenderEnv, 1, &m_pCommandList, pCommandAllocator);
	WaitForGPU();

	CreateVoxelGridPass::RenderParams createGridParams;
	createGridParams.m_pRenderEnv = m_pRenderEnv;
	createGridParams.m_pCommandList = m_pCommandList;
	createGridParams.m_pCommandAllocator = pCommandAllocator;
	createGridParams.m_pResources = m_pCreateVoxelGridResources;
	createGridParams.m_pViewport = m_pViewport;
	createGridParams.m_pMeshBatch = m_pMeshBatch;
	createGridParams.m_pDrawMeshCommandBuffer = m_pDrawMeshCommandBuffer;
	createGridParams.m_pNumDrawMeshesBuffer = m_pNumVisibleMeshesBuffer;
	
	m_pCreateVoxelGridPass->Record(&createGridParams);
	m_pCommandQueue->ExecuteCommandLists(m_pRenderEnv, 1, &m_pCommandList, pCommandAllocator);
	WaitForGPU();
	
	VisualizeVoxelGridPass::RenderParams visualizeGridParams;
	visualizeGridParams.m_pRenderEnv = m_pRenderEnv;
	visualizeGridParams.m_pCommandList = m_pCommandList;
	visualizeGridParams.m_pCommandAllocator = pCommandAllocator;
	visualizeGridParams.m_pResources = m_VisualizeVoxelGridResources[m_BackBufferIndex];
	visualizeGridParams.m_pViewport = m_pViewport;
	
	//m_pVisualizeVoxelGridPass->Record(&visualizeGridParams);
	//m_pCommandQueue->ExecuteCommandLists(m_pRenderEnv, 1, &m_pCommandList, pCommandAllocator);
	
	if (pRenderTarget->GetState() != D3D12_RESOURCE_STATE_PRESENT)
	{
		m_pCommandList->Reset(pCommandAllocator, nullptr);

		D3DResourceTransitionBarrier resourceTransition(pRenderTarget, pRenderTarget->GetState(), D3D12_RESOURCE_STATE_PRESENT);
		m_pCommandList->ResourceBarrier(1, &resourceTransition);
		pRenderTarget->SetState(D3D12_RESOURCE_STATE_PRESENT);

		m_pCommandList->Close();
		m_pCommandQueue->ExecuteCommandLists(m_pRenderEnv, 1, &m_pCommandList, nullptr);
	}

	m_pSwapChain->Present(1, 0);
	MoveToNextFrame();
}

void DXApplication::OnDestroy()
{
	WaitForGPU();
}

void DXApplication::OnKeyDown(UINT8 key)
{
}

void DXApplication::OnKeyUp(UINT8 key)
{
}

void DXApplication::WaitForGPU()
{
	m_pCommandQueue->Signal(m_pFence, m_FenceValues[m_BackBufferIndex]);
	m_pFence->WaitForSignal(m_FenceValues[m_BackBufferIndex]);

	++m_FenceValues[m_BackBufferIndex];
}

void DXApplication::MoveToNextFrame()
{
	const UINT64 currentFenceValue = m_FenceValues[m_BackBufferIndex];
	m_pCommandQueue->Signal(m_pFence, currentFenceValue);

	m_BackBufferIndex = m_pSwapChain->GetCurrentBackBufferIndex();
	m_pFence->WaitForSignal(m_FenceValues[m_BackBufferIndex]);

	m_FenceValues[m_BackBufferIndex] = currentFenceValue + 1;
}
