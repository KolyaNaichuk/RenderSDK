#include "DXApplication.h"
#include "DX/DXFactory.h"
#include "DX/DXDevice.h"
#include "DX/DXSwapChain.h"
#include "DX/DXCommandQueue.h"
#include "DX/DXCommandAllocator.h"
#include "DX/DXCommandList.h"
#include "DX/DXDescriptorHeap.h"
#include "DX/DXResource.h"
#include "DX/DXFence.h"
#include "DX/DXRenderEnvironment.h"
#include "DX/DXUtils.h"
#include "DX/DXResourceList.h"
#include "DX/DXCommandSignature.h"
#include "CommandRecorders/FillGBufferRecorder.h"
#include "CommandRecorders/TiledShadingRecorder.h"
#include "CommandRecorders/ClearVoxelGridRecorder.h"
#include "CommandRecorders/CreateVoxelGridRecorder.h"
#include "CommandRecorders/InjectVPLsIntoVoxelGridRecorder.h"
#include "CommandRecorders/VisualizeVoxelGridRecorder.h"
#include "CommandRecorders/VisualizeMeshRecorder.h"
#include "CommandRecorders/DetectVisibleMeshesRecorder.h"
#include "CommandRecorders/CreateFillGBufferCommandsRecorder.h"
#include "CommandRecorders/CopyTextureRecorder.h"
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
	kNumGridCellsZ = 64
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

struct CullingData
{
	Vector4f m_ViewFrustumPlanes[6];
	u32 m_NumMeshes;
	Vector3u m_NotUsed1;
	Vector4f m_NotUsed2[9];
};

struct ShadingData
{
	Vector2f m_RcpScreenSize;
	Vector2f m_NotUsed1;
	Vector3f m_WorldSpaceLightDir;
	f32 m_NotUsed2;
	Vector3f m_LightColor;
	f32 m_NotUsed3;
	Vector3f m_WorldSpaceCameraPos;
	f32 m_NotUsed4;
	Matrix4f m_ViewMatrix;
	Matrix4f m_ProjMatrix;
	Matrix4f m_ProjInvMatrix;
	Matrix4f m_ViewProjInvMatrix;
	Matrix4f m_NotUsed5[3];
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
	, m_pDefaultHeapProps(new DXHeapProperties(D3D12_HEAP_TYPE_DEFAULT))
	, m_pUploadHeapProps(new DXHeapProperties(D3D12_HEAP_TYPE_UPLOAD))
	, m_pShaderInvisibleRTVHeap(nullptr)
	, m_pShaderInvisibleDSVHeap(nullptr)
	, m_pShaderInvisibleSRVHeap(nullptr)
	, m_pShaderVisibleSRVHeap(nullptr)
	, m_pDepthTexture(nullptr)
	, m_pDiffuseTexture(nullptr)
	, m_pNormalTexture(nullptr)
	, m_pSpecularTexture(nullptr)
	, m_pAccumLightTexture(nullptr)
	, m_pViewport(nullptr)
	, m_pObjectTransformBuffer(nullptr)
	, m_pCameraTransformBuffer(nullptr)
	, m_pGridBuffer(nullptr)
	, m_pGridConfigBuffer(nullptr)
	, m_pCullingDataBuffer(nullptr)
	, m_pShadingDataBuffer(nullptr)
	, m_pDrawCommandBuffer(nullptr)
	, m_pNumVisibleMeshesBuffer(nullptr)
	, m_pVisibleMeshIndexBuffer(nullptr)
	, m_pLightIndicesPerTileBuffer(nullptr)
	, m_pNumLightIndicesForAllTilesBuffer(nullptr)
	, m_pRenderShadowMapCommandBuffer(nullptr)
	, m_pNumRenderShadowMapCommandsBuffer(nullptr)
	, m_pLightIndicesPerMeshBuffer(nullptr)
	, m_pNumLightIndicesForAllMeshesBuffer(nullptr)
	, m_pRenderEnv(new DXRenderEnvironment())
	, m_pFence(nullptr)
	, m_BackBufferIndex(0)
	, m_pFillGBufferRecorder(nullptr)
	, m_pFillGBufferResources(nullptr)
	, m_pTiledShadingRecorder(nullptr)
	, m_pTiledShadingResources(nullptr)
	, m_pClearVoxelGridRecorder(nullptr)
	, m_pClearVoxelGridResources(nullptr)
	, m_pCreateVoxelGridRecorder(nullptr)
	, m_pCreateVoxelGridResources(nullptr)
	, m_pInjectVPLsIntoVoxelGridRecorder(nullptr)
	, m_pVisualizeVoxelGridRecorder(nullptr)
	, m_pVisualizeMeshRecorder(nullptr)
	, m_pDetectVisibleMeshesRecorder(nullptr)
	, m_pDetectVisibleMeshesResources(nullptr)
	, m_pCreateFillGBufferCommandsRecorder(nullptr)
	, m_pCreateFillGBufferCommandsResources(nullptr)
	, m_pCopyTextureRecorder(nullptr)
	, m_pMeshBatch(nullptr)
	, m_pSpotLightBuffer(nullptr)
	, m_pPointLightBuffer(nullptr)
	, m_pCamera(nullptr)
{
	std::memset(m_CommandAllocators, 0, sizeof(m_CommandAllocators));
	std::memset(m_FenceValues, 0, sizeof(m_FenceValues));
	std::memset(m_VisualizeMeshResources, 0, sizeof(m_VisualizeMeshResources));
	std::memset(m_VisualizeVoxelGridResources, 0, sizeof(m_VisualizeVoxelGridResources));
	std::memset(m_CopyTextureResources, 0, sizeof(m_CopyTextureResources));
}

DXApplication::~DXApplication()
{
	for (UINT index = 0; index < kBackBufferCount; ++index)
	{
		SafeDelete(m_CommandAllocators[index]);
		SafeDelete(m_VisualizeMeshResources[index]);
		SafeDelete(m_VisualizeVoxelGridResources[index]);
		SafeDelete(m_CopyTextureResources[index]);
	}

	SafeDelete(m_pCamera);
	SafeDelete(m_pSpotLightBuffer);
	SafeDelete(m_pPointLightBuffer);
	SafeDelete(m_pMeshBatch);
	SafeDelete(m_pLightIndicesPerTileBuffer);
	SafeDelete(m_pNumLightIndicesForAllTilesBuffer);
	SafeDelete(m_pRenderShadowMapCommandBuffer);
	SafeDelete(m_pNumRenderShadowMapCommandsBuffer);
	SafeDelete(m_pLightIndicesPerMeshBuffer);
	SafeDelete(m_pNumLightIndicesForAllMeshesBuffer);
	SafeDelete(m_pCopyTextureRecorder);
	SafeDelete(m_pClearVoxelGridRecorder);
	SafeDelete(m_pClearVoxelGridResources);
	SafeDelete(m_pCreateVoxelGridRecorder);
	SafeDelete(m_pCreateVoxelGridResources);
	SafeDelete(m_pInjectVPLsIntoVoxelGridRecorder);
	SafeDelete(m_pVisualizeVoxelGridRecorder);
	SafeDelete(m_pVisualizeMeshRecorder);
	SafeDelete(m_pTiledShadingRecorder);
	SafeDelete(m_pTiledShadingResources);
	SafeDelete(m_pFillGBufferRecorder);
	SafeDelete(m_pFillGBufferResources);
	SafeDelete(m_pCreateFillGBufferCommandsRecorder);
	SafeDelete(m_pCreateFillGBufferCommandsResources);
	SafeDelete(m_pDetectVisibleMeshesRecorder);
	SafeDelete(m_pDetectVisibleMeshesResources);
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
	SafeDelete(m_pDrawCommandBuffer);
	SafeDelete(m_pCullingDataBuffer);
	SafeDelete(m_pShadingDataBuffer);
	SafeDelete(m_pGridConfigBuffer);
	SafeDelete(m_pGridBuffer);
	SafeDelete(m_pCameraTransformBuffer);
	SafeDelete(m_pObjectTransformBuffer);
	SafeDelete(m_pDiffuseTexture);
	SafeDelete(m_pNormalTexture);
	SafeDelete(m_pSpecularTexture);
	SafeDelete(m_pAccumLightTexture);
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
	DXFactory factory;
	m_pDevice = new DXDevice(&factory, D3D_FEATURE_LEVEL_12_0, true);

	D3D12_FEATURE_DATA_D3D12_OPTIONS supportedOptions;
	m_pDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &supportedOptions, sizeof(supportedOptions));
	//assert(supportedOptions.ConservativeRasterizationTier != D3D12_CONSERVATIVE_RASTERIZATION_TIER_NOT_SUPPORTED);
	assert(supportedOptions.ROVsSupported == TRUE);

	DXCommandQueueDesc commandQueueDesc(D3D12_COMMAND_LIST_TYPE_DIRECT);
	m_pCommandQueue = new DXCommandQueue(m_pDevice, &commandQueueDesc, L"m_pCommandQueue");

	DXDescriptorHeapDesc rtvHeapDesc(D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 9, false);
	m_pShaderInvisibleRTVHeap = new DXDescriptorHeap(m_pDevice, &rtvHeapDesc, L"m_pShaderInvisibleRTVHeap");

	DXDescriptorHeapDesc shaderVisibleSRVHeapDesc(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 43, true);
	m_pShaderVisibleSRVHeap = new DXDescriptorHeap(m_pDevice, &shaderVisibleSRVHeapDesc, L"m_pShaderVisibleSRVHeap");

	DXDescriptorHeapDesc shaderInvisibleSRVHeapDesc(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 32, false);
	m_pShaderInvisibleSRVHeap = new DXDescriptorHeap(m_pDevice, &shaderInvisibleSRVHeapDesc, L"m_pShaderInvisibleSRVHeap");

	DXDescriptorHeapDesc dsvHeapDesc(D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 1, false);
	m_pShaderInvisibleDSVHeap = new DXDescriptorHeap(m_pDevice, &dsvHeapDesc, L"m_pShaderInvisibleDSVHeap");

	for (UINT index = 0; index < kBackBufferCount; ++index)
		m_CommandAllocators[index] = new DXCommandAllocator(m_pDevice, D3D12_COMMAND_LIST_TYPE_DIRECT, L"m_CommandAllocators");

	m_pCommandListPool = new DXCommandListPool(m_pDevice, D3D12_COMMAND_LIST_TYPE_DIRECT);
	m_pCommandList = new DXCommandList(m_pDevice, m_CommandAllocators[m_BackBufferIndex], nullptr, L"m_pCommandList");

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

	m_pViewport = new DXViewport(0.0f, 0.0f, (FLOAT)bufferWidth, (float)bufferHeight);

	m_pCamera = new Camera(Camera::ProjType_Perspective, 0.1f, 1300.0f, FLOAT(bufferWidth) / FLOAT(bufferHeight));
	m_pCamera->SetClearFlags(Camera::ClearFlag_Color | Camera::ClearFlag_Depth);
	m_pCamera->SetBackgroundColor(Color::GRAY);

	Transform& transform = m_pCamera->GetTransform();
	transform.SetPosition(Vector3f(278.0f, 274.0f, 700.0f));
	transform.SetRotation(CreateRotationYQuaternion(Radian(PI)));

	DXSwapChainDesc swapChainDesc(kBackBufferCount, m_pWindow->GetHWND(), bufferWidth, bufferHeight);
	m_pSwapChain = new DXSwapChain(&factory, m_pRenderEnv, &swapChainDesc, m_pCommandQueue);
	m_BackBufferIndex = m_pSwapChain->GetCurrentBackBufferIndex();
	
	DXDepthTexture2DDesc depthTexDesc(DXGI_FORMAT_R32_TYPELESS, bufferWidth, bufferHeight, true, true);
	DXDepthStencilValue optimizedClearDepth(1.0f);
	m_pDepthTexture = new DXDepthTexture(m_pRenderEnv, m_pRenderEnv->m_pDefaultHeapProps, &depthTexDesc, D3D12_RESOURCE_STATE_DEPTH_WRITE, &optimizedClearDepth, L"m_pDepthTexture");

	const FLOAT optimizedClearColor[4] = {0.0f, 0.0f, 0.0f, 0.0f};
	DXColorTexture2DDesc diffuseTexDesc(DXGI_FORMAT_R10G10B10A2_UNORM, bufferWidth, bufferHeight, true, true, false);
	m_pDiffuseTexture = new DXColorTexture(m_pRenderEnv, m_pRenderEnv->m_pDefaultHeapProps, &diffuseTexDesc, D3D12_RESOURCE_STATE_RENDER_TARGET, optimizedClearColor, L"m_pDiffuseTexture");

	DXColorTexture2DDesc normalTexDesc(DXGI_FORMAT_R8G8B8A8_SNORM, bufferWidth, bufferHeight, true, true, false);
	m_pNormalTexture = new DXColorTexture(m_pRenderEnv, m_pRenderEnv->m_pDefaultHeapProps, &normalTexDesc, D3D12_RESOURCE_STATE_RENDER_TARGET, optimizedClearColor, L"m_pNormalTexture");

	DXColorTexture2DDesc specularTexDesc(DXGI_FORMAT_R8G8B8A8_UNORM, bufferWidth, bufferHeight, true, true, false);
	m_pSpecularTexture = new DXColorTexture(m_pRenderEnv, m_pRenderEnv->m_pDefaultHeapProps, &specularTexDesc, D3D12_RESOURCE_STATE_RENDER_TARGET, optimizedClearColor, L"m_pSpecularTexture");

	DXColorTexture2DDesc accumLightTexDesc(DXGI_FORMAT_R10G10B10A2_UNORM, bufferWidth, bufferHeight, false, true, true);
	m_pAccumLightTexture = new DXColorTexture(m_pRenderEnv, m_pRenderEnv->m_pDefaultHeapProps, &accumLightTexDesc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, optimizedClearColor, L"m_pAccumLightTexture");

	DXConstantBufferDesc objectTransformBufferDesc(sizeof(ObjectTransform));
	m_pObjectTransformBuffer = new DXBuffer(m_pRenderEnv, m_pRenderEnv->m_pUploadHeapProps, &objectTransformBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, L"m_pObjectTransformBuffer");

	DXConstantBufferDesc cameraTransformBufferDesc(sizeof(CameraTransform));
	m_pCameraTransformBuffer = new DXBuffer(m_pRenderEnv, m_pRenderEnv->m_pUploadHeapProps, &cameraTransformBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, L"m_pCameraTransformBuffer");

	DXConstantBufferDesc gridConfigBufferDesc(sizeof(GridConfig));
	m_pGridConfigBuffer = new DXBuffer(m_pRenderEnv, m_pRenderEnv->m_pUploadHeapProps, &gridConfigBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, L"m_pGridConfigBuffer");

	DXConstantBufferDesc cullingDataBufferDesc(sizeof(CullingData));
	m_pCullingDataBuffer = new DXBuffer(m_pRenderEnv, m_pRenderEnv->m_pUploadHeapProps, &cullingDataBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, L"m_pCullingDataBuffer");

	DXConstantBufferDesc shadingDataBufferDesc(sizeof(ShadingData));
	m_pShadingDataBuffer = new DXBuffer(m_pRenderEnv, m_pRenderEnv->m_pUploadHeapProps, &shadingDataBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, L"m_pShadingDataBuffer");

	const u32 numGridElements = kNumGridCellsX * kNumGridCellsY * kNumGridCellsZ;
	DXStructuredBufferDesc gridBufferDesc(numGridElements, sizeof(Voxel), true, true);
	m_pGridBuffer = new DXBuffer(m_pRenderEnv, m_pRenderEnv->m_pDefaultHeapProps, &gridBufferDesc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, L"m_pGridBuffer");

	m_pFence = new DXFence(m_pDevice, m_FenceValues[m_BackBufferIndex]);
	++m_FenceValues[m_BackBufferIndex];

	Scene* pScene = SceneLoader::LoadCornellBox(CornellBoxSettings_Test2);
	
	assert(pScene->GetNumMeshBatches() == 1);
	MeshBatchData* pMeshBatchData = *pScene->GetMeshBatches();
	
	DXStructuredBufferDesc drawCommandBufferDesc(pMeshBatchData->GetNumMeshes(), sizeof(DrawIndexedCommand), true, true);
	m_pDrawCommandBuffer = new DXBuffer(m_pRenderEnv, m_pRenderEnv->m_pDefaultHeapProps, &drawCommandBufferDesc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, L"m_pDrawCommandBuffer");
	
	m_pMeshBatch = new MeshBatch(m_pRenderEnv, pMeshBatchData);
	m_pMeshBatch->RecordDataForUpload(m_pCommandList);

	if (pScene->GetNumPointLights() > 0)
	{
		m_pPointLightBuffer = new LightBuffer(m_pRenderEnv, pScene->GetNumPointLights(), pScene->GetPointLights());
		m_pPointLightBuffer->RecordDataForUpload(m_pCommandList);
	}
	if (pScene->GetNumSpotLights() > 0)
	{
		m_pSpotLightBuffer = new LightBuffer(m_pRenderEnv, pScene->GetNumSpotLights(), pScene->GetSpotLights());
		m_pSpotLightBuffer->RecordDataForUpload(m_pCommandList);
	}
	
	m_pCommandList->Close();
	m_pCommandQueue->ExecuteCommandLists(m_pRenderEnv, 1, &m_pCommandList, nullptr);
	WaitForGPU();

	m_pMeshBatch->RemoveDataForUpload();
	
	if (m_pPointLightBuffer != nullptr)
		m_pPointLightBuffer->RemoveDataForUpload();

	if (m_pSpotLightBuffer != nullptr)
		m_pSpotLightBuffer->RemoveDataForUpload();

	DXFormattedBufferDesc numVisibleMeshesBufferDesc(1, DXGI_FORMAT_R32_UINT, true, true);
	m_pNumVisibleMeshesBuffer = new DXBuffer(m_pRenderEnv, m_pRenderEnv->m_pDefaultHeapProps, &numVisibleMeshesBufferDesc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, L"m_pNumCommandsBuffer");

	DXFormattedBufferDesc visibleMeshIndexBufferDesc(m_pMeshBatch->GetNumMeshes(), DXGI_FORMAT_R32_UINT, true, true);
	m_pVisibleMeshIndexBuffer = new DXBuffer(m_pRenderEnv, m_pRenderEnv->m_pDefaultHeapProps, &visibleMeshIndexBufferDesc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, L"m_pVisibleMeshIndexBuffer");
	
	DXBuffer* pMeshBoundsBuffer = m_pMeshBatch->GetMeshBoundsBuffer();
	DXBuffer* pMeshDescBuffer = m_pMeshBatch->GetMeshDescBuffer();
	DXBuffer* pMaterialBuffer = m_pMeshBatch->GetMaterialBuffer();

	DetectVisibleMeshesRecorder::InitParams detectVisibleMeshesParams;
	detectVisibleMeshesParams.m_pRenderEnv = m_pRenderEnv;
	detectVisibleMeshesParams.m_NumMeshesInBatch = m_pMeshBatch->GetNumMeshes();
	
	m_pDetectVisibleMeshesRecorder = new DetectVisibleMeshesRecorder(&detectVisibleMeshesParams);
	
	m_pDetectVisibleMeshesResources = new DXBindingResourceList();
	m_pDetectVisibleMeshesResources->m_ResourceTransitions.emplace_back(m_pNumVisibleMeshesBuffer, m_pNumVisibleMeshesBuffer->GetWriteState());
	m_pDetectVisibleMeshesResources->m_ResourceTransitions.emplace_back(m_pVisibleMeshIndexBuffer, m_pVisibleMeshIndexBuffer->GetWriteState());
	m_pDetectVisibleMeshesResources->m_ResourceTransitions.emplace_back(pMeshBoundsBuffer, pMeshBoundsBuffer->GetReadState());
	
	m_pDetectVisibleMeshesResources->m_SRVHeapStart = m_pShaderVisibleSRVHeap->Allocate();
	m_pDevice->CopyDescriptor(m_pDetectVisibleMeshesResources->m_SRVHeapStart, m_pNumVisibleMeshesBuffer->GetUAVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), m_pVisibleMeshIndexBuffer->GetUAVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), pMeshBoundsBuffer->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), m_pCullingDataBuffer->GetCBVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	
	CreateFillGBufferCommandsRecorder::InitParams createFillGBufferCommandsParams;
	createFillGBufferCommandsParams.m_pRenderEnv = m_pRenderEnv;
	createFillGBufferCommandsParams.m_NumMeshesInBatch = m_pMeshBatch->GetNumMeshes();
	
	m_pCreateFillGBufferCommandsRecorder = new CreateFillGBufferCommandsRecorder(&createFillGBufferCommandsParams);

	m_pCreateFillGBufferCommandsResources = new DXBindingResourceList();
	m_pCreateFillGBufferCommandsResources->m_ResourceTransitions.emplace_back(m_pNumVisibleMeshesBuffer, m_pNumVisibleMeshesBuffer->GetReadState());
	m_pCreateFillGBufferCommandsResources->m_ResourceTransitions.emplace_back(m_pVisibleMeshIndexBuffer, m_pVisibleMeshIndexBuffer->GetReadState());
	m_pCreateFillGBufferCommandsResources->m_ResourceTransitions.emplace_back(pMeshDescBuffer, pMeshDescBuffer->GetReadState());
	m_pCreateFillGBufferCommandsResources->m_ResourceTransitions.emplace_back(m_pDrawCommandBuffer, m_pDrawCommandBuffer->GetWriteState());
	
	m_pCreateFillGBufferCommandsResources->m_SRVHeapStart = m_pShaderVisibleSRVHeap->Allocate();
	m_pDevice->CopyDescriptor(m_pCreateFillGBufferCommandsResources->m_SRVHeapStart, m_pNumVisibleMeshesBuffer->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), m_pVisibleMeshIndexBuffer->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), pMeshDescBuffer->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), m_pDrawCommandBuffer->GetUAVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	FillGBufferRecorder::InitParams fillGBufferParams;
	fillGBufferParams.m_pRenderEnv = m_pRenderEnv;
	fillGBufferParams.m_NormalRTVFormat = GetRenderTargetViewFormat(m_pNormalTexture->GetFormat());
	fillGBufferParams.m_DiffuseRTVFormat = GetRenderTargetViewFormat(m_pDiffuseTexture->GetFormat());
	fillGBufferParams.m_SpecularRTVFormat = GetRenderTargetViewFormat(m_pSpecularTexture->GetFormat());
	fillGBufferParams.m_DSVFormat = GetDepthStencilViewFormat(m_pDepthTexture->GetFormat());
	fillGBufferParams.m_pMeshBatch = m_pMeshBatch;
	
	m_pFillGBufferRecorder = new FillGBufferRecorder(&fillGBufferParams);

	m_pFillGBufferResources = new DXBindingResourceList();
	m_pFillGBufferResources->m_ResourceTransitions.emplace_back(m_pNormalTexture, m_pNormalTexture->GetWriteState());
	m_pFillGBufferResources->m_ResourceTransitions.emplace_back(m_pDiffuseTexture, m_pDiffuseTexture->GetWriteState());
	m_pFillGBufferResources->m_ResourceTransitions.emplace_back(m_pSpecularTexture, m_pSpecularTexture->GetWriteState());
	m_pFillGBufferResources->m_ResourceTransitions.emplace_back(m_pDepthTexture, m_pDepthTexture->GetWriteState());
	m_pFillGBufferResources->m_ResourceTransitions.emplace_back(pMaterialBuffer, pMaterialBuffer->GetReadState());
	
	m_pFillGBufferResources->m_RTVHeapStart = m_pShaderInvisibleRTVHeap->Allocate();
	m_pFillGBufferResources->m_DSVHeapStart = m_pDepthTexture->GetDSVHandle();
	m_pFillGBufferResources->m_SRVHeapStart = m_pShaderVisibleSRVHeap->Allocate();
	m_pDevice->CopyDescriptor(m_pFillGBufferResources->m_RTVHeapStart, m_pNormalTexture->GetRTVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	m_pDevice->CopyDescriptor(m_pShaderInvisibleRTVHeap->Allocate(), m_pDiffuseTexture->GetRTVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	m_pDevice->CopyDescriptor(m_pShaderInvisibleRTVHeap->Allocate(), m_pSpecularTexture->GetRTVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	m_pDevice->CopyDescriptor(m_pFillGBufferResources->m_SRVHeapStart, m_pObjectTransformBuffer->GetCBVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), pMaterialBuffer->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	TiledShadingRecorder::InitParams tiledShadingParams;
	tiledShadingParams.m_pRenderEnv = m_pRenderEnv;
	tiledShadingParams.m_ShadingMode = ShadingMode_Phong;
	tiledShadingParams.m_NumTilesX = kNumTilesX;
	tiledShadingParams.m_NumTilesY = kNumTilesY;
	tiledShadingParams.m_NumPointLights = (m_pPointLightBuffer != nullptr) ? m_pPointLightBuffer->GetNumLights() : 0;
	tiledShadingParams.m_NumSpotLights = (m_pSpotLightBuffer != nullptr) ? m_pSpotLightBuffer->GetNumLights() : 0;
	tiledShadingParams.m_UseDirectionalLight = pScene->GetDirectionalLight() != nullptr;

	m_pTiledShadingRecorder = new TiledShadingRecorder(&tiledShadingParams);

	m_pTiledShadingResources = new DXBindingResourceList();
	m_pTiledShadingResources->m_ResourceTransitions.emplace_back(m_pAccumLightTexture, m_pAccumLightTexture->GetWriteState());
	m_pTiledShadingResources->m_ResourceTransitions.emplace_back(m_pDepthTexture, m_pDepthTexture->GetReadState());
	m_pTiledShadingResources->m_ResourceTransitions.emplace_back(m_pNormalTexture, m_pNormalTexture->GetReadState());
	m_pTiledShadingResources->m_ResourceTransitions.emplace_back(m_pDiffuseTexture, m_pDiffuseTexture->GetReadState());
	m_pTiledShadingResources->m_ResourceTransitions.emplace_back(m_pSpecularTexture, m_pSpecularTexture->GetReadState());

	m_pTiledShadingResources->m_SRVHeapStart = m_pShaderVisibleSRVHeap->Allocate();
	m_pDevice->CopyDescriptor(m_pTiledShadingResources->m_SRVHeapStart, m_pAccumLightTexture->GetUAVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), m_pShadingDataBuffer->GetCBVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), m_pDepthTexture->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), m_pNormalTexture->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), m_pDiffuseTexture->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), m_pSpecularTexture->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	if (m_pPointLightBuffer != nullptr)
	{
		DXBuffer* pLightBoundsBuffer = m_pPointLightBuffer->GetLightBoundsBuffer();
		DXBuffer* pLightPropsBuffer = m_pPointLightBuffer->GetLightPropsBuffer();

		m_pTiledShadingResources->m_ResourceTransitions.emplace_back(pLightBoundsBuffer, pLightBoundsBuffer->GetReadState());
		m_pTiledShadingResources->m_ResourceTransitions.emplace_back(pLightPropsBuffer, pLightPropsBuffer->GetReadState());

		m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), pLightBoundsBuffer->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), pLightPropsBuffer->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	}
	if (m_pSpotLightBuffer != nullptr)
	{
		DXBuffer* pLightBoundsBuffer = m_pSpotLightBuffer->GetLightBoundsBuffer();
		DXBuffer* pLightPropsBuffer = m_pSpotLightBuffer->GetLightPropsBuffer();

		m_pTiledShadingResources->m_ResourceTransitions.emplace_back(pLightBoundsBuffer, pLightBoundsBuffer->GetReadState());
		m_pTiledShadingResources->m_ResourceTransitions.emplace_back(pLightPropsBuffer, pLightPropsBuffer->GetReadState());

		m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), pLightBoundsBuffer->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), pLightPropsBuffer->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	}
		
	ClearVoxelGridRecorder::InitParams clearGridParams;
	clearGridParams.m_pRenderEnv = m_pRenderEnv;
	clearGridParams.m_NumGridCellsX = kNumGridCellsX;
	clearGridParams.m_NumGridCellsY = kNumGridCellsY;
	clearGridParams.m_NumGridCellsZ = kNumGridCellsZ;

	m_pClearVoxelGridRecorder = new ClearVoxelGridRecorder(&clearGridParams);

	m_pClearVoxelGridResources = new DXBindingResourceList;
	m_pClearVoxelGridResources->m_ResourceTransitions.emplace_back(m_pGridBuffer, m_pGridBuffer->GetWriteState());
	m_pClearVoxelGridResources->m_SRVHeapStart = m_pShaderVisibleSRVHeap->Allocate();
	m_pDevice->CopyDescriptor(m_pClearVoxelGridResources->m_SRVHeapStart, m_pGridConfigBuffer->GetCBVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), m_pGridBuffer->GetUAVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	CreateVoxelGridRecorder::InitParams createGridParams;
	createGridParams.m_pRenderEnv = m_pRenderEnv;
	createGridParams.m_pMeshBatch = m_pMeshBatch;

	m_pCreateVoxelGridRecorder = new CreateVoxelGridRecorder(&createGridParams);

	m_pCreateVoxelGridResources = new DXBindingResourceList();
	m_pCreateVoxelGridResources->m_ResourceTransitions.emplace_back(pMaterialBuffer, pMaterialBuffer->GetReadState());
	m_pCreateVoxelGridResources->m_ResourceTransitions.emplace_back(m_pGridBuffer, m_pGridBuffer->GetWriteState());	
	m_pCreateVoxelGridResources->m_SRVHeapStart = m_pShaderVisibleSRVHeap->Allocate();
	
	m_pDevice->CopyDescriptor(m_pCreateVoxelGridResources->m_SRVHeapStart, m_pObjectTransformBuffer->GetCBVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), m_pCameraTransformBuffer->GetCBVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), m_pGridConfigBuffer->GetCBVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), pMaterialBuffer->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), m_pGridBuffer->GetUAVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	InjectVPLsIntoVoxelGridRecorder::InitPrams injectVPLsParams;
	injectVPLsParams.m_pRenderEnv = m_pRenderEnv;
	injectVPLsParams.m_NumGridCellsX = kNumGridCellsX;
	injectVPLsParams.m_NumGridCellsY = kNumGridCellsY;
	injectVPLsParams.m_NumGridCellsZ = kNumGridCellsZ;

	//m_pInjectVPLsIntoVoxelGridRecorder = new InjectVPLsIntoVoxelGridRecorder(&injectVPLsParams);

	VisualizeVoxelGridRecorder::InitParams visualizeGridParams;
	visualizeGridParams.m_pRenderEnv = m_pRenderEnv;
	visualizeGridParams.m_RTVFormat = GetRenderTargetViewFormat(m_pSwapChain->GetBackBuffer(m_BackBufferIndex)->GetFormat());
	
	m_pVisualizeVoxelGridRecorder = new VisualizeVoxelGridRecorder(&visualizeGridParams);

	for (u8 index = 0; index < kBackBufferCount; ++index)
	{
		DXColorTexture* pRenderTarget = m_pSwapChain->GetBackBuffer(index);

		m_VisualizeVoxelGridResources[index] = new DXBindingResourceList();
		m_VisualizeVoxelGridResources[index]->m_ResourceTransitions.emplace_back(pRenderTarget, pRenderTarget->GetWriteState());
		m_VisualizeVoxelGridResources[index]->m_ResourceTransitions.emplace_back(m_pDepthTexture, m_pDepthTexture->GetReadState());

		m_VisualizeVoxelGridResources[index]->m_RTVHeapStart = pRenderTarget->GetRTVHandle();
		m_VisualizeVoxelGridResources[index]->m_SRVHeapStart = m_pShaderVisibleSRVHeap->Allocate();

		m_pDevice->CopyDescriptor(m_VisualizeVoxelGridResources[index]->m_SRVHeapStart, m_pGridConfigBuffer->GetCBVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), m_pCameraTransformBuffer->GetCBVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), m_pDepthTexture->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), m_pGridBuffer->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	}
	
	VisualizeMeshRecorder::InitParams visualizeMeshParams;
	visualizeMeshParams.m_pRenderEnv = m_pRenderEnv;
	visualizeMeshParams.m_MeshDataElement = MeshDataElement_Normal;
	visualizeMeshParams.m_pMeshBatch = m_pMeshBatch;
	visualizeMeshParams.m_RTVFormat = GetRenderTargetViewFormat(m_pSwapChain->GetBackBuffer(m_BackBufferIndex)->GetFormat());
	visualizeMeshParams.m_DSVFormat = GetDepthStencilViewFormat(m_pDepthTexture->GetFormat());
		
	//m_pVisualizeMeshRecorder = new VisualizeMeshRecorder(&visualizeMeshParams);

	for (u8 index = 0; index < kBackBufferCount; ++index)
	{
		DXColorTexture* pRenderTarget = m_pSwapChain->GetBackBuffer(index);

		m_VisualizeMeshResources[index] = new DXBindingResourceList();
		m_VisualizeMeshResources[index]->m_ResourceTransitions.emplace_back(pRenderTarget, pRenderTarget->GetWriteState());
		m_VisualizeMeshResources[index]->m_ResourceTransitions.emplace_back(m_pDepthTexture, m_pDepthTexture->GetWriteState());
		
		m_VisualizeMeshResources[index]->m_RTVHeapStart = pRenderTarget->GetRTVHandle();
		m_VisualizeMeshResources[index]->m_DSVHeapStart = m_pDepthTexture->GetDSVHandle();
		m_VisualizeMeshResources[index]->m_SRVHeapStart = m_pShaderVisibleSRVHeap->Allocate();
		
		m_pDevice->CopyDescriptor(m_VisualizeMeshResources[index]->m_SRVHeapStart, m_pObjectTransformBuffer->GetCBVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	}

	DXColorTexture* pCopyTexture = m_pAccumLightTexture;

	CopyTextureRecorder::InitParams copyTextureParams;
	copyTextureParams.m_pRenderEnv = m_pRenderEnv;
	copyTextureParams.m_RTVFormat = GetRenderTargetViewFormat(m_pSwapChain->GetBackBuffer(0)->GetFormat());
	
	m_pCopyTextureRecorder = new CopyTextureRecorder(&copyTextureParams);

	for (u8 index = 0; index < kBackBufferCount; ++index)
	{
		DXColorTexture* pRenderTarget = m_pSwapChain->GetBackBuffer(index);

		m_CopyTextureResources[index] = new DXBindingResourceList();
		m_CopyTextureResources[index]->m_ResourceTransitions.emplace_back(pRenderTarget, pRenderTarget->GetWriteState());
		m_CopyTextureResources[index]->m_ResourceTransitions.emplace_back(pCopyTexture, pCopyTexture->GetReadState());

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
			
	CullingData cullingData;
	for (u8 planeIndex = 0; planeIndex < Frustum::NumPlanes; ++planeIndex)
		cullingData.m_ViewFrustumPlanes[planeIndex] = ToVector4f(mainCameraFrustum.m_Planes[planeIndex]);
	cullingData.m_NumMeshes = m_pMeshBatch->GetNumMeshes();
	m_pCullingDataBuffer->Write(&cullingData, sizeof(cullingData));

	ShadingData shadingData;
	shadingData.m_RcpScreenSize = Rcp(Vector2f((f32)bufferWidth, (f32)bufferHeight));
	shadingData.m_WorldSpaceCameraPos = mainCameraPos;
	shadingData.m_ViewMatrix = m_pCamera->GetViewMatrix();
	shadingData.m_ProjMatrix = m_pCamera->GetProjMatrix();
	shadingData.m_ProjInvMatrix = Inverse(m_pCamera->GetProjMatrix());
	shadingData.m_ViewProjInvMatrix = Inverse(m_pCamera->GetViewMatrix() * m_pCamera->GetProjMatrix());

	const DirectionalLight* pDirectionalLight = pScene->GetDirectionalLight();
	if (pDirectionalLight != nullptr)
	{
		const BasisAxes lightBasis = ExtractBasisAxes(pDirectionalLight->GetTransform().GetRotation());

		shadingData.m_WorldSpaceLightDir = Normalize(lightBasis.m_ZAxis);
		shadingData.m_LightColor = pDirectionalLight->GetColor();
	}

	m_pShadingDataBuffer->Write(&shadingData, sizeof(shadingData));

	SafeDelete(pScene);
}

void DXApplication::OnUpdate()
{
}

void DXApplication::OnRender()
{
	DXCommandAllocator* pCommandAllocator = m_CommandAllocators[m_BackBufferIndex];
	pCommandAllocator->Reset();

	DXColorTexture* pRenderTarget = m_pSwapChain->GetBackBuffer(m_BackBufferIndex);
	
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = pRenderTarget->GetRTVHandle();
	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = m_pDepthTexture->GetDSVHandle();
	
	const u8 clearFlags = m_pCamera->GetClearFlags();
	if (clearFlags != 0)
	{
		m_pCommandList->Reset(pCommandAllocator);

		std::vector<DXResourceTransitionBarrier> resourceTransitions;
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
		m_pCommandList->ResourceBarrier(resourceTransitions.size(), &resourceTransitions[0]);

		const Vector4f& clearColor = m_pCamera->GetBackgroundColor();
		m_pCommandList->ClearRenderTargetView(rtvHandle, &clearColor.m_X);
		m_pCommandList->ClearDepthView(dsvHandle);				
		m_pCommandList->Close();

		m_pCommandQueue->ExecuteCommandLists(m_pRenderEnv, 1, &m_pCommandList, pCommandAllocator);
		WaitForGPU();
	}

	DetectVisibleMeshesRecorder::RenderPassParams detectVisibleMeshesParams;
	detectVisibleMeshesParams.m_pRenderEnv = m_pRenderEnv;
	detectVisibleMeshesParams.m_pCommandList = m_pCommandList;
	detectVisibleMeshesParams.m_pCommandAllocator = pCommandAllocator;
	detectVisibleMeshesParams.m_pResources = m_pDetectVisibleMeshesResources;
	detectVisibleMeshesParams.m_pNumVisibleMeshesBuffer = m_pNumVisibleMeshesBuffer;

	m_pDetectVisibleMeshesRecorder->Record(&detectVisibleMeshesParams);
	m_pCommandQueue->ExecuteCommandLists(m_pRenderEnv, 1, &m_pCommandList, pCommandAllocator);
	WaitForGPU();
	
	CreateFillGBufferCommandsRecorder::RenderPassParams createFillGBufferCommandsParams;
	createFillGBufferCommandsParams.m_pRenderEnv = m_pRenderEnv;
	createFillGBufferCommandsParams.m_pCommandList = m_pCommandList;
	createFillGBufferCommandsParams.m_pCommandAllocator = pCommandAllocator;
	createFillGBufferCommandsParams.m_pResources = m_pCreateFillGBufferCommandsResources;

	m_pCreateFillGBufferCommandsRecorder->Record(&createFillGBufferCommandsParams);
	m_pCommandQueue->ExecuteCommandLists(m_pRenderEnv, 1, &m_pCommandList, pCommandAllocator);
	WaitForGPU();

	FillGBufferRecorder::RenderPassParams fillGBufferParams;
	fillGBufferParams.m_pRenderEnv = m_pRenderEnv;
	fillGBufferParams.m_pCommandList = m_pCommandList;
	fillGBufferParams.m_pCommandAllocator = pCommandAllocator;
	fillGBufferParams.m_pResources = m_pFillGBufferResources;
	fillGBufferParams.m_pViewport = m_pViewport;
	fillGBufferParams.m_pMeshBatch = m_pMeshBatch;
	fillGBufferParams.m_pDrawCommandBuffer = m_pDrawCommandBuffer;
	fillGBufferParams.m_pNumCommandsBuffer = m_pNumVisibleMeshesBuffer;
	
	m_pFillGBufferRecorder->Record(&fillGBufferParams);
	m_pCommandQueue->ExecuteCommandLists(m_pRenderEnv, 1, &m_pCommandList, pCommandAllocator);
	WaitForGPU();

	TiledShadingRecorder::RenderPassParams tiledShadingParams;
	tiledShadingParams.m_pRenderEnv = m_pRenderEnv;
	tiledShadingParams.m_pCommandList = m_pCommandList;
	tiledShadingParams.m_pCommandAllocator = pCommandAllocator;
	tiledShadingParams.m_pResources = m_pTiledShadingResources;
	tiledShadingParams.m_pAccumLightTexture = m_pAccumLightTexture;
	
	m_pTiledShadingRecorder->Record(&tiledShadingParams);
	m_pCommandQueue->ExecuteCommandLists(m_pRenderEnv, 1, &m_pCommandList, pCommandAllocator);
	WaitForGPU();

	CopyTextureRecorder::RenderPassParams copyTextureParams;
	copyTextureParams.m_pRenderEnv = m_pRenderEnv;
	copyTextureParams.m_pCommandList = m_pCommandList;
	copyTextureParams.m_pCommandAllocator = pCommandAllocator;
	copyTextureParams.m_pResources = m_CopyTextureResources[m_BackBufferIndex];
	copyTextureParams.m_pViewport = m_pViewport;

	m_pCopyTextureRecorder->Record(&copyTextureParams);
	m_pCommandQueue->ExecuteCommandLists(m_pRenderEnv, 1, &m_pCommandList, pCommandAllocator);
	WaitForGPU();

	/*
	VisualizeMeshRecorder::RenderPassParams visualizeMeshParams;
	visualizeMeshParams.m_pRenderEnv = m_pRenderEnv;
	visualizeMeshParams.m_pCommandList = m_pCommandList;
	visualizeMeshParams.m_pCommandAllocator = pCommandAllocator;
	visualizeMeshParams.m_pResources = m_VisualizeMeshResources[m_BackBufferIndex];
	visualizeMeshParams.m_pViewport = m_pViewport;
	visualizeMeshParams.m_pMeshBatch = m_pMeshBatch;

	m_pVisualizeMeshRecorder->Record(&visualizeMeshParams);
	m_pCommandQueue->ExecuteCommandLists(m_pRenderEnv, 1, &m_pCommandList, pCommandAllocator);
	WaitForGPU();
	*/

	ClearVoxelGridRecorder::RenderPassParams clearGridParams;
	clearGridParams.m_pRenderEnv = m_pRenderEnv;
	clearGridParams.m_pCommandAllocator = pCommandAllocator;
	clearGridParams.m_pCommandList = m_pCommandList;
	clearGridParams.m_pResources = m_pClearVoxelGridResources;
	
	m_pClearVoxelGridRecorder->Record(&clearGridParams);
	m_pCommandQueue->ExecuteCommandLists(m_pRenderEnv, 1, &m_pCommandList, pCommandAllocator);
	WaitForGPU();

	CreateVoxelGridRecorder::RenderPassParams createGridParams;
	createGridParams.m_pRenderEnv = m_pRenderEnv;
	createGridParams.m_pCommandList = m_pCommandList;
	createGridParams.m_pCommandAllocator = pCommandAllocator;
	createGridParams.m_pResources = m_pCreateVoxelGridResources;
	createGridParams.m_pViewport = m_pViewport;
	createGridParams.m_pMeshBatch = m_pMeshBatch;
	createGridParams.m_pDrawCommandBuffer = m_pDrawCommandBuffer;
	createGridParams.m_pNumCommandsBuffer = m_pNumVisibleMeshesBuffer;
	
	m_pCreateVoxelGridRecorder->Record(&createGridParams);
	m_pCommandQueue->ExecuteCommandLists(m_pRenderEnv, 1, &m_pCommandList, pCommandAllocator);
	WaitForGPU();
	
	VisualizeVoxelGridRecorder::RenderPassParams visualizeGridParams;
	visualizeGridParams.m_pRenderEnv = m_pRenderEnv;
	visualizeGridParams.m_pCommandList = m_pCommandList;
	visualizeGridParams.m_pCommandAllocator = pCommandAllocator;
	visualizeGridParams.m_pResources = m_VisualizeVoxelGridResources[m_BackBufferIndex];
	visualizeGridParams.m_pViewport = m_pViewport;
	
	m_pVisualizeVoxelGridRecorder->Record(&visualizeGridParams);
	//m_pCommandQueue->ExecuteCommandLists(m_pRenderEnv, 1, &m_pCommandList, pCommandAllocator);
	
	if (pRenderTarget->GetState() != D3D12_RESOURCE_STATE_PRESENT)
	{
		m_pCommandList->Reset(pCommandAllocator, nullptr);

		DXResourceTransitionBarrier resourceTransition(pRenderTarget, pRenderTarget->GetState(), D3D12_RESOURCE_STATE_PRESENT);
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
