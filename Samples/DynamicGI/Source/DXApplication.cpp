#include "DXApplication.h"
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
#include "RenderPasses/CreateRenderShadowMapCommandsPass.h"
#include "RenderPasses/CreateVoxelizeCommandsPass.h"
#include "RenderPasses/PropagateLightPass.h"
#include "RenderPasses/RenderGBufferPass.h"
#include "RenderPasses/RenderTiledShadowMapPass.h"
#include "RenderPasses/SetupTiledShadowMapPass.h"
#include "RenderPasses/TiledLightCullingPass.h"
#include "RenderPasses/TiledShadingPass.h"
#include "RenderPasses/FrustumLightCullingPass.h"
#include "RenderPasses/VisualizeTexturePass.h"
#include "RenderPasses/VisualizeVoxelReflectancePass.h"
#include "RenderPasses/DownscaleAndReprojectDepthPass.h"
#include "RenderPasses/FrustumMeshCullingPass.h"
#include "RenderPasses/FillVisibilityBufferPass.h"
#include "RenderPasses/CreateMainDrawCommandsPass.h"
#include "RenderPasses/CreateFalseNegativeDrawCommandsPass.h"
#include "RenderPasses/FillMeshTypeDepthBufferPass.h"
#include "RenderPasses/CalcShadingRectanglesPass.h"
#include "RenderPasses/VoxelizePass.h"
#include "Common/Mesh.h"
#include "Common/MeshBatch.h"
#include "Common/GeometryBuffer.h"
#include "Common/MeshRenderResources.h"
#include "Common/LightRenderResources.h"
#include "Common/MaterialRenderResources.h"
#include "Common/Color.h"
#include "Common/Camera.h"
#include "Common/Scene.h"
#include "Common/SceneLoader.h"
#include "Math/Vector3.h"
#include "Math/Vector4.h"
#include "Math/Matrix4.h"
#include "Math/Transform.h"
#include "Math/BasisAxes.h"

/*
To do:
- VoxelizePass is using 4th component for calculating how many objects overlap the voxel. Should be set to opacity instead.
- Fix format for voxel reflectance texture
- Directional light m_EnableDirectionalLight seems to be disabled on all render passes. Enable
- Verify voxel texture position is compatible with texture coordinates in VoxelizePS.hlsl and VisualizeVoxelGridPS.hlsl
- VoxelizePass and TiledShadingPass make copy of material descriptors. Reuse material descriptors between them
- Check how spot light bounds are calculated. See TiledShadingPS.hlsl and VoxelizePS.hlsl how spot light position is calculated
- When injecting reflected radiance into voxel grid add shadow map contribution.
- Add support for mip levels. Search for keyword "KolyaMipLevels"
- Review light view matrix computation for shadow maps in LightRenderResources.
- OOB for a set of points mimics AABB. Improve implementation.
- When converting plane mesh to unit cube space, world matrix is not optimal for OOB.
For an example, plane in original coordinates is passing through point (0, 0, 0).
OOB will have coordinates expanding from -1 to 1 not merely passing through 0 when world matrix is applied.
- Make camera transform part of Scene object.
Can check format OpenGEX for inspiration - http://opengex.org/
- MeshRenderResources, LightRenderResources, MaterialRenderResources should not be part of Common folder
- Using std::experimental::filesystem::path from OBJFileLoader. Should be consistent with the code.
- Review to-dos
- Use output resource states to initialize input resource states from the previous pass.
- Use Task graph for resource state transition after each render pass.
https://patterns.eecs.berkeley.edu/?page_id=609
- Fix compilation warnings for x64 build
- Check that inside CreateRenderShadowMapCommands.hlsl we are checking the bound
against MAX_NUM_SPOT_LIGHTS_PER_SHADOW_CASTER and MAX_NUM_POINT_LIGHTS_PER_SHADOW_CASTER
while writing data to the local storage
*/

namespace
{
	struct AppData
	{
		Matrix4f m_ViewMatrix;
		Matrix4f m_ViewInvMatrix;
		Matrix4f m_ProjMatrix;
		Matrix4f m_ProjInvMatrix;

		Matrix4f m_ViewProjMatrix;
		Matrix4f m_ViewProjInvMatrix;
		Matrix4f m_PrevViewProjMatrix;
		Matrix4f m_PrevViewProjInvMatrix;

		Matrix4f m_NotUsed1;
		Vector4f m_CameraWorldSpacePos;
		Vector4f m_CameraWorldFrustumPlanes[Frustum::NumPlanes];
		f32 m_CameraNearPlane;
		f32 m_CameraFarPlane;
		Vector2f m_NotUsed2;
		Vector2u m_ScreenSize;
		Vector2f m_RcpScreenSize;
		Vector2u m_ScreenHalfSize;
		Vector2f m_RcpScreenHalfSize;
		Vector2u m_ScreenQuarterSize;
		Vector2f m_RcpScreenQuarterSize;
		Vector4f m_SunWorldSpaceDir;
		
		Vector4f m_SunLightColor;
		Vector2u m_ScreenTileSize;
		Vector2u m_NumScreenTiles;
		Vector3f m_VoxelGridWorldMinPoint;
		f32 m_NotUsed3;
		Vector3f m_VoxelGridWorldMaxPoint;
		f32 m_NotUsed4;
		Matrix4f m_VoxelGridViewProjMatrices[3];
		Vector3f m_VoxelRcpSize;

		f32 m_NotUsed5[61];
	};

	using BufferElementFormatter = std::function<std::string (const void* pElementData)>;
	
	void OutputBufferContent(RenderEnv* pRenderEnv, Buffer* pBuffer, D3D12_RESOURCE_STATES bufferState,
		UINT elementSizeInBytes, BufferElementFormatter elementFormatter)
	{
		ID3D12Device* pD3DDevice = pRenderEnv->m_pDevice->GetD3DObject();
		ID3D12CommandQueue* pD3DQueue = pRenderEnv->m_pCommandQueue->GetD3DObject();

		ID3D12Resource* pD3DResource = pBuffer->GetD3DObject();
		D3D12_RESOURCE_DESC resourceDesc = pD3DResource->GetDesc();
		assert(resourceDesc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER);
		resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

		ComPtr<ID3D12Resource> d3dResourceCopy;
		VerifyD3DResult(pD3DDevice->CreateCommittedResource(pRenderEnv->m_pReadbackHeapProps,
			D3D12_HEAP_FLAG_NONE,
			&resourceDesc,
			D3D12_RESOURCE_STATE_COPY_DEST,
			nullptr, IID_PPV_ARGS(&d3dResourceCopy)));
		d3dResourceCopy->SetName(L"OutputBufferContent::d3dResourceCopy");

		D3D12_COMMAND_LIST_TYPE commandListType = D3D12_COMMAND_LIST_TYPE_DIRECT;

		ComPtr<ID3D12CommandAllocator> d3dCommandAllocator;
		VerifyD3DResult(pD3DDevice->CreateCommandAllocator(commandListType,
			IID_PPV_ARGS(&d3dCommandAllocator)));
		d3dCommandAllocator->SetName(L"OutputBufferContent::d3dCommandAllocator");

		ComPtr<ID3D12GraphicsCommandList> d3dCommandList;
		VerifyD3DResult(pD3DDevice->CreateCommandList(0, commandListType,
			d3dCommandAllocator.Get(), nullptr, IID_PPV_ARGS(&d3dCommandList)));
		VerifyD3DResult(d3dCommandList->Close());
		VerifyD3DResult(d3dCommandList->SetName(L"OutputBufferContent::d3dCommandList"));

		UINT64 fenceValue = 0;
		ComPtr<ID3D12Fence> d3dFence;
		VerifyD3DResult(pD3DDevice->CreateFence(fenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&d3dFence)));
		VerifyD3DResult(d3dFence->SetName(L"OutputBufferContent::d3dFence"));

		HANDLE completionEvent = CreateEventEx(nullptr, FALSE, FALSE, EVENT_ALL_ACCESS);
		assert(completionEvent != INVALID_HANDLE_VALUE);

		d3dCommandAllocator->Reset();
		d3dCommandList->Reset(d3dCommandAllocator.Get(), nullptr);
		if (bufferState != D3D12_RESOURCE_STATE_COPY_SOURCE)
		{
			D3D12_RESOURCE_BARRIER resourceBarrier;
			resourceBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			resourceBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
			resourceBarrier.Transition.pResource = pD3DResource;
			resourceBarrier.Transition.StateBefore = bufferState;
			resourceBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE;
			resourceBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

			d3dCommandList->ResourceBarrier(1, &resourceBarrier);
		}
		d3dCommandList->CopyResource(d3dResourceCopy.Get(), pD3DResource);
		d3dCommandList->Close();

		++fenceValue;
		ID3D12CommandList* d3dCommandLists[] = {d3dCommandList.Get()};
		pD3DQueue->ExecuteCommandLists(ARRAYSIZE(d3dCommandLists), d3dCommandLists);
		VerifyD3DResult(pD3DQueue->Signal(d3dFence.Get(), fenceValue));
		VerifyD3DResult(d3dFence->SetEventOnCompletion(fenceValue, completionEvent));
		WaitForSingleObject(completionEvent, INFINITE);
		CloseHandle(completionEvent);

		SIZE_T numBytes = (SIZE_T)resourceDesc.Width;
		SIZE_T numElements = numBytes / elementSizeInBytes;
		std::vector<u8> byteData(numBytes / sizeof(u8));

		void* pResourceData = nullptr;
		MemoryRange readRange(0, numBytes);
		VerifyD3DResult(d3dResourceCopy->Map(0, &readRange, reinterpret_cast<void**>(&pResourceData)));
		std::memcpy(byteData.data(), pResourceData, numBytes);
		MemoryRange writtenRange(0, 0);
		d3dResourceCopy->Unmap(0, &writtenRange);

		std::stringstream outputStream;
		outputStream << "numElements: " << numElements << "\n";
		for (SIZE_T i = 0; i < numElements; ++i)
		{ 
			const u8* pElementData = byteData.data() + i * elementSizeInBytes;
			outputStream << i << ":\n" << elementFormatter(pElementData) << "\n";
		}
		std::string outputString = outputStream.str();
		OutputDebugStringA(outputString.c_str());
	}
}

enum
{
	kTileSize = 16,
	kNumTilesX = 90,
	kNumTilesY = 60,
	kNumVoxelsInGridX = 256,
	kNumVoxelsInGridY = 256,
	kNumVoxelsInGridZ = 256,
	kBackBufferWidth = kNumTilesX * kTileSize,
	kBackBufferHeight = kNumTilesY * kTileSize,
	kShadowMapTileSize = 512,
};

struct ShadowMapData
{
	Vector2f m_TileTexSpaceSize;
	Vector2f m_NotUsed[31];
};

struct ShadowMapTile
{
	Vector2f m_TexSpaceTopLeftPos;
	Vector2f m_TexSpaceSize;
};

DXApplication::DXApplication(HINSTANCE hApp)
	: Application(hApp, L"Global Illumination", 0, 0, kBackBufferWidth, kBackBufferHeight)
	, m_DisplayResult(DisplayResult::Unknown)
	, m_ShadingMode(TileShadingMode::DirectLight)
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
	, m_pShaderInvisibleSamplerHeap(nullptr)
	, m_pShaderVisibleSRVHeap(nullptr)
	, m_pShaderVisibleSamplerHeap(nullptr)
	, m_pDepthTexture(nullptr)
	, m_pAccumLightTexture(nullptr)
	, m_pSpotLightTiledShadowMap(nullptr)
	, m_pPointLightTiledShadowMap(nullptr)
	, m_pBackBufferViewport(nullptr)
	, m_pSpotLightTiledShadowMapViewport(nullptr)
	, m_pPointLightTiledShadowMapViewport(nullptr)
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
	, m_pPointLightShadowMapDataBuffer(nullptr)
	, m_pPointLightShadowMapTileBuffer(nullptr)
	, m_pPointLightViewTileProjMatrixBuffer(nullptr)
	, m_pRenderEnv(new RenderEnv())
	, m_pFence(nullptr)
	, m_BackBufferIndex(0)
	, m_pPropagateLightPass(nullptr)
	, m_pCreateRenderShadowMapCommandsPass(nullptr)
	, m_pCreateRenderShadowMapCommandsArgumentBuffer(nullptr)
	, m_pRenderSpotLightTiledShadowMapPass(nullptr)
	, m_pRenderPointLightTiledShadowMapPass(nullptr)
	, m_pSetupSpotLightTiledShadowMapPass(nullptr)
	, m_pSetupPointLightTiledShadowMapPass(nullptr)
	, m_pVisualizeSpotLightTiledShadowMapPass(nullptr)
	, m_pVisualizePointLightTiledShadowMapPass(nullptr)
	, m_pCamera(nullptr)
	, m_pMeshRenderResources(nullptr)
	, m_pPointLightRenderResources(nullptr)
	, m_pSpotLightRenderResources(nullptr)
	, m_pMaterialRenderResources(nullptr)
	, m_pGeometryBuffer(nullptr)
	, m_pDownscaleAndReprojectDepthPass(nullptr)
	, m_pFrustumMeshCullingPass(nullptr)
	, m_pFillVisibilityBufferMainPass(nullptr)
	, m_pCreateMainDrawCommandsPass(nullptr)
	, m_pRenderGBufferMainPass(nullptr)
	, m_pFillVisibilityBufferFalseNegativePass(nullptr)
	, m_pCreateFalseNegativeDrawCommandsPass(nullptr)
	, m_pRenderGBufferFalseNegativePass(nullptr)
	, m_pFillMeshTypeDepthBufferPass(nullptr)
	, m_pCalcShadingRectanglesPass(nullptr)
	, m_pFrustumPointLightCullingPass(nullptr)
	, m_pFrustumSpotLightCullingPass(nullptr)
	, m_pTiledLightCullingPass(nullptr)
	, m_pCreateVoxelizeCommandsPass(nullptr)
	, m_pVoxelizePass(nullptr)
	, m_pTiledShadingPass(nullptr)
{
	VerifyD3DResult(CoInitializeEx(nullptr, COINITBASE_MULTITHREADED));

	for (u8 index = 0; index < kNumBackBuffers; ++index)
		m_FrameCompletionFenceValues[index] = m_pRenderEnv->m_LastSubmissionFenceValue;

	::ZeroMemory(m_pVisualizeAccumLightPasses, sizeof(m_pVisualizeAccumLightPasses));
	::ZeroMemory(m_VisualizeNormalBufferPasses, sizeof(m_VisualizeNormalBufferPasses));
	::ZeroMemory(m_VisualizeDepthBufferPasses, sizeof(m_VisualizeDepthBufferPasses));
	::ZeroMemory(m_VisualizeReprojectedDepthBufferPasses, sizeof(m_VisualizeReprojectedDepthBufferPasses));
	::ZeroMemory(m_VisualizeTexCoordBufferPasses, sizeof(m_VisualizeTexCoordBufferPasses));
	::ZeroMemory(m_VisualizeDepthBufferWithMeshTypePasses, sizeof(m_VisualizeDepthBufferWithMeshTypePasses));
	::ZeroMemory(m_VisualizeVoxelReflectancePasses, sizeof(m_VisualizeVoxelReflectancePasses));
	::ZeroMemory(m_pAppDataBuffers, sizeof(m_pAppDataBuffers));
	
	UpdateDisplayResult(DisplayResult::ShadingResult);
}

DXApplication::~DXApplication()
{
	CoUninitialize();

	SafeDelete(m_pCamera);
	SafeDelete(m_pGeometryBuffer);
	SafeDelete(m_pMeshRenderResources);
	SafeDelete(m_pPointLightRenderResources);
	SafeDelete(m_pSpotLightRenderResources);
	SafeDelete(m_pMaterialRenderResources);
	SafeDelete(m_pDownscaleAndReprojectDepthPass);
	SafeDelete(m_pFrustumMeshCullingPass);
	SafeDelete(m_pFillVisibilityBufferMainPass);
	SafeDelete(m_pCreateMainDrawCommandsPass);
	SafeDelete(m_pRenderGBufferMainPass);
	SafeDelete(m_pFillVisibilityBufferFalseNegativePass);
	SafeDelete(m_pCreateFalseNegativeDrawCommandsPass);
	SafeDelete(m_pRenderGBufferFalseNegativePass);
	SafeDelete(m_pFillMeshTypeDepthBufferPass);
	SafeDelete(m_pCalcShadingRectanglesPass);
	SafeDelete(m_pFrustumPointLightCullingPass);
	SafeDelete(m_pFrustumSpotLightCullingPass);
	SafeDelete(m_pTiledLightCullingPass);
	SafeDelete(m_pCreateVoxelizeCommandsPass);
	SafeDelete(m_pVoxelizePass);
	SafeDelete(m_pTiledShadingPass);

	for (u8 index = 0; index < kNumBackBuffers; ++index)
	{
		SafeDelete(m_pVisualizeAccumLightPasses[index]);
		SafeDelete(m_VisualizeNormalBufferPasses[index]);
		SafeDelete(m_VisualizeDepthBufferPasses[index]);
		SafeDelete(m_VisualizeReprojectedDepthBufferPasses[index]);
		SafeDelete(m_VisualizeTexCoordBufferPasses[index]);
		SafeDelete(m_VisualizeDepthBufferWithMeshTypePasses[index]);
		SafeDelete(m_VisualizeVoxelReflectancePasses[index]);

		m_pAppDataBuffers[index]->Unmap(0, sizeof(AppData));
		SafeDelete(m_pAppDataBuffers[index]);
	}
	
	// Old
	SafeDelete(m_pVisualizeSpotLightTiledShadowMapPass);
	SafeDelete(m_pVisualizePointLightTiledShadowMapPass);
	SafeDelete(m_pSpotLightShadowMapDataBuffer);
	SafeDelete(m_pSpotLightShadowMapTileBuffer);
	SafeDelete(m_pSpotLightViewTileProjMatrixBuffer);
	SafeDelete(m_pPointLightShadowMapDataBuffer);
	SafeDelete(m_pPointLightShadowMapTileBuffer);
	SafeDelete(m_pPointLightViewTileProjMatrixBuffer);
	SafeDelete(m_pSetupSpotLightTiledShadowMapPass);
	SafeDelete(m_pSetupPointLightTiledShadowMapPass);
	SafeDelete(m_pCommandListPool);
	SafeDelete(m_pShadowCastingPointLightIndexBuffer);
	SafeDelete(m_pNumShadowCastingPointLightsBuffer);
	SafeDelete(m_pDrawPointLightShadowCasterCommandBuffer);
	SafeDelete(m_pNumDrawPointLightShadowCastersBuffer);
	SafeDelete(m_pShadowCastingSpotLightIndexBuffer);
	SafeDelete(m_pNumShadowCastingSpotLightsBuffer);
	SafeDelete(m_pDrawSpotLightShadowCasterCommandBuffer);
	SafeDelete(m_pNumDrawSpotLightShadowCastersBuffer);
	SafeDelete(m_pPropagateLightPass);
	SafeDelete(m_pCreateRenderShadowMapCommandsPass);
	SafeDelete(m_pCreateRenderShadowMapCommandsArgumentBuffer);
	SafeDelete(m_pRenderSpotLightTiledShadowMapPass);
	SafeDelete(m_pRenderPointLightTiledShadowMapPass);
	SafeDelete(m_pFence);
	SafeDelete(m_pUploadHeapProps);
	SafeDelete(m_pDefaultHeapProps);
	SafeDelete(m_pReadbackHeapProps);
	SafeDelete(m_pShaderInvisibleDSVHeap);
	SafeDelete(m_pShaderInvisibleSRVHeap);
	SafeDelete(m_pShaderInvisibleSamplerHeap);
	SafeDelete(m_pShaderInvisibleRTVHeap);
	SafeDelete(m_pShaderVisibleSRVHeap);
	SafeDelete(m_pShaderVisibleSamplerHeap);
	SafeDelete(m_pRenderEnv);
	SafeDelete(m_pAccumLightTexture);
	SafeDelete(m_pSpotLightTiledShadowMap);
	SafeDelete(m_pPointLightTiledShadowMap);
	SafeDelete(m_pDepthTexture);
	SafeDelete(m_pCommandQueue);
	SafeDelete(m_pCommandListPool);
	SafeDelete(m_pSwapChain);
	SafeDelete(m_pDevice);
	SafeDelete(m_pBackBufferViewport);
	SafeDelete(m_pSpotLightTiledShadowMapViewport);
	SafeDelete(m_pPointLightTiledShadowMapViewport);
}

void DXApplication::OnInit()
{
	InitRenderEnv(kBackBufferWidth, kBackBufferHeight);
	
	Scene* pScene = SceneLoader::LoadSponza();
	InitScene(pScene, kBackBufferWidth, kBackBufferHeight);
	
	InitDownscaleAndReprojectDepthPass();
	InitFrustumMeshCullingPass();
	
	InitFillVisibilityBufferMainPass();
	InitCreateMainDrawCommandsPass();
	InitRenderGBufferMainPass(kBackBufferWidth, kBackBufferHeight);
	InitFillVisibilityBufferFalseNegativePass();
	InitCreateFalseNegativeDrawCommandsPass();
	InitRenderGBufferFalseNegativePass(kBackBufferWidth, kBackBufferHeight);
	InitCalcShadingRectanglesPass();
	InitFillMeshTypeDepthBufferPass();
		
	if (m_pPointLightRenderResources != nullptr)
		InitFrustumPointLightCullingPass();
	if (m_pSpotLightRenderResources != nullptr)
		InitFrustumSpotLightCullingPass();
	InitTiledLightCullingPass();
	
	InitCreateVoxelizeCommandsPass();
	InitVoxelizePass();

	InitTiledShadingPass();

	InitVisualizeAccumLightPass();
	InitVisualizeDepthBufferPass();
	InitVisualizeReprojectedDepthBufferPass();
	InitVisualizeNormalBufferPass();
	InitVisualizeTexCoordBufferPass();
	InitVisualizeDepthBufferWithMeshTypePass();
	InitVisualizeVoxelReflectancePass();
		
	SafeDelete(pScene);

	/*
	InitCreateRenderShadowMapCommandsPass();
	
	if (m_pPointLightRenderResources != nullptr)
	{
		InitSetupPointLightTiledShadowMapPass();
		InitRenderPointLightTiledShadowMapPass();
	}
	if (m_pSpotLightRenderResources != nullptr)
	{
		InitSetupSpotLightTiledShadowMapPass();
		InitRenderSpotLightTiledShadowMapPass();
	}

	InitPropagateLightPass();
	InitVisualizePointLightTiledShadowMapPass();
	InitVisualizeIntensityPass();
	*/
}

void DXApplication::OnUpdate()
{
	const Vector3f& cameraWorldSpacePos = m_pCamera->GetTransform().GetPosition();
	
	AppData appData;
	appData.m_ViewMatrix = m_pCamera->GetViewMatrix();
	appData.m_ViewInvMatrix = Inverse(appData.m_ViewMatrix);
	appData.m_ProjMatrix = m_pCamera->GetProjMatrix();
	appData.m_ProjInvMatrix = Inverse(appData.m_ProjMatrix);
	appData.m_ViewProjMatrix = m_pCamera->GetViewMatrix() * m_pCamera->GetProjMatrix();
	appData.m_ViewProjInvMatrix = Inverse(appData.m_ViewProjMatrix);
	appData.m_PrevViewProjMatrix = appData.m_ViewProjMatrix;
	appData.m_PrevViewProjInvMatrix = Inverse(appData.m_PrevViewProjMatrix);

	appData.m_CameraWorldSpacePos = Vector4f(cameraWorldSpacePos.m_X, cameraWorldSpacePos.m_Y, cameraWorldSpacePos.m_Z, 1.0f);
	const Frustum cameraWorldFrustum = ExtractWorldFrustum(*m_pCamera);
	for (u8 planeIndex = 0; planeIndex < Frustum::NumPlanes; ++planeIndex)
		appData.m_CameraWorldFrustumPlanes[planeIndex] = ToVector(cameraWorldFrustum.m_Planes[planeIndex]);

	appData.m_CameraNearPlane = m_pCamera->GetNearClipPlane();
	appData.m_CameraFarPlane = m_pCamera->GetFarClipPlane();
	appData.m_ScreenSize = Vector2u(kBackBufferWidth, kBackBufferHeight);
	appData.m_RcpScreenSize = Vector2f(1.0f / f32(appData.m_ScreenSize.m_X), 1.0f / f32(appData.m_ScreenSize.m_Y));
	appData.m_ScreenHalfSize = Vector2u(appData.m_ScreenSize.m_X >> 1, appData.m_ScreenSize.m_Y >> 1);
	appData.m_RcpScreenHalfSize = Vector2f(1.0f / f32(appData.m_ScreenHalfSize.m_X), 1.0f / f32(appData.m_ScreenHalfSize.m_Y));
	appData.m_ScreenQuarterSize = Vector2u(appData.m_ScreenHalfSize.m_X >> 1, appData.m_ScreenHalfSize.m_Y >> 1);
	appData.m_RcpScreenQuarterSize = Vector2f(1.0f / f32(appData.m_ScreenQuarterSize.m_X), 1.0f / f32(appData.m_ScreenQuarterSize.m_Y));
	appData.m_SunWorldSpaceDir = Vector4f(0.0f, 0.0f, 1.0f, 0.0f);
	appData.m_SunLightColor = Color::WHITE;
	appData.m_ScreenTileSize = Vector2u(kTileSize, kTileSize);
	appData.m_NumScreenTiles = Vector2u(kNumTilesX, kNumTilesY);

	const AxisAlignedBox cameraWorldAABB(Frustum::NumCorners, cameraWorldFrustum.m_Corners);

	const Vector3f voxelGridWorldCenter = cameraWorldAABB.m_Center;
	const Vector3f voxelGridWorldRadius(512.0f, 512.0f, 512.0f);
	const Vector3f numVoxelsInGrid(kNumVoxelsInGridX, kNumVoxelsInGridY, kNumVoxelsInGridZ);
	const Vector3f voxelGridWorldSize = 2.0f * voxelGridWorldRadius;
	
	appData.m_VoxelGridWorldMinPoint = voxelGridWorldCenter - voxelGridWorldRadius;
	appData.m_VoxelGridWorldMaxPoint = voxelGridWorldCenter + voxelGridWorldRadius;
	appData.m_VoxelRcpSize = numVoxelsInGrid / voxelGridWorldSize;
	
	Camera voxelGridCameraAlongX(Camera::ProjType_Ortho, 0.0f, voxelGridWorldSize.m_X, voxelGridWorldSize.m_Z / voxelGridWorldSize.m_Y);
	voxelGridCameraAlongX.SetSizeY(voxelGridWorldSize.m_Y);
	voxelGridCameraAlongX.GetTransform().SetPosition(voxelGridWorldCenter - voxelGridWorldRadius * Vector3f::RIGHT);
	voxelGridCameraAlongX.GetTransform().SetRotation(CreateRotationYQuaternion(PI_DIV_2));

	Camera voxelGridCameraAlongY(Camera::ProjType_Ortho, 0.0f, voxelGridWorldSize.m_Y, voxelGridWorldSize.m_X / voxelGridWorldSize.m_Z);
	voxelGridCameraAlongY.SetSizeY(voxelGridWorldSize.m_Z);
	voxelGridCameraAlongY.GetTransform().SetPosition(voxelGridWorldCenter - voxelGridWorldRadius * Vector3f::UP);
	voxelGridCameraAlongY.GetTransform().SetRotation(CreateRotationXQuaternion(-PI_DIV_2));
	
	Camera voxelGridCameraAlongZ(Camera::ProjType_Ortho, 0.0f, voxelGridWorldSize.m_Z, voxelGridWorldSize.m_X / voxelGridWorldSize.m_Y);
	voxelGridCameraAlongZ.SetSizeY(voxelGridWorldSize.m_Y);
	voxelGridCameraAlongZ.GetTransform().SetPosition(voxelGridWorldCenter - voxelGridWorldRadius * Vector3f::FORWARD);

	appData.m_VoxelGridViewProjMatrices[0] = voxelGridCameraAlongX.GetViewMatrix() * voxelGridCameraAlongX.GetProjMatrix();
	appData.m_VoxelGridViewProjMatrices[1] = voxelGridCameraAlongY.GetViewMatrix() * voxelGridCameraAlongY.GetProjMatrix();
	appData.m_VoxelGridViewProjMatrices[2] = voxelGridCameraAlongZ.GetViewMatrix() * voxelGridCameraAlongZ.GetProjMatrix();

	std::memcpy(m_pAppDataPointers[m_BackBufferIndex], &appData, sizeof(appData));
}

void DXApplication::OnRender()
{
	static CommandList* commandListBatch[MAX_NUM_COMMAND_LISTS_IN_BATCH];
	
	u8 commandListBatchSize = 0;
	commandListBatch[commandListBatchSize++] = RecordDownscaleAndReprojectDepthPass();
	commandListBatch[commandListBatchSize++] = RecordClearResourcesPass();
	commandListBatch[commandListBatchSize++] = RecordFrustumMeshCullingPass();
	commandListBatch[commandListBatchSize++] = RecordFillVisibilityBufferMainPass();
	commandListBatch[commandListBatchSize++] = RecordCreateMainDrawCommandsPass();
	commandListBatch[commandListBatchSize++] = RecordRenderGBufferMainPass();
	commandListBatch[commandListBatchSize++] = RecordFillVisibilityBufferFalseNegativePass();
	commandListBatch[commandListBatchSize++] = RecordCreateFalseNegativeDrawCommandsPass();
	commandListBatch[commandListBatchSize++] = RecordRenderGBufferFalseNegativePass();
	commandListBatch[commandListBatchSize++] = RecordCalcShadingRectanglesPass();
	commandListBatch[commandListBatchSize++] = RecordFillMeshTypeDepthBufferPass();
	
	if (m_pFrustumPointLightCullingPass != nullptr)
		commandListBatch[commandListBatchSize++] = RecordFrustumPointLightCullingPass();
	if (m_pFrustumSpotLightCullingPass != nullptr)
		commandListBatch[commandListBatchSize++] = RecordFrustumSpotLightCullingPass();
	commandListBatch[commandListBatchSize++] = RecordTiledLightCullingPass();
	
	commandListBatch[commandListBatchSize++] = RecordCreateVoxelizeCommandsPass();
	commandListBatch[commandListBatchSize++] = RecordVoxelizePass();

	commandListBatch[commandListBatchSize++] = RecordTiledShadingPass();
	commandListBatch[commandListBatchSize++] = RecordDisplayResultPass();
	commandListBatch[commandListBatchSize++] = RecordPostRenderPass();
				
	++m_pRenderEnv->m_LastSubmissionFenceValue;
	m_pCommandQueue->ExecuteCommandLists(commandListBatchSize, commandListBatch, m_pFence, m_pRenderEnv->m_LastSubmissionFenceValue);
	ColorTexture* pRenderTarget = m_pSwapChain->GetBackBuffer(m_BackBufferIndex);

	++m_pRenderEnv->m_LastSubmissionFenceValue;
	m_pSwapChain->Present(1, 0);
	m_pCommandQueue->Signal(m_pFence, m_pRenderEnv->m_LastSubmissionFenceValue);

#ifdef DEBUG_RENDER_PASS
	m_pFence->WaitForSignalOnCPU(m_pRenderEnv->m_LastSubmissionFenceValue);
	OuputDebugRenderPassResult();
#endif // DEBUG_RENDER_PASS

	m_FrameCompletionFenceValues[m_BackBufferIndex] = m_pRenderEnv->m_LastSubmissionFenceValue;
	m_BackBufferIndex = m_pSwapChain->GetCurrentBackBufferIndex();
	m_pFence->WaitForSignalOnCPU(m_FrameCompletionFenceValues[m_BackBufferIndex]);
	
	/*
	if ((m_pPointLightRenderResources != nullptr) || (m_pSpotLightRenderResources != nullptr))
	{
		submissionBatch.emplace_back(RecordUpdateCreateRenderShadowMapCommandsArgumentBufferPass());
		submissionBatch.emplace_back(RecordCreateRenderShadowMapCommandsPass());
	}		
	if (m_pPointLightRenderResources != nullptr)
	{
		submissionBatch.emplace_back(RecordSetupPointLightTiledShadowMapPass());
		submissionBatch.emplace_back(RecordRenderPointLightTiledShadowMapPass());
	}	
	if (m_pSpotLightRenderResources != nullptr)
	{
		submissionBatch.emplace_back(RecordSetupSpotLightTiledShadowMapPass());
		submissionBatch.emplace_back(RecordRenderSpotLightTiledShadowMapPass());
	}
	if (m_NumPropagationIterations > 0)
		submissionBatch.emplace_back(RecordPropagateLightPass());

	submissionBatch.emplace_back(RecordTiledShadingPass());
	submissionBatch.emplace_back(RecordDisplayResultPass());
	*/
}

void DXApplication::OnDestroy()
{
	m_pCommandQueue->Signal(m_pFence, m_pRenderEnv->m_LastSubmissionFenceValue);
	m_pFence->WaitForSignalOnCPU(m_pRenderEnv->m_LastSubmissionFenceValue);
}

void DXApplication::OnKeyDown(UINT8 key)
{
	const f32 cameraMoveSpeed = 1.0f;
	const f32 rotationInDegrees = 2.0f;

	Transform& cameraTransform = m_pCamera->GetTransform();
	BasisAxes cameraBasisAxes = ExtractBasisAxes(cameraTransform.GetRotation());

	switch (key)
	{
		case VK_UP:
		{
			cameraTransform.SetRotation(CreateRotationXQuaternion(ToRadians(-rotationInDegrees)) * cameraTransform.GetRotation());
			break;
		}
		case VK_DOWN:
		{
			cameraTransform.SetRotation(CreateRotationXQuaternion(ToRadians(rotationInDegrees)) * cameraTransform.GetRotation());
			break;
		}
		case VK_LEFT:
		{
			cameraTransform.SetRotation(CreateRotationYQuaternion(ToRadians(-rotationInDegrees)) * cameraTransform.GetRotation());
			break;
		}
		case VK_RIGHT:
		{
			cameraTransform.SetRotation(CreateRotationYQuaternion(ToRadians(rotationInDegrees)) * cameraTransform.GetRotation());
			break;
		}

		case 'a':
		case 'A':
		{
			cameraTransform.SetPosition(cameraTransform.GetPosition() - cameraMoveSpeed * cameraBasisAxes.m_XAxis);
			break;
		}
		case 'd':
		case 'D':
		{
			cameraTransform.SetPosition(cameraTransform.GetPosition() + cameraMoveSpeed * cameraBasisAxes.m_XAxis);
			break;
		}
		case 'w':
		case 'W':
		{
			cameraTransform.SetPosition(cameraTransform.GetPosition() + cameraMoveSpeed * cameraBasisAxes.m_ZAxis);
			break;
		}
		case 's':
		case 'S':
		{
			cameraTransform.SetPosition(cameraTransform.GetPosition() - cameraMoveSpeed * cameraBasisAxes.m_ZAxis);
			break;
		}
		case 'q':
		case 'Q':
		{
			cameraTransform.SetPosition(cameraTransform.GetPosition() + cameraMoveSpeed * cameraBasisAxes.m_YAxis);
			break;
		}
		case 'e':
		case 'E':
		{
			cameraTransform.SetPosition(cameraTransform.GetPosition() - cameraMoveSpeed * cameraBasisAxes.m_YAxis);
			break;
		}
				
		case '1':
		{
			UpdateDisplayResult(DisplayResult::ShadingResult);
			break;
		}
		case '2':
		{
			UpdateDisplayResult(DisplayResult::DepthBuffer);
			break;
		}
		case '3':
		{
			UpdateDisplayResult(DisplayResult::ReprojectedDepthBuffer);
			break;
		}
		case '4':
		{
			UpdateDisplayResult(DisplayResult::NormalBuffer);
			break;
		}
		case '5':
		{
			UpdateDisplayResult(DisplayResult::TexCoordBuffer);
			break;
		}
		case '6':
		{
			UpdateDisplayResult(DisplayResult::DepthBufferWithMeshType);
			break;
		}
		case '7':
		{
			UpdateDisplayResult(DisplayResult::VoxelRelectance);
			break;
		}
	}

#if 0
	const auto& pos = cameraTransform.GetPosition();
	const auto& rot = cameraTransform.GetRotation();
	
	std::stringstream stream;
	stream << "Position: "
		<< pos.m_X << ", "
		<< pos.m_Y << ", "
		<< pos.m_Z << ", "
		<< " rotation: "
		<< rot.m_X << ", "
		<< rot.m_Y << ", "
		<< rot.m_Z << ", "
		<< rot.m_W << "\n";

	OutputDebugStringA(stream.str().c_str());
#endif
}

void DXApplication::OnKeyUp(UINT8 key)
{
}

void DXApplication::InitRenderEnv(UINT backBufferWidth, UINT backBufferHeight)
{
	GraphicsFactory factory;
	m_pDevice = new GraphicsDevice(&factory, D3D_FEATURE_LEVEL_11_0);

	CommandQueueDesc commandQueueDesc(D3D12_COMMAND_LIST_TYPE_DIRECT);
	m_pCommandQueue = new CommandQueue(m_pDevice, &commandQueueDesc, L"m_pCommandQueue");

	m_pCommandListPool = new CommandListPool(m_pDevice, D3D12_COMMAND_LIST_TYPE_DIRECT);

	DescriptorHeapDesc rtvHeapDesc(D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 16, false);
	m_pShaderInvisibleRTVHeap = new DescriptorHeap(m_pDevice, &rtvHeapDesc, L"m_pShaderInvisibleRTVHeap");

	DescriptorHeapDesc shaderVisibleSRVHeapDesc(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 512, true);
	m_pShaderVisibleSRVHeap = new DescriptorHeap(m_pDevice, &shaderVisibleSRVHeapDesc, L"m_pShaderVisibleSRVHeap");

	DescriptorHeapDesc shaderInvisibleSRVHeapDesc(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 512, false);
	m_pShaderInvisibleSRVHeap = new DescriptorHeap(m_pDevice, &shaderInvisibleSRVHeapDesc, L"m_pShaderInvisibleSRVHeap");

	DescriptorHeapDesc shaderVisibleSamplerHeapDesc(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, 10, true);
	m_pShaderVisibleSamplerHeap = new DescriptorHeap(m_pDevice, &shaderVisibleSamplerHeapDesc, L"m_pShaderVisibleSamplerHeap");

	DescriptorHeapDesc shaderInvisibleSamplerHeapDesc(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, 10, false);
	m_pShaderInvisibleSamplerHeap = new DescriptorHeap(m_pDevice, &shaderInvisibleSamplerHeapDesc, L"m_pShaderInvisibleSamplerHeap");

	DescriptorHeapDesc dsvHeapDesc(D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 8, false);
	m_pShaderInvisibleDSVHeap = new DescriptorHeap(m_pDevice, &dsvHeapDesc, L"m_pShaderInvisibleDSVHeap");

	m_pBackBufferViewport = new Viewport(0.0f, 0.0f, (FLOAT)backBufferWidth, (FLOAT)backBufferHeight);
	m_pFence = new Fence(m_pDevice, m_pRenderEnv->m_LastSubmissionFenceValue, L"m_pFence");

	m_pRenderEnv->m_pDevice = m_pDevice;
	m_pRenderEnv->m_pCommandQueue = m_pCommandQueue;
	m_pRenderEnv->m_pFence = m_pFence;
	m_pRenderEnv->m_pCommandListPool = m_pCommandListPool;
	m_pRenderEnv->m_pUploadHeapProps = m_pUploadHeapProps;
	m_pRenderEnv->m_pDefaultHeapProps = m_pDefaultHeapProps;
	m_pRenderEnv->m_pReadbackHeapProps = m_pReadbackHeapProps;
	m_pRenderEnv->m_pShaderInvisibleRTVHeap = m_pShaderInvisibleRTVHeap;
	m_pRenderEnv->m_pShaderInvisibleSRVHeap = m_pShaderInvisibleSRVHeap;
	m_pRenderEnv->m_pShaderInvisibleDSVHeap = m_pShaderInvisibleDSVHeap;
	m_pRenderEnv->m_pShaderInvisibleSamplerHeap = m_pShaderInvisibleSamplerHeap;
	m_pRenderEnv->m_pShaderVisibleSRVHeap = m_pShaderVisibleSRVHeap;
	m_pRenderEnv->m_pShaderVisibleSamplerHeap = m_pShaderVisibleSamplerHeap;

	SwapChainDesc swapChainDesc(kNumBackBuffers, m_pWindow->GetHWND(), backBufferWidth, backBufferHeight);
	m_pSwapChain = new SwapChain(&factory, m_pRenderEnv, &swapChainDesc, m_pCommandQueue);
	m_BackBufferIndex = m_pSwapChain->GetCurrentBackBufferIndex();

	GeometryBuffer::InitParams params;
	params.m_pRenderEnv = m_pRenderEnv;
	params.m_BufferWidth = backBufferWidth;
	params.m_BufferHeight = backBufferHeight;
	params.m_InputResourceStates.m_TexCoordTextureState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	params.m_InputResourceStates.m_NormalTextureState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	params.m_InputResourceStates.m_MaterialIDTextureState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;

	m_pGeometryBuffer = new GeometryBuffer(&params);

	DepthStencilValue optimizedClearDepth(1.0f);
	DepthTexture2DDesc depthTexDesc(DXGI_FORMAT_R32_TYPELESS, backBufferWidth, backBufferHeight, true, true);
	m_pDepthTexture = new DepthTexture(m_pRenderEnv, m_pRenderEnv->m_pDefaultHeapProps, &depthTexDesc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE, &optimizedClearDepth, L"m_pDepthTexture");

	const FLOAT optimizedClearColor[] = {0.0f, 0.0f, 0.0f, 0.0f};
	ColorTexture2DDesc accumLightTexDesc(DXGI_FORMAT_R10G10B10A2_UNORM, backBufferWidth, backBufferHeight, true, true, false);
	m_pAccumLightTexture = new ColorTexture(m_pRenderEnv, m_pRenderEnv->m_pDefaultHeapProps, &accumLightTexDesc,
		D3D12_RESOURCE_STATE_RENDER_TARGET, optimizedClearColor, L"m_pAccumLightTexture");

	ConstantBufferDesc appDataBufferDesc(sizeof(AppData));
	for (u8 index = 0; index < kNumBackBuffers; ++index)
	{
		m_pAppDataBuffers[index] = new Buffer(m_pRenderEnv, m_pRenderEnv->m_pUploadHeapProps, &appDataBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, L"m_pAppDataBuffer");
		m_pAppDataPointers[index] = m_pAppDataBuffers[index]->Map(0, 0);
	}
	
	ResourceBarrier initResourceBarrier(m_pDepthTexture, D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	CommandList* pInitCommandList = m_pCommandListPool->Create(L"pInitResourcesCommandList");
	pInitCommandList->Begin();
	pInitCommandList->ClearDepthView(m_pDepthTexture->GetDSVHandle(), 1.0f);
	pInitCommandList->ResourceBarrier(1, &initResourceBarrier);
	pInitCommandList->End();

	++m_pRenderEnv->m_LastSubmissionFenceValue;
	m_pCommandQueue->ExecuteCommandLists(1, &pInitCommandList, m_pFence, m_pRenderEnv->m_LastSubmissionFenceValue);
	m_pFence->WaitForSignalOnCPU(m_pRenderEnv->m_LastSubmissionFenceValue);
}

void DXApplication::InitScene(Scene* pScene, UINT backBufferWidth, UINT backBufferHeight)
{
	f32 aspectRatio = FLOAT(backBufferWidth) / FLOAT(backBufferHeight);

	m_pCamera = new Camera(Camera::ProjType_Perspective, pScene->GetCamera()->GetNearClipPlane(), pScene->GetCamera()->GetFarClipPlane(), aspectRatio);	
	m_pCamera->GetTransform().SetPosition(pScene->GetCamera()->GetTransform().GetPosition());
	m_pCamera->GetTransform().SetRotation(pScene->GetCamera()->GetTransform().GetRotation());
			
	if (pScene->GetNumMeshBatches() > 0)
		m_pMeshRenderResources = new MeshRenderResources(m_pRenderEnv, pScene->GetNumMeshBatches(), pScene->GetMeshBatches());

	if (pScene->GetNumPointLights() > 0)
		m_pPointLightRenderResources = new LightRenderResources(m_pRenderEnv, pScene->GetNumPointLights(), pScene->GetPointLights());

	if (pScene->GetNumSpotLights() > 0)
		m_pSpotLightRenderResources = new LightRenderResources(m_pRenderEnv, pScene->GetNumSpotLights(), pScene->GetSpotLights());

	if (pScene->GetNumMaterials() > 0)
		m_pMaterialRenderResources = new MaterialRenderResources(m_pRenderEnv, pScene->GetNumMaterials(), pScene->GetMaterials());
		
	//Kolya. Fix me
	//Vector3u shadowMapCommandsArgumentBufferValues(0, 1, 1);
	//StructuredBufferDesc shadowMapCommandsArgumentBufferDesc(1, sizeof(shadowMapCommandsArgumentBufferValues), false, false);
	//m_pCreateRenderShadowMapCommandsArgumentBuffer = new Buffer(m_pRenderEnv, m_pRenderEnv->m_pDefaultHeapProps, &shadowMapCommandsArgumentBufferDesc, D3D12_RESOURCE_STATE_COPY_DEST, L"m_pCreateRenderShadowMapCommandsArgumentBuffer");
	//UploadData(m_pRenderEnv, m_pCreateRenderShadowMapCommandsArgumentBuffer, &shadowMapCommandsArgumentBufferDesc, &shadowMapCommandsArgumentBufferValues, sizeof(shadowMapCommandsArgumentBufferValues));
}

void DXApplication::InitDownscaleAndReprojectDepthPass()
{
	assert(m_pDownscaleAndReprojectDepthPass == nullptr);

	DownscaleAndReprojectDepthPass::InitParams params;
	params.m_pRenderEnv = m_pRenderEnv;
	params.m_InputResourceStates.m_PrevDepthTextureState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	params.m_InputResourceStates.m_ReprojectedDepthTextureState = D3D12_RESOURCE_STATE_DEPTH_WRITE;
	params.m_pPrevDepthTexture = m_pDepthTexture;

	m_pDownscaleAndReprojectDepthPass = new DownscaleAndReprojectDepthPass(&params);
}

CommandList* DXApplication::RecordDownscaleAndReprojectDepthPass()
{
	assert(m_pDownscaleAndReprojectDepthPass != nullptr);

	DownscaleAndReprojectDepthPass::RenderParams params;
	params.m_pRenderEnv = m_pRenderEnv;
	params.m_pCommandList = m_pCommandListPool->Create(L"pDownscaleAndReprojectDepthCommandList");
	params.m_pAppDataBuffer = m_pAppDataBuffers[m_BackBufferIndex];

	m_pDownscaleAndReprojectDepthPass->Record(&params);
	return params.m_pCommandList;
}

CommandList* DXApplication::RecordClearResourcesPass()
{
	const FLOAT clearColor[] = {0.0f, 0.0f, 0.0f, 0.0f};
		
	CommandList* pCommandList = m_pCommandListPool->Create(L"pClearResourcesCommandList");
	pCommandList->Begin();
	pCommandList->ClearRenderTargetView(m_pAccumLightTexture->GetRTVHandle(), clearColor);
	pCommandList->End();
	
	return pCommandList;
}

void DXApplication::InitFrustumMeshCullingPass()
{
	assert(m_pMeshRenderResources != nullptr);
	assert(m_pFrustumMeshCullingPass == nullptr);

	FrustumMeshCullingPass::InitParams params;
	params.m_pRenderEnv = m_pRenderEnv;
	params.m_pInstanceWorldAABBBuffer = m_pMeshRenderResources->GetInstanceWorldAABBBuffer();
	params.m_pMeshInfoBuffer = m_pMeshRenderResources->GetMeshInfoBuffer();
	params.m_MaxNumMeshes = m_pMeshRenderResources->GetTotalNumMeshes();
	params.m_MaxNumInstances = m_pMeshRenderResources->GetTotalNumInstances();
	params.m_MaxNumInstancesPerMesh = m_pMeshRenderResources->GetMaxNumInstancesPerMesh();

	params.m_InputResourceStates.m_MeshInfoBufferState = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
	params.m_InputResourceStates.m_InstanceWorldAABBBufferState = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
	params.m_InputResourceStates.m_NumVisibleMeshesBufferState = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
	params.m_InputResourceStates.m_VisibleMeshInfoBufferState = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
	params.m_InputResourceStates.m_VisibleInstanceIndexBufferState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
	params.m_InputResourceStates.m_NumVisibleInstancesBufferState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;

	m_pFrustumMeshCullingPass = new FrustumMeshCullingPass(&params);
}

CommandList* DXApplication::RecordFrustumMeshCullingPass()
{
	assert(m_pFrustumMeshCullingPass != nullptr);

	FrustumMeshCullingPass::RenderParams params;
	params.m_pRenderEnv = m_pRenderEnv;
	params.m_pCommandList = m_pCommandListPool->Create(L"pFrustumMeshCullingCommandList");
	params.m_pAppDataBuffer = m_pAppDataBuffers[m_BackBufferIndex];

	m_pFrustumMeshCullingPass->Record(&params);
	return params.m_pCommandList;
}

void DXApplication::InitFillVisibilityBufferMainPass()
{
	assert(m_pFillVisibilityBufferMainPass == nullptr);
	assert(m_pDownscaleAndReprojectDepthPass != nullptr);
	assert(m_pFrustumMeshCullingPass != nullptr);
	assert(m_pMeshRenderResources);
		
	FillVisibilityBufferPass::InitParams params;
	params.m_pRenderEnv = m_pRenderEnv;

	params.m_InputResourceStates.m_InstanceIndexBufferState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
	params.m_InputResourceStates.m_InstanceWorldOBBMatrixBufferState = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
	params.m_InputResourceStates.m_NumInstancesBufferState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
	params.m_InputResourceStates.m_DepthTextureState = D3D12_RESOURCE_STATE_DEPTH_WRITE;
	params.m_InputResourceStates.m_VisibilityBufferState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
	
	params.m_pInstanceIndexBuffer = m_pFrustumMeshCullingPass->GetVisibleInstanceIndexBuffer();
	params.m_pNumInstancesBuffer = m_pFrustumMeshCullingPass->GetNumVisibleInstancesBuffer();
	params.m_pInstanceWorldOBBMatrixBuffer = m_pMeshRenderResources->GetInstanceWorldOBBMatrixBuffer();
	params.m_pDepthTexture = m_pDownscaleAndReprojectDepthPass->GetReprojectedDepthTexture();
	params.m_ClampVerticesBehindCameraNearPlane = true;
	params.m_MaxNumInstances = m_pMeshRenderResources->GetTotalNumInstances();

	m_pFillVisibilityBufferMainPass = new FillVisibilityBufferPass(&params);
}

CommandList* DXApplication::RecordFillVisibilityBufferMainPass()
{
	assert(m_pFillVisibilityBufferMainPass != nullptr);
	assert(m_pFrustumMeshCullingPass != nullptr);
	
	FillVisibilityBufferPass::RenderParams params;
	params.m_pRenderEnv = m_pRenderEnv;
	params.m_pCommandList = m_pCommandListPool->Create(L"pFillVisibilityBufferMainCommandList");
	params.m_pAppDataBuffer = m_pAppDataBuffers[m_BackBufferIndex];
	params.m_pNumInstancesBuffer = m_pFrustumMeshCullingPass->GetNumVisibleInstancesBuffer();

	m_pFillVisibilityBufferMainPass->Record(&params);
	return params.m_pCommandList;
}

void DXApplication::InitCreateMainDrawCommandsPass()
{
	assert(m_pFrustumMeshCullingPass != nullptr);
	assert(m_pFillVisibilityBufferMainPass != nullptr);
	assert(m_pCreateMainDrawCommandsPass == nullptr);

	const FrustumMeshCullingPass::ResourceStates* pFrustumMeshCullingPassStates = 
		m_pFrustumMeshCullingPass->GetOutputResourceStates();
	
	const FillVisibilityBufferPass::ResourceStates* pFillVisibilityBufferMainPassStates =
		m_pFillVisibilityBufferMainPass->GetOutputResourceStates();

	CreateMainDrawCommandsPass::InitParams params;
	params.m_pRenderEnv = m_pRenderEnv;

	params.m_InputResourceStates.m_NumMeshesBufferState = pFrustumMeshCullingPassStates->m_NumVisibleMeshesBufferState;
	params.m_InputResourceStates.m_MeshInfoBufferState = pFrustumMeshCullingPassStates->m_VisibleMeshInfoBufferState;
	params.m_InputResourceStates.m_InstanceIndexBufferState = pFillVisibilityBufferMainPassStates->m_InstanceIndexBufferState;
	params.m_InputResourceStates.m_VisibilityBufferState = pFillVisibilityBufferMainPassStates->m_VisibilityBufferState;
	params.m_InputResourceStates.m_VisibleInstanceIndexBufferState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
	params.m_InputResourceStates.m_NumVisibleMeshesPerTypeBufferState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
	params.m_InputResourceStates.m_DrawCommandBufferState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
	params.m_InputResourceStates.m_NumOccludedInstancesBufferState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
	params.m_InputResourceStates.m_OccludedInstanceIndexBufferState = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;

	params.m_pNumMeshesBuffer = m_pFrustumMeshCullingPass->GetNumVisibleMeshesBuffer();
	params.m_pMeshInfoBuffer = m_pFrustumMeshCullingPass->GetVisibleMeshInfoBuffer();
	params.m_pInstanceIndexBuffer = m_pFrustumMeshCullingPass->GetVisibleInstanceIndexBuffer();
	params.m_pVisibilityBuffer = m_pFillVisibilityBufferMainPass->GetVisibilityBuffer();
	params.m_NumMeshTypes = m_pMeshRenderResources->GetNumMeshTypes();
	params.m_MaxNumMeshes = m_pMeshRenderResources->GetTotalNumMeshes();
	params.m_MaxNumInstances = m_pMeshRenderResources->GetTotalNumInstances();
	params.m_MaxNumInstancesPerMesh = m_pMeshRenderResources->GetMaxNumInstancesPerMesh();

	m_pCreateMainDrawCommandsPass = new CreateMainDrawCommandsPass(&params);
}

CommandList* DXApplication::RecordCreateMainDrawCommandsPass()
{
	assert(m_pCreateMainDrawCommandsPass != nullptr);
	
	CreateMainDrawCommandsPass::RenderParams params;
	params.m_pRenderEnv = m_pRenderEnv;
	params.m_pCommandList = m_pCommandListPool->Create(L"pCreateMainDrawCommandsList");
	params.m_pNumMeshesBuffer = m_pFrustumMeshCullingPass->GetNumVisibleMeshesBuffer();

	m_pCreateMainDrawCommandsPass->Record(&params);
	return params.m_pCommandList;
}

void DXApplication::InitRenderGBufferMainPass(UINT bufferWidth, UINT bufferHeight)
{
	assert(m_pGeometryBuffer != nullptr);
	assert(m_pMeshRenderResources != nullptr);
	assert(m_pCreateMainDrawCommandsPass != nullptr);
	assert(m_pRenderGBufferMainPass == nullptr);

	const GeometryBuffer::ResourceStates* pGeometryBufferStates =
		m_pGeometryBuffer->GetOutputResourceStates();

	const CreateMainDrawCommandsPass::ResourceStates* pCreateMainDrawCommandsPassStates =
		m_pCreateMainDrawCommandsPass->GetOutputResourceStates();

	const DownscaleAndReprojectDepthPass::ResourceStates* pDownscaleAndReprojectDepthPassStates =
		m_pDownscaleAndReprojectDepthPass->GetOutputResourceStates();

	RenderGBufferPass::InitParams params;
	params.m_pRenderEnv = m_pRenderEnv;
	params.m_BufferWidth = bufferWidth;
	params.m_BufferHeight = bufferHeight;

	params.m_InputResourceStates.m_TexCoordTextureState = pGeometryBufferStates->m_TexCoordTextureState;
	params.m_InputResourceStates.m_NormalTextureState = pGeometryBufferStates->m_NormalTextureState;
	params.m_InputResourceStates.m_MaterialIDTextureState = pGeometryBufferStates->m_MaterialIDTextureState;
	params.m_InputResourceStates.m_DepthTextureState = pDownscaleAndReprojectDepthPassStates->m_PrevDepthTextureState;
	params.m_InputResourceStates.m_InstanceWorldMatrixBufferState = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
	params.m_InputResourceStates.m_InstanceIndexBufferState = pCreateMainDrawCommandsPassStates->m_VisibleInstanceIndexBufferState;
	params.m_InputResourceStates.m_NumVisibleMeshesPerTypeBufferState = pCreateMainDrawCommandsPassStates->m_NumVisibleMeshesPerTypeBufferState;
	params.m_InputResourceStates.m_DrawCommandBufferState = pCreateMainDrawCommandsPassStates->m_DrawCommandBufferState;

	params.m_pMeshRenderResources = m_pMeshRenderResources;
	params.m_pTexCoordTexture = m_pGeometryBuffer->GetTexCoordTexture();
	params.m_pNormalTexture = m_pGeometryBuffer->GetNormalTexture();
	params.m_pMaterialIDTexture = m_pGeometryBuffer->GetMaterialIDTexture();
	params.m_pDepthTexture = m_pDepthTexture;
	params.m_pInstanceWorldMatrixBuffer = m_pMeshRenderResources->GetInstanceWorldMatrixBuffer();
	params.m_pInstanceIndexBuffer = m_pCreateMainDrawCommandsPass->GetVisibleInstanceIndexBuffer();
	params.m_NumVisibleMeshesPerTypeBuffer = m_pCreateMainDrawCommandsPass->GetNumVisibleMeshesPerTypeBuffer();
	params.m_DrawCommandBuffer = m_pCreateMainDrawCommandsPass->GetDrawCommandBuffer();

	m_pRenderGBufferMainPass = new RenderGBufferPass(&params);
}

CommandList* DXApplication::RecordRenderGBufferMainPass()
{
	assert(m_pRenderGBufferMainPass != nullptr);

	RenderGBufferPass::RenderParams params;
	params.m_pRenderEnv = m_pRenderEnv;
	params.m_pCommandList = m_pCommandListPool->Create(L"pRenderGBufferMainCommandList");
	params.m_pAppDataBuffer = m_pAppDataBuffers[m_BackBufferIndex];
	params.m_pMeshRenderResources = m_pMeshRenderResources;
	params.m_pNumVisibleMeshesPerTypeBuffer = m_pCreateMainDrawCommandsPass->GetNumVisibleMeshesPerTypeBuffer();
	params.m_pDrawCommandBuffer = m_pCreateMainDrawCommandsPass->GetDrawCommandBuffer();
	params.m_pViewport = m_pBackBufferViewport;
	params.m_ClearGBufferBeforeRendering = true;

	m_pRenderGBufferMainPass->Record(&params);
	return params.m_pCommandList;
}

void DXApplication::InitFillVisibilityBufferFalseNegativePass()
{
	assert(m_pCreateMainDrawCommandsPass != nullptr);
	assert(m_pFillVisibilityBufferFalseNegativePass == nullptr);

	const CreateMainDrawCommandsPass::ResourceStates* pCreateMainDrawCommandsPassStates =
		m_pCreateMainDrawCommandsPass->GetOutputResourceStates();

	FillVisibilityBufferPass::InitParams params;
	params.m_pRenderEnv = m_pRenderEnv;

	params.m_InputResourceStates.m_InstanceIndexBufferState = pCreateMainDrawCommandsPassStates->m_OccludedInstanceIndexBufferState;
	params.m_InputResourceStates.m_NumInstancesBufferState = pCreateMainDrawCommandsPassStates->m_NumOccludedInstancesBufferState;
	params.m_InputResourceStates.m_InstanceWorldOBBMatrixBufferState = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
	params.m_InputResourceStates.m_DepthTextureState = D3D12_RESOURCE_STATE_DEPTH_WRITE;
	params.m_InputResourceStates.m_VisibilityBufferState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;

	params.m_pInstanceIndexBuffer = m_pCreateMainDrawCommandsPass->GetOccludedInstanceIndexBuffer();
	params.m_pNumInstancesBuffer = m_pCreateMainDrawCommandsPass->GetNumOccludedInstancesBuffer();
	params.m_pInstanceWorldOBBMatrixBuffer = m_pMeshRenderResources->GetInstanceWorldOBBMatrixBuffer();
	params.m_pDepthTexture = m_pDepthTexture;
	params.m_ClampVerticesBehindCameraNearPlane = false;
	params.m_MaxNumInstances = m_pMeshRenderResources->GetTotalNumInstances();

	m_pFillVisibilityBufferFalseNegativePass = new FillVisibilityBufferPass(&params);
}

CommandList* DXApplication::RecordFillVisibilityBufferFalseNegativePass()
{
	FillVisibilityBufferPass::RenderParams params;
	params.m_pRenderEnv = m_pRenderEnv;
	params.m_pCommandList = m_pCommandListPool->Create(L"pFillVisibilityBufferFalseNegativeCommandList");
	params.m_pAppDataBuffer = m_pAppDataBuffers[m_BackBufferIndex];
	params.m_pNumInstancesBuffer = m_pCreateMainDrawCommandsPass->GetNumOccludedInstancesBuffer();

	m_pFillVisibilityBufferFalseNegativePass->Record(&params);
	return params.m_pCommandList;
}

void DXApplication::InitCreateFalseNegativeDrawCommandsPass()
{
	assert(m_pFrustumMeshCullingPass != nullptr);
	assert(m_pFillVisibilityBufferFalseNegativePass != nullptr);
	assert(m_pCreateMainDrawCommandsPass != nullptr);
	assert(m_pCreateFalseNegativeDrawCommandsPass == nullptr);

	const FrustumMeshCullingPass::ResourceStates* pFrustumMeshCullingPassStates =
		m_pFrustumMeshCullingPass->GetOutputResourceStates();

	const FillVisibilityBufferPass::ResourceStates* pFillVisibilityBufferFalseNegativePassStates =
		m_pFillVisibilityBufferFalseNegativePass->GetOutputResourceStates();

	const CreateMainDrawCommandsPass::ResourceStates* pCreateMainDrawCommandsPassStates =
		m_pCreateMainDrawCommandsPass->GetOutputResourceStates();

	CreateFalseNegativeDrawCommandsPass::InitParams params;
	params.m_pRenderEnv = m_pRenderEnv;
	
	params.m_InputResourceStates.m_NumMeshesBufferState = pCreateMainDrawCommandsPassStates->m_NumMeshesBufferState;
	params.m_InputResourceStates.m_MeshInfoBufferState = pCreateMainDrawCommandsPassStates->m_MeshInfoBufferState;
	params.m_InputResourceStates.m_InstanceIndexBufferState = pFillVisibilityBufferFalseNegativePassStates->m_InstanceIndexBufferState;
	params.m_InputResourceStates.m_VisibilityBufferState = pFillVisibilityBufferFalseNegativePassStates->m_VisibilityBufferState;
	params.m_InputResourceStates.m_VisibleInstanceIndexBufferState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
	params.m_InputResourceStates.m_NumVisibleMeshesPerTypeBufferState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
	params.m_InputResourceStates.m_DrawCommandBufferState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;

	params.m_pNumMeshesBuffer = m_pFrustumMeshCullingPass->GetNumVisibleMeshesBuffer();
	params.m_pMeshInfoBuffer = m_pFrustumMeshCullingPass->GetVisibleMeshInfoBuffer();
	params.m_pInstanceIndexBuffer = m_pCreateMainDrawCommandsPass->GetOccludedInstanceIndexBuffer();
	params.m_pVisibilityBuffer = m_pFillVisibilityBufferFalseNegativePass->GetVisibilityBuffer();
	params.m_NumMeshTypes = m_pMeshRenderResources->GetNumMeshTypes();
	params.m_MaxNumMeshes = m_pMeshRenderResources->GetTotalNumMeshes();
	params.m_MaxNumInstances = m_pMeshRenderResources->GetTotalNumInstances();
	params.m_MaxNumInstancesPerMesh = m_pMeshRenderResources->GetMaxNumInstancesPerMesh();

	m_pCreateFalseNegativeDrawCommandsPass = new CreateFalseNegativeDrawCommandsPass(&params);
}

CommandList* DXApplication::RecordCreateFalseNegativeDrawCommandsPass()
{
	assert(m_pCreateFalseNegativeDrawCommandsPass != nullptr);
	
	CreateFalseNegativeDrawCommandsPass::RenderParams params;
	params.m_pRenderEnv = m_pRenderEnv;
	params.m_pCommandList = m_pCommandListPool->Create(L"pCreateFalseNegativeDrawCommandsList");
	params.m_pNumMeshesBuffer = m_pFrustumMeshCullingPass->GetNumVisibleMeshesBuffer();

	m_pCreateFalseNegativeDrawCommandsPass->Record(&params);
	return params.m_pCommandList;
}

void DXApplication::InitRenderGBufferFalseNegativePass(UINT bufferWidth, UINT bufferHeight)
{
	assert(m_pRenderGBufferFalseNegativePass == nullptr);
	assert(m_pRenderGBufferMainPass != nullptr);
	assert(m_pFillVisibilityBufferFalseNegativePass != nullptr);
	assert(m_pCreateFalseNegativeDrawCommandsPass != nullptr);

	const RenderGBufferPass::ResourceStates* pRenderGBufferMainPassStates =
		m_pRenderGBufferMainPass->GetOutputResourceStates();

	const FillVisibilityBufferPass::ResourceStates* pFillVisibilityBufferFalseNegativePassStates =
		m_pFillVisibilityBufferFalseNegativePass->GetOutputResourceStates();

	const CreateFalseNegativeDrawCommandsPass::ResourceStates* pCreateFalseNegativeDrawCommandsPassStates =
		m_pCreateFalseNegativeDrawCommandsPass->GetOutputResourceStates();
	
	RenderGBufferPass::InitParams params;
	params.m_pRenderEnv = m_pRenderEnv;
	params.m_BufferWidth = bufferWidth;
	params.m_BufferHeight = bufferHeight;

	params.m_InputResourceStates.m_TexCoordTextureState = pRenderGBufferMainPassStates->m_TexCoordTextureState;
	params.m_InputResourceStates.m_NormalTextureState = pRenderGBufferMainPassStates->m_NormalTextureState;
	params.m_InputResourceStates.m_MaterialIDTextureState = pRenderGBufferMainPassStates->m_MaterialIDTextureState;
	params.m_InputResourceStates.m_DepthTextureState = pFillVisibilityBufferFalseNegativePassStates->m_DepthTextureState;
	params.m_InputResourceStates.m_InstanceWorldMatrixBufferState = pRenderGBufferMainPassStates->m_InstanceWorldMatrixBufferState;
	params.m_InputResourceStates.m_InstanceIndexBufferState = pCreateFalseNegativeDrawCommandsPassStates->m_VisibleInstanceIndexBufferState;
	params.m_InputResourceStates.m_NumVisibleMeshesPerTypeBufferState = pCreateFalseNegativeDrawCommandsPassStates->m_NumVisibleMeshesPerTypeBufferState;
	params.m_InputResourceStates.m_DrawCommandBufferState = pCreateFalseNegativeDrawCommandsPassStates->m_DrawCommandBufferState;

	params.m_pMeshRenderResources = m_pMeshRenderResources;
	params.m_pTexCoordTexture = m_pGeometryBuffer->GetTexCoordTexture();
	params.m_pNormalTexture = m_pGeometryBuffer->GetNormalTexture();
	params.m_pMaterialIDTexture = m_pGeometryBuffer->GetMaterialIDTexture();
	params.m_pDepthTexture = m_pDepthTexture;
	params.m_pInstanceWorldMatrixBuffer = m_pMeshRenderResources->GetInstanceWorldMatrixBuffer();
	params.m_pInstanceIndexBuffer = m_pCreateFalseNegativeDrawCommandsPass->GetVisibleInstanceIndexBuffer();
	params.m_NumVisibleMeshesPerTypeBuffer = m_pCreateFalseNegativeDrawCommandsPass->GetNumVisibleMeshesPerTypeBuffer();
	params.m_DrawCommandBuffer = m_pCreateFalseNegativeDrawCommandsPass->GetDrawCommandBuffer();
	
	m_pRenderGBufferFalseNegativePass = new RenderGBufferPass(&params);
}

CommandList* DXApplication::RecordRenderGBufferFalseNegativePass()
{
	assert(m_pRenderGBufferFalseNegativePass != nullptr);

	RenderGBufferPass::RenderParams params;
	params.m_pRenderEnv = m_pRenderEnv;
	params.m_pCommandList = m_pCommandListPool->Create(L"pRecordRenderGBufferFalseNegativeCommandList");
	params.m_pAppDataBuffer = m_pAppDataBuffers[m_BackBufferIndex];
	params.m_pMeshRenderResources = m_pMeshRenderResources;
	params.m_pNumVisibleMeshesPerTypeBuffer = m_pCreateFalseNegativeDrawCommandsPass->GetNumVisibleMeshesPerTypeBuffer();
	params.m_pDrawCommandBuffer = m_pCreateFalseNegativeDrawCommandsPass->GetDrawCommandBuffer();
	params.m_pViewport = m_pBackBufferViewport;
	params.m_ClearGBufferBeforeRendering = false;

	m_pRenderGBufferFalseNegativePass->Record(&params);
	return params.m_pCommandList;
}

void DXApplication::InitCalcShadingRectanglesPass()
{
	assert(m_pCalcShadingRectanglesPass == nullptr);
	assert(m_pRenderGBufferFalseNegativePass != nullptr);
	assert(m_pMaterialRenderResources != nullptr);
		
	const RenderGBufferPass::ResourceStates* pRenderGBufferFalseNegativePassStates =
		m_pRenderGBufferFalseNegativePass->GetOutputResourceStates();

	CalcShadingRectanglesPass::InitParams params;
	params.m_pRenderEnv = m_pRenderEnv;
	params.m_InputResourceStates.m_MaterialIDTextureState = pRenderGBufferFalseNegativePassStates->m_MaterialIDTextureState;
	params.m_InputResourceStates.m_MeshTypePerMaterialIDBufferState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	params.m_InputResourceStates.m_ShadingRectangleMinPointBufferState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
	params.m_InputResourceStates.m_ShadingRectangleMaxPointBufferState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
	params.m_pMaterialIDTexture = m_pGeometryBuffer->GetMaterialIDTexture();
	params.m_pMeshTypePerMaterialIDBuffer = m_pMaterialRenderResources->GetMeshTypePerMaterialIDBuffer();
	params.m_NumMeshTypes = m_pMeshRenderResources->GetNumMeshTypes();
	
	m_pCalcShadingRectanglesPass = new CalcShadingRectanglesPass(&params);
}

CommandList* DXApplication::RecordCalcShadingRectanglesPass()
{
	assert(m_pCalcShadingRectanglesPass != nullptr);

	CalcShadingRectanglesPass::RenderParams params;
	params.m_pRenderEnv = m_pRenderEnv;
	params.m_pCommandList = m_pCommandListPool->Create(L"pCalcShadingRectanglesCommandList");
	params.m_pAppDataBuffer = m_pAppDataBuffers[m_BackBufferIndex];

	m_pCalcShadingRectanglesPass->Record(&params);
	return params.m_pCommandList;
}

void DXApplication::InitFillMeshTypeDepthBufferPass()
{
	assert(m_pFillMeshTypeDepthBufferPass == nullptr);
	assert(m_pGeometryBuffer != nullptr);
	assert(m_pMaterialRenderResources != nullptr);
	
	const CalcShadingRectanglesPass::ResourceStates* pCalcShadingRectanglesPassStates =
		m_pCalcShadingRectanglesPass->GetOutputResourceStates();

	FillMeshTypeDepthBufferPass::InitParams params;
	params.m_pRenderEnv = m_pRenderEnv;
	params.m_InputResourceStates.m_MaterialIDTextureState = pCalcShadingRectanglesPassStates->m_MaterialIDTextureState;
	params.m_InputResourceStates.m_MeshTypePerMaterialIDBufferState = pCalcShadingRectanglesPassStates->m_MeshTypePerMaterialIDBufferState;
	params.m_InputResourceStates.m_MeshTypeDepthTextureState = D3D12_RESOURCE_STATE_DEPTH_WRITE;
	params.m_pMaterialIDTexture = m_pGeometryBuffer->GetMaterialIDTexture();
	params.m_pMeshTypePerMaterialIDBuffer = m_pMaterialRenderResources->GetMeshTypePerMaterialIDBuffer();

	m_pFillMeshTypeDepthBufferPass = new FillMeshTypeDepthBufferPass(&params);
}

CommandList* DXApplication::RecordFillMeshTypeDepthBufferPass()
{
	assert(m_pFillMeshTypeDepthBufferPass != nullptr);
	assert(m_pMeshRenderResources != nullptr);

	FillMeshTypeDepthBufferPass::RenderParams params;
	params.m_pRenderEnv = m_pRenderEnv;
	params.m_pCommandList = m_pCommandListPool->Create(L"pFillMeshTypeDepthBufferCommandList");
	params.m_NumMeshTypes = m_pMeshRenderResources->GetNumMeshTypes();
	params.m_pViewport = m_pBackBufferViewport;

	m_pFillMeshTypeDepthBufferPass->Record(&params);
	return params.m_pCommandList;
}

void DXApplication::InitVisualizeDepthBufferPass()
{
	assert(m_pTiledShadingPass != nullptr);
	const TiledShadingPass::ResourceStates* pTiledShadingPassStates =
		m_pTiledShadingPass->GetOutputResourceStates();

	for (u8 index = 0; index < kNumBackBuffers; ++index)
	{
		assert(m_VisualizeDepthBufferPasses[index] == nullptr);
		
		VisualizeTexturePass::InitParams params;
		params.m_pRenderEnv = m_pRenderEnv;
		params.m_InputResourceStates.m_InputTextureState = pTiledShadingPassStates->m_DepthTextureState;
		params.m_InputResourceStates.m_BackBufferState = D3D12_RESOURCE_STATE_PRESENT;
		params.m_pInputTexture = m_pDepthTexture;
		params.m_InputTextureSRV = m_pDepthTexture->GetSRVHandle();
		params.m_pBackBuffer = m_pSwapChain->GetBackBuffer(index);
		params.m_TextureType = VisualizeTexturePass::TextureType_Depth;

		m_VisualizeDepthBufferPasses[index] = new VisualizeTexturePass(&params);
	}
}

CommandList* DXApplication::RecordVisualizeDepthBufferPass()
{
	VisualizeTexturePass* pVisualizeTexturePass = m_VisualizeDepthBufferPasses[m_BackBufferIndex];
	assert(pVisualizeTexturePass != nullptr);

	VisualizeTexturePass::RenderParams params;
	params.m_pRenderEnv = m_pRenderEnv;
	params.m_pCommandList = m_pCommandListPool->Create(L"pVisualizeDepthBufferCommandList");
	params.m_pAppDataBuffer = m_pAppDataBuffers[m_BackBufferIndex];
	params.m_pViewport = m_pBackBufferViewport;

	pVisualizeTexturePass->Record(&params);
	return params.m_pCommandList;
}

void DXApplication::InitVisualizeReprojectedDepthBufferPass()
{
	assert(m_pDownscaleAndReprojectDepthPass != nullptr);
	DepthTexture* pProjectedDepthTexture = m_pDownscaleAndReprojectDepthPass->GetReprojectedDepthTexture();

	const DownscaleAndReprojectDepthPass::ResourceStates* pResourceStates =
		m_pDownscaleAndReprojectDepthPass->GetOutputResourceStates();

	for (u8 index = 0; index < kNumBackBuffers; ++index)
	{
		assert(m_VisualizeReprojectedDepthBufferPasses[index] == nullptr);

		VisualizeTexturePass::InitParams params;
		params.m_pRenderEnv = m_pRenderEnv;
		params.m_InputResourceStates.m_InputTextureState = pResourceStates->m_ReprojectedDepthTextureState;
		params.m_InputResourceStates.m_BackBufferState = D3D12_RESOURCE_STATE_PRESENT;
		params.m_pInputTexture = pProjectedDepthTexture;
		params.m_InputTextureSRV = pProjectedDepthTexture->GetSRVHandle();
		params.m_pBackBuffer = m_pSwapChain->GetBackBuffer(index);
		params.m_TextureType = VisualizeTexturePass::TextureType_Depth;

		m_VisualizeReprojectedDepthBufferPasses[index] = new VisualizeTexturePass(&params);
	}
}

CommandList* DXApplication::RecordVisualizeReprojectedDepthBufferPass()
{
	VisualizeTexturePass* pVisualizeTexturePass = m_VisualizeReprojectedDepthBufferPasses[m_BackBufferIndex];
	assert(pVisualizeTexturePass != nullptr);

	VisualizeTexturePass::RenderParams params;
	params.m_pRenderEnv = m_pRenderEnv;
	params.m_pCommandList = m_pCommandListPool->Create(L"pVisualizeReprojectedDepthBufferCommandList");
	params.m_pAppDataBuffer = m_pAppDataBuffers[m_BackBufferIndex];
	params.m_pViewport = m_pBackBufferViewport;

	pVisualizeTexturePass->Record(&params);
	return params.m_pCommandList;
}

void DXApplication::InitVisualizeNormalBufferPass()
{
	assert(m_pTiledShadingPass != nullptr);
	const TiledShadingPass::ResourceStates* pTiledShadingPassStates =
		m_pTiledShadingPass->GetOutputResourceStates();

	for (u8 index = 0; index < kNumBackBuffers; ++index)
	{
		assert(m_VisualizeNormalBufferPasses[index] == nullptr);

		VisualizeTexturePass::InitParams params;
		params.m_pRenderEnv = m_pRenderEnv;
		params.m_InputResourceStates.m_InputTextureState = pTiledShadingPassStates->m_NormalTextureState;
		params.m_InputResourceStates.m_BackBufferState = D3D12_RESOURCE_STATE_PRESENT;
		params.m_pInputTexture = m_pGeometryBuffer->GetNormalTexture();
		params.m_InputTextureSRV = m_pGeometryBuffer->GetNormalTexture()->GetSRVHandle();
		params.m_pBackBuffer = m_pSwapChain->GetBackBuffer(index);
		params.m_TextureType = VisualizeTexturePass::TextureType_GBufferNormal;

		m_VisualizeNormalBufferPasses[index] = new VisualizeTexturePass(&params);
	}
}

CommandList* DXApplication::RecordVisualizeNormalBufferPass()
{
	VisualizeTexturePass* pVisualizeTexturePass = m_VisualizeNormalBufferPasses[m_BackBufferIndex];
	assert(pVisualizeTexturePass != nullptr);

	VisualizeTexturePass::RenderParams params;
	params.m_pRenderEnv = m_pRenderEnv;
	params.m_pCommandList = m_pCommandListPool->Create(L"pVisualizeNormalBufferCommandList");
	params.m_pAppDataBuffer = m_pAppDataBuffers[m_BackBufferIndex];
	params.m_pViewport = m_pBackBufferViewport;

	pVisualizeTexturePass->Record(&params);
	return params.m_pCommandList;
}

void DXApplication::InitVisualizeTexCoordBufferPass()
{
	assert(m_pTiledShadingPass != nullptr);
	const TiledShadingPass::ResourceStates* pTiledShadingPassStates =
		m_pTiledShadingPass->GetOutputResourceStates();

	for (u8 index = 0; index < kNumBackBuffers; ++index)
	{
		assert(m_VisualizeTexCoordBufferPasses[index] == nullptr);

		VisualizeTexturePass::InitParams params;
		params.m_pRenderEnv = m_pRenderEnv;
		params.m_InputResourceStates.m_InputTextureState = pTiledShadingPassStates->m_TexCoordTextureState;
		params.m_InputResourceStates.m_BackBufferState = D3D12_RESOURCE_STATE_PRESENT;
		params.m_pInputTexture = m_pGeometryBuffer->GetTexCoordTexture();
		params.m_InputTextureSRV = m_pGeometryBuffer->GetTexCoordTexture()->GetSRVHandle();
		params.m_pBackBuffer = m_pSwapChain->GetBackBuffer(index);
		params.m_TextureType = VisualizeTexturePass::TextureType_GBufferTexCoord;

		m_VisualizeTexCoordBufferPasses[index] = new VisualizeTexturePass(&params);
	}
}

CommandList* DXApplication::RecordVisualizeTexCoordBufferPass()
{
	VisualizeTexturePass* pVisualizeTexturePass = m_VisualizeTexCoordBufferPasses[m_BackBufferIndex];
	assert(pVisualizeTexturePass != nullptr);

	VisualizeTexturePass::RenderParams params;
	params.m_pRenderEnv = m_pRenderEnv;
	params.m_pCommandList = m_pCommandListPool->Create(L"pVisualizeTexCoordBufferCommandList");
	params.m_pAppDataBuffer = m_pAppDataBuffers[m_BackBufferIndex];
	params.m_pViewport = m_pBackBufferViewport;

	pVisualizeTexturePass->Record(&params);
	return params.m_pCommandList;
}

void DXApplication::InitVisualizeDepthBufferWithMeshTypePass()
{
	assert(m_pFillMeshTypeDepthBufferPass != nullptr);
	DepthTexture* pDepthTextureWithMeshType = m_pFillMeshTypeDepthBufferPass->GetMeshTypeDepthTexture();

	const FillMeshTypeDepthBufferPass::ResourceStates* pFillMeshTypeDepthBufferPassStates =
		m_pFillMeshTypeDepthBufferPass->GetOutputResourceStates();

	for (u8 index = 0; index < kNumBackBuffers; ++index)
	{
		assert(m_VisualizeDepthBufferWithMeshTypePasses[index] == nullptr);

		VisualizeTexturePass::InitParams params;
		params.m_pRenderEnv = m_pRenderEnv;
		params.m_InputResourceStates.m_InputTextureState = pFillMeshTypeDepthBufferPassStates->m_MeshTypeDepthTextureState;
		params.m_InputResourceStates.m_BackBufferState = D3D12_RESOURCE_STATE_PRESENT;
		params.m_pInputTexture = pDepthTextureWithMeshType;
		params.m_InputTextureSRV = pDepthTextureWithMeshType->GetSRVHandle();
		params.m_pBackBuffer = m_pSwapChain->GetBackBuffer(index);
		params.m_TextureType = VisualizeTexturePass::TextureType_Depth;

		m_VisualizeDepthBufferWithMeshTypePasses[index] = new VisualizeTexturePass(&params);
	}
}

CommandList* DXApplication::RecordVisualizeDepthBufferWithMeshTypePass()
{
	VisualizeTexturePass* pVisualizeTexturePass = m_VisualizeDepthBufferWithMeshTypePasses[m_BackBufferIndex];
	assert(pVisualizeTexturePass != nullptr);

	VisualizeTexturePass::RenderParams params;
	params.m_pRenderEnv = m_pRenderEnv;
	params.m_pCommandList = m_pCommandListPool->Create(L"pVisualizeDepthBufferWithMeshTypeCommandList");
	params.m_pAppDataBuffer = m_pAppDataBuffers[m_BackBufferIndex];
	params.m_pViewport = m_pBackBufferViewport;

	pVisualizeTexturePass->Record(&params);
	return params.m_pCommandList;
}

void DXApplication::InitVisualizeAccumLightPass()
{
	assert(m_pTiledShadingPass != nullptr);

	const TiledShadingPass::ResourceStates* pTiledShadingPassStates =
		m_pTiledShadingPass->GetOutputResourceStates();

	for (u8 index = 0; index < kNumBackBuffers; ++index)
	{
		assert(m_pVisualizeAccumLightPasses[index] == nullptr);

		VisualizeTexturePass::InitParams params;
		params.m_pRenderEnv = m_pRenderEnv;
		params.m_InputResourceStates.m_InputTextureState = pTiledShadingPassStates->m_AccumLightTextureState;
		params.m_InputResourceStates.m_BackBufferState = D3D12_RESOURCE_STATE_PRESENT;
		params.m_pInputTexture = m_pAccumLightTexture;
		params.m_InputTextureSRV = m_pAccumLightTexture->GetSRVHandle();
		params.m_pBackBuffer = m_pSwapChain->GetBackBuffer(index);
		params.m_TextureType = VisualizeTexturePass::TextureType_Other;

		m_pVisualizeAccumLightPasses[index] = new VisualizeTexturePass(&params);
	}
}

CommandList* DXApplication::RecordVisualizeAccumLightPass()
{
	VisualizeTexturePass* pVisualizeTexturePass = m_pVisualizeAccumLightPasses[m_BackBufferIndex];
	assert(pVisualizeTexturePass != nullptr);

	VisualizeTexturePass::RenderParams params;
	params.m_pRenderEnv = m_pRenderEnv;
	params.m_pCommandList = m_pCommandListPool->Create(L"pVisualizeAccumLightCommandList");
	params.m_pAppDataBuffer = m_pAppDataBuffers[m_BackBufferIndex];
	params.m_pViewport = m_pBackBufferViewport;

	pVisualizeTexturePass->Record(&params);
	return params.m_pCommandList;
}

void DXApplication::InitVisualizeVoxelReflectancePass()
{
	assert(m_pVoxelizePass != nullptr);
	assert(m_pTiledShadingPass != nullptr);
	
	const VoxelizePass::ResourceStates* pVoxelizePassStates =
		m_pVoxelizePass->GetOutputResourceStates();

	const TiledShadingPass::ResourceStates* pTiledShadingPassStates =
		m_pTiledShadingPass->GetOutputResourceStates();

	for (u8 index = 0; index < kNumBackBuffers; ++index)
	{
		assert(m_VisualizeVoxelReflectancePasses[index] == nullptr);

		VisualizeVoxelReflectancePass::InitParams params;
		params.m_pRenderEnv = m_pRenderEnv;
		params.m_InputResourceStates.m_BackBufferState = D3D12_RESOURCE_STATE_PRESENT;
		params.m_InputResourceStates.m_DepthTextureState = pTiledShadingPassStates->m_DepthTextureState;
		params.m_InputResourceStates.m_VoxelReflectanceTextureState = pVoxelizePassStates->m_VoxelReflectanceTextureState;
		params.m_pBackBuffer = m_pSwapChain->GetBackBuffer(index);
		params.m_pDepthTexture = m_pDepthTexture;
		params.m_pVoxelReflectanceTexture = m_pVoxelizePass->GetVoxelReflectanceTexture();

		m_VisualizeVoxelReflectancePasses[index] = new VisualizeVoxelReflectancePass(&params);
	}
}

CommandList* DXApplication::RecordVisualizeVoxelReflectancePass()
{
	VisualizeVoxelReflectancePass* pVisualizeVoxelReflectancePass = m_VisualizeVoxelReflectancePasses[m_BackBufferIndex];
	assert(pVisualizeVoxelReflectancePass != nullptr);

	VisualizeVoxelReflectancePass::RenderParams params;
	params.m_pRenderEnv = m_pRenderEnv;
	params.m_pCommandList = m_pCommandListPool->Create(L"pVisualizeVoxelReflectanceCommandList");
	params.m_pAppDataBuffer = m_pAppDataBuffers[m_BackBufferIndex];
	params.m_pViewport = m_pBackBufferViewport;

	pVisualizeVoxelReflectancePass->Record(&params);
	return params.m_pCommandList;
}

void DXApplication::InitFrustumPointLightCullingPass()
{
	assert(m_pFrustumPointLightCullingPass == nullptr);
	assert(m_pPointLightRenderResources != nullptr);
	
	FrustumLightCullingPass::InitParams params;
	params.m_pRenderEnv = m_pRenderEnv;
	params.m_InputResourceStates.m_LightWorldBoundsBufferState = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
	params.m_InputResourceStates.m_NumVisibleLightsBufferState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
	params.m_InputResourceStates.m_VisibleLightIndexBufferState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
	params.m_pLightWorldBoundsBuffer = m_pPointLightRenderResources->GetLightWorldBoundsBuffer();
	params.m_NumTotalLights = m_pPointLightRenderResources->GetNumLights();

	m_pFrustumPointLightCullingPass = new FrustumLightCullingPass(&params);
}

CommandList* DXApplication::RecordFrustumPointLightCullingPass()
{
	assert(m_pFrustumPointLightCullingPass != nullptr);

	FrustumLightCullingPass::RenderParams params;
	params.m_pRenderEnv = m_pRenderEnv;
	params.m_pCommandList = m_pCommandListPool->Create(L"pFrustumPointLightCullingCommandList");
	params.m_pAppDataBuffer = m_pAppDataBuffers[m_BackBufferIndex];

	m_pFrustumPointLightCullingPass->Record(&params);
	return params.m_pCommandList;
}

void DXApplication::InitFrustumSpotLightCullingPass()
{
	assert(m_pFrustumSpotLightCullingPass == nullptr);
	assert(m_pSpotLightRenderResources != nullptr);

	FrustumLightCullingPass::InitParams params;
	params.m_pRenderEnv = m_pRenderEnv;
	params.m_InputResourceStates.m_LightWorldBoundsBufferState = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
	params.m_InputResourceStates.m_NumVisibleLightsBufferState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
	params.m_InputResourceStates.m_VisibleLightIndexBufferState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
	params.m_pLightWorldBoundsBuffer = m_pSpotLightRenderResources->GetLightWorldBoundsBuffer();
	params.m_NumTotalLights = m_pSpotLightRenderResources->GetNumLights();

	m_pFrustumSpotLightCullingPass = new FrustumLightCullingPass(&params);
}

CommandList* DXApplication::RecordFrustumSpotLightCullingPass()
{
	assert(m_pFrustumSpotLightCullingPass != nullptr);

	FrustumLightCullingPass::RenderParams params;
	params.m_pRenderEnv = m_pRenderEnv;
	params.m_pCommandList = m_pCommandListPool->Create(L"pFrustumSpotLightCullingCommandList");
	params.m_pAppDataBuffer = m_pAppDataBuffers[m_BackBufferIndex];

	m_pFrustumSpotLightCullingPass->Record(&params);
	return params.m_pCommandList;
}

void DXApplication::InitTiledLightCullingPass()
{
	assert(m_pTiledLightCullingPass == nullptr);
	assert(m_pRenderGBufferFalseNegativePass != nullptr);

	const RenderGBufferPass::ResourceStates* pRenderGBufferFalseNegativePassStates =
		m_pRenderGBufferFalseNegativePass->GetOutputResourceStates();

	TiledLightCullingPass::InitParams params;
	params.m_pRenderEnv = m_pRenderEnv;
	params.m_pDepthTexture = m_pDepthTexture;
	params.m_InputResourceStates.m_DepthTextureState = pRenderGBufferFalseNegativePassStates->m_DepthTextureState;
				
	if (m_pFrustumPointLightCullingPass != nullptr)
	{
		const FrustumLightCullingPass::ResourceStates* pFrustumPointLightCullingPassStates =
			m_pFrustumPointLightCullingPass->GetOutputResourceStates();

		params.m_InputResourceStates.m_NumPointLightsBufferState = pFrustumPointLightCullingPassStates->m_NumVisibleLightsBufferState;
		params.m_InputResourceStates.m_PointLightIndexBufferState = pFrustumPointLightCullingPassStates->m_VisibleLightIndexBufferState;
		params.m_InputResourceStates.m_PointLightWorldBoundsBufferState = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
		params.m_InputResourceStates.m_PointLightIndexPerTileBufferState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
		params.m_InputResourceStates.m_PointLightRangePerTileBufferState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;

		params.m_MaxNumPointLights = m_pPointLightRenderResources->GetNumLights();
		params.m_pPointLightWorldBoundsBuffer = m_pPointLightRenderResources->GetLightWorldBoundsBuffer();
		params.m_pNumPointLightsBuffer = m_pFrustumPointLightCullingPass->GetNumVisibleLightsBuffer();
		params.m_pPointLightIndexBuffer = m_pFrustumPointLightCullingPass->GetVisibleLightIndexBuffer();
	}
	else
	{
		params.m_MaxNumPointLights = 0;
		params.m_pNumPointLightsBuffer = nullptr;
		params.m_pPointLightIndexBuffer = nullptr;
		params.m_pPointLightWorldBoundsBuffer = nullptr;
	}

	if (m_pFrustumSpotLightCullingPass != nullptr)
	{
		const FrustumLightCullingPass::ResourceStates* pFrustumSpotLightCullingPassStates =
			m_pFrustumSpotLightCullingPass->GetOutputResourceStates();

		params.m_InputResourceStates.m_NumSpotLightsBufferState = pFrustumSpotLightCullingPassStates->m_NumVisibleLightsBufferState;
		params.m_InputResourceStates.m_SpotLightIndexBufferState = pFrustumSpotLightCullingPassStates->m_VisibleLightIndexBufferState;
		params.m_InputResourceStates.m_SpotLightWorldBoundsBufferState = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
		params.m_InputResourceStates.m_SpotLightIndexPerTileBufferState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
		params.m_InputResourceStates.m_SpotLightRangePerTileBufferState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;

		params.m_MaxNumSpotLights = m_pSpotLightRenderResources->GetNumLights();
		params.m_pSpotLightWorldBoundsBuffer = m_pSpotLightRenderResources->GetLightWorldBoundsBuffer();
		params.m_pNumSpotLightsBuffer = m_pFrustumSpotLightCullingPass->GetNumVisibleLightsBuffer();
		params.m_pSpotLightIndexBuffer = m_pFrustumSpotLightCullingPass->GetVisibleLightIndexBuffer();
	}
	else
	{
		params.m_MaxNumSpotLights = 0;
		params.m_pNumSpotLightsBuffer = nullptr;
		params.m_pSpotLightIndexBuffer = nullptr;
		params.m_pSpotLightWorldBoundsBuffer = nullptr;
	}
	
	params.m_TileSize = kTileSize;
	params.m_NumTilesX = kNumTilesX;
	params.m_NumTilesY = kNumTilesY;

	m_pTiledLightCullingPass = new TiledLightCullingPass(&params);
}

CommandList* DXApplication::RecordTiledLightCullingPass()
{
	assert(m_pTiledLightCullingPass != nullptr);

	TiledLightCullingPass::RenderParams params;
	params.m_pRenderEnv = m_pRenderEnv;
	params.m_pCommandList = m_pCommandListPool->Create(L"pTiledLightCullingCommandList");
	params.m_pAppDataBuffer = m_pAppDataBuffers[m_BackBufferIndex];
		
	m_pTiledLightCullingPass->Record(&params);
	return params.m_pCommandList;
}

void DXApplication::InitCreateVoxelizeCommandsPass()
{	
	assert(m_pCreateVoxelizeCommandsPass == nullptr);
	assert(m_pFrustumMeshCullingPass != nullptr);
	assert(m_pCreateFalseNegativeDrawCommandsPass != nullptr);
	assert(m_pMeshRenderResources != nullptr);

	const CreateFalseNegativeDrawCommandsPass::ResourceStates* pResourceStates =
		m_pCreateFalseNegativeDrawCommandsPass->GetOutputResourceStates();

	CreateVoxelizeCommandsPass::InitParams params;
	params.m_pRenderEnv = m_pRenderEnv;
	
	params.m_InputResourceStates.m_NumMeshesBufferState = pResourceStates->m_NumMeshesBufferState;
	params.m_InputResourceStates.m_MeshInfoBufferState = pResourceStates->m_MeshInfoBufferState;
	params.m_InputResourceStates.m_NumCommandsPerMeshTypeBufferState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
	params.m_InputResourceStates.m_VoxelizeCommandBufferState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;

	params.m_pNumMeshesBuffer = m_pFrustumMeshCullingPass->GetNumVisibleMeshesBuffer();
	params.m_pMeshInfoBuffer = m_pFrustumMeshCullingPass->GetVisibleMeshInfoBuffer();
	params.m_NumMeshTypes = m_pMeshRenderResources->GetNumMeshTypes();
	params.m_MaxNumMeshes = m_pMeshRenderResources->GetTotalNumMeshes();
		
	m_pCreateVoxelizeCommandsPass = new CreateVoxelizeCommandsPass(&params);
}

CommandList* DXApplication::RecordCreateVoxelizeCommandsPass()
{
	assert(m_pCreateVoxelizeCommandsPass != nullptr);

	CreateVoxelizeCommandsPass::RenderParams params;
	params.m_pRenderEnv = m_pRenderEnv = m_pRenderEnv;
	params.m_pCommandList = m_pCommandListPool->Create(L"pCreateVoxelizeCommandsCommandList");

	m_pCreateVoxelizeCommandsPass->Record(&params);
	return params.m_pCommandList;
}

void DXApplication::InitVoxelizePass()
{
	assert(m_pVoxelizePass == nullptr);
	assert(m_pMeshRenderResources != nullptr);
	assert(m_pMaterialRenderResources != nullptr);
	assert(m_pCreateVoxelizeCommandsPass != nullptr);
	assert(m_pFrustumMeshCullingPass != nullptr);
	assert(m_pCreateMainDrawCommandsPass != nullptr);
	assert(m_pRenderGBufferFalseNegativePass != nullptr);
	assert(m_pTiledLightCullingPass != nullptr);
	
	const CreateMainDrawCommandsPass::ResourceStates* pCreateMainDrawCommandsPassStates =
		m_pCreateMainDrawCommandsPass->GetOutputResourceStates();

	const CreateVoxelizeCommandsPass::ResourceStates* pCreateVoxelizeCommandsPassStates =
		m_pCreateVoxelizeCommandsPass->GetOutputResourceStates();

	const RenderGBufferPass::ResourceStates* pRenderGBufferFalseNegativePassStates =
		m_pRenderGBufferFalseNegativePass->GetOutputResourceStates();

	const TiledLightCullingPass::ResourceStates* pTiledLightCullingPassStates =
		m_pTiledLightCullingPass->GetOutputResourceStates();
		
	VoxelizePass::InitParams params;
	params.m_pRenderEnv = m_pRenderEnv;
	
	params.m_InputResourceStates.m_NumCommandsPerMeshTypeBufferState = pCreateVoxelizeCommandsPassStates->m_NumCommandsPerMeshTypeBufferState;
	params.m_InputResourceStates.m_VoxelizeCommandBufferState = pCreateVoxelizeCommandsPassStates->m_VoxelizeCommandBufferState;
	params.m_InputResourceStates.m_InstanceIndexBufferState = pCreateMainDrawCommandsPassStates->m_InstanceIndexBufferState;
	params.m_InputResourceStates.m_InstanceWorldMatrixBufferState = pRenderGBufferFalseNegativePassStates->m_InstanceWorldMatrixBufferState;
	params.m_InputResourceStates.m_VoxelReflectanceTextureState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
	params.m_InputResourceStates.m_FirstResourceIndexPerMaterialIDBufferState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
		
	params.m_pMeshRenderResources = m_pMeshRenderResources;
	params.m_NumVoxelsX = kNumVoxelsInGridX;
	params.m_NumVoxelsY = kNumVoxelsInGridY;
	params.m_NumVoxelsZ = kNumVoxelsInGridZ;
	
	params.m_NumMaterialTextures = m_pMaterialRenderResources->GetNumTextures();
	params.m_ppMaterialTextures = m_pMaterialRenderResources->GetTextures();
	params.m_pFirstResourceIndexPerMaterialIDBuffer = m_pMaterialRenderResources->GetFirstResourceIndexPerMaterialIDBuffer();
	params.m_pNumCommandsPerMeshTypeBuffer = m_pCreateVoxelizeCommandsPass->GetNumCommandsPerMeshTypeBuffer();
	params.m_pVoxelizeCommandBuffer = m_pCreateVoxelizeCommandsPass->GetVoxelizeCommandBuffer();
	params.m_pInstanceIndexBuffer = m_pFrustumMeshCullingPass->GetVisibleInstanceIndexBuffer();
	params.m_pInstanceWorldMatrixBuffer = m_pMeshRenderResources->GetInstanceWorldMatrixBuffer();
	
	params.m_EnableDirectionalLight = false;
	if (m_pPointLightRenderResources != nullptr)
	{
		assert(m_pFrustumPointLightCullingPass != nullptr);
		params.m_EnablePointLights = true;

		params.m_InputResourceStates.m_PointLightWorldBoundsBufferState = pTiledLightCullingPassStates->m_PointLightWorldBoundsBufferState;
		params.m_InputResourceStates.m_PointLightPropsBufferState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
		params.m_InputResourceStates.m_NumPointLightsBufferState = pTiledLightCullingPassStates->m_NumPointLightsBufferState;
		params.m_InputResourceStates.m_PointLightIndexBufferState = pTiledLightCullingPassStates->m_PointLightIndexBufferState;

		params.m_pPointLightWorldBoundsBuffer = m_pPointLightRenderResources->GetLightWorldBoundsBuffer();
		params.m_pPointLightPropsBuffer = m_pPointLightRenderResources->GetLightPropsBuffer();
		params.m_pNumPointLightsBuffer = m_pFrustumPointLightCullingPass->GetNumVisibleLightsBuffer();
		params.m_pPointLightIndexBuffer = m_pFrustumPointLightCullingPass->GetVisibleLightIndexBuffer();
	}
	else
	{
		params.m_EnablePointLights = false;
		params.m_pPointLightWorldBoundsBuffer = nullptr;
		params.m_pPointLightPropsBuffer = nullptr;
		params.m_pNumPointLightsBuffer = nullptr;
		params.m_pPointLightIndexBuffer = nullptr;
	}	
	if (m_pSpotLightRenderResources != nullptr)
	{
		assert(m_pFrustumSpotLightCullingPass != nullptr);
		params.m_EnableSpotLights = true;

		params.m_InputResourceStates.m_SpotLightWorldBoundsBufferState = pTiledLightCullingPassStates->m_SpotLightWorldBoundsBufferState;
		params.m_InputResourceStates.m_SpotLightPropsBufferState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
		params.m_InputResourceStates.m_NumSpotLightsBufferState = pTiledLightCullingPassStates->m_NumSpotLightsBufferState;
		params.m_InputResourceStates.m_SpotLightIndexBufferState = pTiledLightCullingPassStates->m_SpotLightIndexBufferState;

		params.m_pSpotLightWorldBoundsBuffer = m_pSpotLightRenderResources->GetLightWorldBoundsBuffer();
		params.m_pSpotLightPropsBuffer = m_pSpotLightRenderResources->GetLightPropsBuffer();
		params.m_pNumSpotLightsBuffer = m_pFrustumSpotLightCullingPass->GetNumVisibleLightsBuffer();
		params.m_pSpotLightIndexBuffer = m_pFrustumSpotLightCullingPass->GetVisibleLightIndexBuffer();
	}
	else
	{
		params.m_EnableSpotLights = false;
		params.m_pSpotLightWorldBoundsBuffer = nullptr;
		params.m_pSpotLightPropsBuffer = nullptr;
		params.m_pNumSpotLightsBuffer = nullptr;
		params.m_pSpotLightIndexBuffer = nullptr;
	}

	m_pVoxelizePass = new VoxelizePass(&params);
}

CommandList* DXApplication::RecordVoxelizePass()
{
	assert(m_pVoxelizePass != nullptr);
	
	VoxelizePass::RenderParams params;
	params.m_pRenderEnv = m_pRenderEnv;
	params.m_pCommandList = m_pCommandListPool->Create(L"pVoxelizeCommandList");
	params.m_pMeshRenderResources = m_pMeshRenderResources;
	params.m_pNumCommandsPerMeshTypeBuffer = m_pCreateVoxelizeCommandsPass->GetNumCommandsPerMeshTypeBuffer();
	params.m_pVoxelizeCommandBuffer = m_pCreateVoxelizeCommandsPass->GetVoxelizeCommandBuffer();
	params.m_pAppDataBuffer = m_pAppDataBuffers[m_BackBufferIndex];;

	m_pVoxelizePass->Record(&params);
	return params.m_pCommandList;
}

void DXApplication::InitTiledShadingPass()
{
	assert(m_pTiledShadingPass == nullptr);
	assert(m_pFillMeshTypeDepthBufferPass != nullptr);
	assert(m_pCalcShadingRectanglesPass != nullptr);
	assert(m_pTiledLightCullingPass != nullptr);
	assert(m_pRenderGBufferFalseNegativePass != nullptr);
	assert(m_pVoxelizePass != nullptr);
	assert(m_pGeometryBuffer != nullptr);
	assert(m_pMaterialRenderResources != nullptr);
	
	const FillMeshTypeDepthBufferPass::ResourceStates* pFillMeshTypeDepthBufferPassStates =
		m_pFillMeshTypeDepthBufferPass->GetOutputResourceStates();

	const CalcShadingRectanglesPass::ResourceStates* pCalcShadingRectanglesPassStates =
		m_pCalcShadingRectanglesPass->GetOutputResourceStates();

	const TiledLightCullingPass::ResourceStates* pTiledLightCullingPassStates =
		m_pTiledLightCullingPass->GetOutputResourceStates();

	const RenderGBufferPass::ResourceStates* pRenderGBufferFalseNegativePassStates =
		m_pRenderGBufferFalseNegativePass->GetOutputResourceStates();

	const VoxelizePass::ResourceStates* pVoxelizePassStates =
		m_pVoxelizePass->GetOutputResourceStates();

	TiledShadingPass::InitParams params;
	params.m_pRenderEnv = m_pRenderEnv;
	params.m_ShadingMode = ShadingMode_BlinnPhong;
	
	params.m_pAccumLightTexture = m_pAccumLightTexture;
	params.m_pMeshTypeDepthTexture = m_pFillMeshTypeDepthBufferPass->GetMeshTypeDepthTexture();
	params.m_pShadingRectangleMinPointBuffer = m_pCalcShadingRectanglesPass->GetShadingRectangleMinPointBuffer();
	params.m_pShadingRectangleMaxPointBuffer = m_pCalcShadingRectanglesPass->GetShadingRectangleMaxPointBuffer();
	params.m_pDepthTexture = m_pDepthTexture;
	params.m_pTexCoordTexture = m_pGeometryBuffer->GetTexCoordTexture();
	params.m_pNormalTexture = m_pGeometryBuffer->GetNormalTexture();
	params.m_pMaterialIDTexture = m_pGeometryBuffer->GetMaterialIDTexture();
	params.m_pFirstResourceIndexPerMaterialIDBuffer = m_pMaterialRenderResources->GetFirstResourceIndexPerMaterialIDBuffer();
	params.m_NumMaterialTextures = m_pMaterialRenderResources->GetNumTextures();
	params.m_ppMaterialTextures = m_pMaterialRenderResources->GetTextures();
	params.m_EnableDirectionalLight = false;
	
	params.m_InputResourceStates.m_AccumLightTextureState = D3D12_RESOURCE_STATE_RENDER_TARGET;
	params.m_InputResourceStates.m_MeshTypeDepthTextureState = pFillMeshTypeDepthBufferPassStates->m_MeshTypeDepthTextureState;
	params.m_InputResourceStates.m_ShadingRectangleMinPointBufferState = pCalcShadingRectanglesPassStates->m_ShadingRectangleMinPointBufferState;
	params.m_InputResourceStates.m_ShadingRectangleMaxPointBufferState = pCalcShadingRectanglesPassStates->m_ShadingRectangleMaxPointBufferState;
	params.m_InputResourceStates.m_DepthTextureState = pTiledLightCullingPassStates->m_DepthTextureState;
	params.m_InputResourceStates.m_TexCoordTextureState = pRenderGBufferFalseNegativePassStates->m_TexCoordTextureState;
	params.m_InputResourceStates.m_NormalTextureState = pRenderGBufferFalseNegativePassStates->m_NormalTextureState;
	params.m_InputResourceStates.m_MaterialIDTextureState = pFillMeshTypeDepthBufferPassStates->m_MaterialIDTextureState;
	params.m_InputResourceStates.m_FirstResourceIndexPerMaterialIDBufferState = pVoxelizePassStates->m_FirstResourceIndexPerMaterialIDBufferState;
	
	if (m_pPointLightRenderResources != nullptr)
	{
		params.m_EnablePointLights = true;
		params.m_pPointLightWorldBoundsBuffer = m_pPointLightRenderResources->GetLightWorldBoundsBuffer();;
		params.m_pPointLightPropsBuffer = m_pPointLightRenderResources->GetLightPropsBuffer();
		params.m_pPointLightIndexPerTileBuffer = m_pTiledLightCullingPass->GetPointLightIndexPerTileBuffer();
		params.m_pPointLightRangePerTileBuffer = m_pTiledLightCullingPass->GetPointLightRangePerTileBuffer();

		params.m_InputResourceStates.m_PointLightWorldBoundsBufferState = pVoxelizePassStates->m_PointLightWorldBoundsBufferState;
		params.m_InputResourceStates.m_PointLightPropsBufferState = pVoxelizePassStates->m_PointLightPropsBufferState;
		params.m_InputResourceStates.m_PointLightIndexPerTileBufferState = pTiledLightCullingPassStates->m_PointLightIndexPerTileBufferState;
		params.m_InputResourceStates.m_PointLightRangePerTileBufferState = pTiledLightCullingPassStates->m_PointLightRangePerTileBufferState;
	}
	else
	{
		params.m_EnablePointLights = false;
		params.m_pPointLightWorldBoundsBuffer = nullptr;
		params.m_pPointLightPropsBuffer = nullptr;
		params.m_pPointLightIndexPerTileBuffer = nullptr;
		params.m_pPointLightRangePerTileBuffer = nullptr;
	}
	
	if (m_pSpotLightRenderResources != nullptr)
	{
		params.m_EnableSpotLights = true;
		params.m_pSpotLightWorldBoundsBuffer = m_pSpotLightRenderResources->GetLightWorldBoundsBuffer();
		params.m_pSpotLightPropsBuffer = m_pSpotLightRenderResources->GetLightPropsBuffer();
		params.m_pSpotLightIndexPerTileBuffer = m_pTiledLightCullingPass->GetSpotLightIndexPerTileBuffer();
		params.m_pSpotLightRangePerTileBuffer = m_pTiledLightCullingPass->GetSpotLightRangePerTileBuffer();

		params.m_InputResourceStates.m_SpotLightWorldBoundsBufferState = pVoxelizePassStates->m_SpotLightWorldBoundsBufferState;
		params.m_InputResourceStates.m_SpotLightPropsBufferState = pVoxelizePassStates->m_SpotLightPropsBufferState;
		params.m_InputResourceStates.m_SpotLightIndexPerTileBufferState = pTiledLightCullingPassStates->m_SpotLightIndexPerTileBufferState;
		params.m_InputResourceStates.m_SpotLightRangePerTileBufferState = pTiledLightCullingPassStates->m_SpotLightRangePerTileBufferState;
	}
	else
	{
		params.m_EnableSpotLights = false;
		params.m_pSpotLightWorldBoundsBuffer = nullptr;
		params.m_pSpotLightPropsBuffer = nullptr;
		params.m_pSpotLightIndexPerTileBuffer = nullptr;
		params.m_pSpotLightRangePerTileBuffer = nullptr;
	}

	m_pTiledShadingPass = new TiledShadingPass(&params);
}

CommandList* DXApplication::RecordTiledShadingPass()
{
	assert(m_pTiledShadingPass != nullptr);

	TiledShadingPass::RenderParams params;
	params.m_pRenderEnv = m_pRenderEnv;
	params.m_pCommandList = m_pCommandListPool->Create(L"pTiledShadingCommandList");
	params.m_pAppDataBuffer = m_pAppDataBuffers[m_BackBufferIndex];
	params.m_pViewport = m_pBackBufferViewport;

	m_pTiledShadingPass->Record(&params);
	return params.m_pCommandList;
}

void DXApplication::InitSetupSpotLightTiledShadowMapPass()
{
	assert(false);
	/*
	assert(m_pSpotLightRenderResources != nullptr);

	ConstantBufferDesc shadowMapDataBufferDesc(sizeof(ShadowMapData));
	m_pSpotLightShadowMapDataBuffer = new Buffer(m_pRenderEnv, m_pRenderEnv->m_pUploadHeapProps, &shadowMapDataBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, L"m_pSpotLightShadowMapDataBuffer");

	StructuredBufferDesc shadowMapTileBufferDesc(m_pSpotLightRenderResources->GetNumLights(), sizeof(ShadowMapTile), true, true);
	m_pSpotLightShadowMapTileBuffer = new Buffer(m_pRenderEnv, m_pRenderEnv->m_pDefaultHeapProps, &shadowMapTileBufferDesc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, L"m_pSpotLightShadowMapTileBuffer");

	StructuredBufferDesc lightViewProjTileMatrixBufferDesc(m_pSpotLightRenderResources->GetNumLights(), sizeof(Matrix4f), true, true);
	m_pSpotLightViewTileProjMatrixBuffer = new Buffer(m_pRenderEnv, m_pRenderEnv->m_pDefaultHeapProps, &lightViewProjTileMatrixBufferDesc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, L"m_pSpotLightShadowMapTileBuffer");

	Buffer* pLightViewProjMatrixBuffer = m_pSpotLightRenderResources->GetLightViewProjMatrixBuffer();

	SetupTiledShadowMapPass::InitParams initParams;
	initParams.m_pRenderEnv = m_pRenderEnv;
	initParams.m_LightType = LightType_Spot;
	initParams.m_MaxNumLights = m_pSpotLightRenderResources->GetNumLights();

	m_pSetupSpotLightTiledShadowMapPass = new SetupTiledShadowMapPass(&initParams);

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
	*/
}

void DXApplication::InitSetupPointLightTiledShadowMapPass()
{
	assert(false);
	/*
	assert(m_pPointLightRenderResources != nullptr);

	ConstantBufferDesc shadowMapDataBufferDesc(sizeof(ShadowMapData));
	m_pPointLightShadowMapDataBuffer = new Buffer(m_pRenderEnv, m_pRenderEnv->m_pUploadHeapProps, &shadowMapDataBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, L"m_pPointLightShadowMapDataBuffer");

	StructuredBufferDesc shadowMapTileBufferDesc(kNumCubeMapFaces *  m_pPointLightRenderResources->GetNumLights(), sizeof(ShadowMapTile), true, true);
	m_pPointLightShadowMapTileBuffer = new Buffer(m_pRenderEnv, m_pRenderEnv->m_pDefaultHeapProps, &shadowMapTileBufferDesc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, L"m_pPointLightShadowMapTileBuffer");

	StructuredBufferDesc lightViewProjTileMatrixBufferDesc(kNumCubeMapFaces * m_pPointLightRenderResources->GetNumLights(), sizeof(Matrix4f), true, true);
	m_pPointLightViewTileProjMatrixBuffer = new Buffer(m_pRenderEnv, m_pRenderEnv->m_pDefaultHeapProps, &lightViewProjTileMatrixBufferDesc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, L"m_pPointLightShadowMapTileBuffer");

	Buffer* pLightViewProjMatrixBuffer = m_pPointLightRenderResources->GetLightViewProjMatrixBuffer();

	SetupTiledShadowMapPass::InitParams initParams;
	initParams.m_pRenderEnv = m_pRenderEnv;
	initParams.m_LightType = LightType_Point;
	initParams.m_MaxNumLights = m_pPointLightRenderResources->GetNumLights();

	m_pSetupPointLightTiledShadowMapPass = new SetupTiledShadowMapPass(&initParams);

	m_pSetupPointLightTiledShadowMapResources->m_RequiredResourceStates.emplace_back(m_pNumVisiblePointLightsBuffer, m_pNumVisiblePointLightsBuffer->GetReadState());
	m_pSetupPointLightTiledShadowMapResources->m_RequiredResourceStates.emplace_back(m_pVisiblePointLightIndexBuffer, m_pVisiblePointLightIndexBuffer->GetReadState());
	m_pSetupPointLightTiledShadowMapResources->m_RequiredResourceStates.emplace_back(pLightViewProjMatrixBuffer, pLightViewProjMatrixBuffer->GetReadState());
	m_pSetupPointLightTiledShadowMapResources->m_RequiredResourceStates.emplace_back(m_pPointLightShadowMapTileBuffer, m_pPointLightShadowMapTileBuffer->GetWriteState());
	m_pSetupPointLightTiledShadowMapResources->m_RequiredResourceStates.emplace_back(m_pPointLightViewTileProjMatrixBuffer, m_pPointLightViewTileProjMatrixBuffer->GetWriteState());

	m_pSetupPointLightTiledShadowMapResources->m_SRVHeapStart = m_pShaderVisibleSRVHeap->Allocate();
	m_pDevice->CopyDescriptor(m_pSetupPointLightTiledShadowMapResources->m_SRVHeapStart, m_pPointLightShadowMapDataBuffer->GetCBVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), m_pNumVisiblePointLightsBuffer->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), m_pVisiblePointLightIndexBuffer->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), pLightViewProjMatrixBuffer->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), m_pPointLightShadowMapTileBuffer->GetUAVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), m_pPointLightViewTileProjMatrixBuffer->GetUAVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	*/
}

void DXApplication::InitCreateRenderShadowMapCommandsPass()
{
	assert(false);
	/*
	assert((m_pPointLightRenderResources != nullptr) || (m_pSpotLightRenderResources != nullptr));

	Buffer* pMeshBoundsBuffer = m_pMeshRenderResources->GetMeshBoundsBuffer();
	Buffer* pMaterialBuffer = m_pMeshRenderResources->GetMaterialBuffer();
	Buffer* pMeshDescBuffer = m_pMeshRenderResources->GetMeshDescBuffer();

	m_pCreateRenderShadowMapCommandsArgumentBufferResources->m_RequiredResourceStates.emplace_back(m_pCreateRenderShadowMapCommandsArgumentBuffer, D3D12_RESOURCE_STATE_COPY_DEST);
	m_pCreateRenderShadowMapCommandsArgumentBufferResources->m_RequiredResourceStates.emplace_back(m_pNumVisibleMeshesBuffer, D3D12_RESOURCE_STATE_COPY_SOURCE);

	CreateRenderShadowMapCommandsPass::InitParams initParams;
	initParams.m_pRenderEnv = m_pRenderEnv;
	initParams.m_EnablePointLights = (m_pPointLightRenderResources != nullptr);
	initParams.m_MaxNumPointLightsPerShadowCaster = (m_pPointLightRenderResources != nullptr) ? m_pPointLightRenderResources->GetNumLights() : 0;
	initParams.m_EnableSpotLights = (m_pSpotLightRenderResources != nullptr);
	initParams.m_MaxNumSpotLightsPerShadowCaster = (m_pSpotLightRenderResources != nullptr) ? m_pSpotLightRenderResources->GetNumLights() : 0;

	m_pCreateRenderShadowMapCommandsPass = new CreateRenderShadowMapCommandsPass(&initParams);

	if (m_pPointLightRenderResources != nullptr)
	{
		FormattedBufferDesc shadowCastingLightIndexBufferDesc(m_pMeshRenderResources->GetNumMeshes() * m_pPointLightRenderResources->GetNumLights(), DXGI_FORMAT_R32_UINT, true, true);
		m_pShadowCastingPointLightIndexBuffer = new Buffer(m_pRenderEnv, m_pRenderEnv->m_pDefaultHeapProps, &shadowCastingLightIndexBufferDesc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, L"m_pShadowCastingPointLightIndexBuffer");

		FormattedBufferDesc numShadowCastingLightsBufferDesc(1, DXGI_FORMAT_R32_UINT, false, true);
		m_pNumShadowCastingPointLightsBuffer = new Buffer(m_pRenderEnv, m_pRenderEnv->m_pDefaultHeapProps, &numShadowCastingLightsBufferDesc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, L"m_pNumShadowCastingPointLightsBuffer");

		StructuredBufferDesc drawCommandBufferDesc(m_pMeshRenderResources->GetNumMeshes(), sizeof(DrawMeshCommand), false, true);
		m_pDrawPointLightShadowCasterCommandBuffer = new Buffer(m_pRenderEnv, m_pRenderEnv->m_pDefaultHeapProps, &drawCommandBufferDesc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, L"m_pDrawPointLightShadowCasterCommandBuffer");

		FormattedBufferDesc numShadowCastersBufferDesc(1, DXGI_FORMAT_R32_UINT, false, true);
		m_pNumDrawPointLightShadowCastersBuffer = new Buffer(m_pRenderEnv, m_pRenderEnv->m_pDefaultHeapProps, &numShadowCastersBufferDesc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, L"m_pNumDrawPointLightShadowCastersBuffer");
	}

	if (m_pSpotLightRenderResources != nullptr)
	{
		FormattedBufferDesc shadowCastingLightIndexBufferDesc(m_pMeshRenderResources->GetNumMeshes() * m_pSpotLightRenderResources->GetNumLights(), DXGI_FORMAT_R32_UINT, true, true);
		m_pShadowCastingSpotLightIndexBuffer = new Buffer(m_pRenderEnv, m_pRenderEnv->m_pDefaultHeapProps, &shadowCastingLightIndexBufferDesc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, L"m_pShadowCastingSpotLightIndexBuffer");

		FormattedBufferDesc numShadowCastingLightsBufferDesc(1, DXGI_FORMAT_R32_UINT, false, true);
		m_pNumShadowCastingSpotLightsBuffer = new Buffer(m_pRenderEnv, m_pRenderEnv->m_pDefaultHeapProps, &numShadowCastingLightsBufferDesc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, L"m_pNumShadowCastingSpotLightsBuffer");

		StructuredBufferDesc drawCommandBufferDesc(m_pMeshRenderResources->GetNumMeshes(), sizeof(DrawMeshCommand), false, true);
		m_pDrawSpotLightShadowCasterCommandBuffer = new Buffer(m_pRenderEnv, m_pRenderEnv->m_pDefaultHeapProps, &drawCommandBufferDesc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, L"m_pDrawSpotLightShadowCasterCommandBuffer");

		FormattedBufferDesc numShadowCastersBufferDesc(1, DXGI_FORMAT_R32_UINT, false, true);
		m_pNumDrawSpotLightShadowCastersBuffer = new Buffer(m_pRenderEnv, m_pRenderEnv->m_pDefaultHeapProps, &numShadowCastersBufferDesc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, L"m_pNumDrawSpotLightShadowCastersBuffer");
	}

	m_pCreateRenderShadowMapCommandsResources->m_RequiredResourceStates.emplace_back(m_pVisibleMeshIndexBuffer, m_pVisibleMeshIndexBuffer->GetReadState());
	m_pCreateRenderShadowMapCommandsResources->m_RequiredResourceStates.emplace_back(pMeshBoundsBuffer, pMeshBoundsBuffer->GetReadState());
	m_pCreateRenderShadowMapCommandsResources->m_RequiredResourceStates.emplace_back(pMeshDescBuffer, pMeshDescBuffer->GetReadState());
	m_pCreateRenderShadowMapCommandsResources->m_RequiredResourceStates.emplace_back(m_pCreateRenderShadowMapCommandsArgumentBuffer, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);

	m_pCreateRenderShadowMapCommandsResources->m_SRVHeapStart = m_pShaderVisibleSRVHeap->Allocate();
	m_pDevice->CopyDescriptor(m_pCreateRenderShadowMapCommandsResources->m_SRVHeapStart, m_pVisibleMeshIndexBuffer->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), pMeshBoundsBuffer->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), pMeshDescBuffer->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	if (m_pPointLightRenderResources != nullptr)
	{
		Buffer* pLightBoundsBuffer = m_pPointLightRenderResources->GetLightBoundsBuffer();

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

	if (m_pSpotLightRenderResources != nullptr)
	{
		Buffer* pLightBoundsBuffer = m_pSpotLightRenderResources->GetLightBoundsBuffer();

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
	*/
}

void DXApplication::InitRenderSpotLightTiledShadowMapPass()
{
	assert(false);
	/*
	assert(m_pSpotLightRenderResources != nullptr);

	const u32 tiledShadowMapWidth = m_pSpotLightRenderResources->GetNumLights() * kShadowMapTileSize;
	const u32 tiledShadowMapHeight = kShadowMapTileSize;

	m_pSpotLightTiledShadowMapViewport = new Viewport(0.0f, 0.0f, (f32)tiledShadowMapWidth, (f32)tiledShadowMapHeight);

	DepthStencilValue optimizedClearDepth(1.0f);
	DepthTexture2DDesc tiledShadowMapDesc(DXGI_FORMAT_R32_TYPELESS, tiledShadowMapWidth, tiledShadowMapHeight, true, true);
	m_pSpotLightTiledShadowMap = new DepthTexture(m_pRenderEnv, m_pRenderEnv->m_pDefaultHeapProps, &tiledShadowMapDesc, D3D12_RESOURCE_STATE_DEPTH_WRITE, &optimizedClearDepth, L"m_pSpotLightTiledShadowMap");

	Buffer* pLightPropsBuffer = m_pSpotLightRenderResources->GetLightPropsBuffer();
	Buffer* pLightFrustumBuffer = m_pSpotLightRenderResources->GetLightFrustumBuffer();

	RenderTiledShadowMapPass::InitParams initParams;
	initParams.m_pRenderEnv = m_pRenderEnv;
	initParams.m_DSVFormat = GetDepthStencilViewFormat(m_pSpotLightTiledShadowMap->GetFormat());
	initParams.m_pMeshBatch = m_pMeshRenderResources;
	initParams.m_LightType = LightType_Spot;

	m_pRenderSpotLightTiledShadowMapPass = new RenderTiledShadowMapPass(&initParams);
		
	m_pRenderSpotLightTiledShadowMapResources->m_RequiredResourceStates.emplace_back(m_pSpotLightTiledShadowMap, m_pSpotLightTiledShadowMap->GetWriteState());
	m_pRenderSpotLightTiledShadowMapResources->m_RequiredResourceStates.emplace_back(m_pShadowCastingSpotLightIndexBuffer, m_pShadowCastingSpotLightIndexBuffer->GetReadState());
	m_pRenderSpotLightTiledShadowMapResources->m_RequiredResourceStates.emplace_back(pLightPropsBuffer, pLightPropsBuffer->GetReadState());
	m_pRenderSpotLightTiledShadowMapResources->m_RequiredResourceStates.emplace_back(m_pSpotLightViewTileProjMatrixBuffer, m_pSpotLightViewTileProjMatrixBuffer->GetReadState());
	m_pRenderSpotLightTiledShadowMapResources->m_RequiredResourceStates.emplace_back(pLightFrustumBuffer, pLightFrustumBuffer->GetReadState());
	m_pRenderSpotLightTiledShadowMapResources->m_RequiredResourceStates.emplace_back(m_pDrawSpotLightShadowCasterCommandBuffer, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
	m_pRenderSpotLightTiledShadowMapResources->m_RequiredResourceStates.emplace_back(m_pNumDrawSpotLightShadowCastersBuffer, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);

	m_pRenderSpotLightTiledShadowMapResources->m_DSVHeapStart = m_pSpotLightTiledShadowMap->GetDSVHandle();
	m_pRenderSpotLightTiledShadowMapResources->m_SRVHeapStart = m_pShaderVisibleSRVHeap->Allocate();

	m_pDevice->CopyDescriptor(m_pRenderSpotLightTiledShadowMapResources->m_SRVHeapStart, m_pObjectTransformBuffer->GetCBVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), m_pShadowCastingSpotLightIndexBuffer->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), pLightPropsBuffer->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), m_pSpotLightViewTileProjMatrixBuffer->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), pLightFrustumBuffer->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	*/
}

void DXApplication::InitRenderPointLightTiledShadowMapPass()
{
	assert(false);
	/*
	assert(m_pPointLightRenderResources != nullptr);

	const u32 tiledShadowMapWidth = m_pPointLightRenderResources->GetNumLights() * kShadowMapTileSize;
	const u32 tiledShadowMapHeight = kNumCubeMapFaces * kShadowMapTileSize;

	m_pPointLightTiledShadowMapViewport = new Viewport(0.0f, 0.0f, (f32)tiledShadowMapWidth, (f32)tiledShadowMapHeight);

	DepthStencilValue optimizedClearDepth(1.0f);
	DepthTexture2DDesc tiledShadowMapDesc(DXGI_FORMAT_R32_TYPELESS, tiledShadowMapWidth, tiledShadowMapHeight, true, true);
	m_pPointLightTiledShadowMap = new DepthTexture(m_pRenderEnv, m_pRenderEnv->m_pDefaultHeapProps, &tiledShadowMapDesc, D3D12_RESOURCE_STATE_DEPTH_WRITE, &optimizedClearDepth, L"m_pPointLightTiledShadowMap");

	Buffer* pLightBoundsBuffer = m_pPointLightRenderResources->GetLightBoundsBuffer();
	Buffer* pLightFrustumBuffer = m_pPointLightRenderResources->GetLightFrustumBuffer();

	RenderTiledShadowMapPass::InitParams initParams;
	initParams.m_pRenderEnv = m_pRenderEnv;
	initParams.m_DSVFormat = GetDepthStencilViewFormat(m_pPointLightTiledShadowMap->GetFormat());
	initParams.m_pMeshBatch = m_pMeshRenderResources;
	initParams.m_LightType = LightType_Point;

	m_pRenderPointLightTiledShadowMapPass = new RenderTiledShadowMapPass(&initParams);

	m_pRenderPointLightTiledShadowMapResources->m_RequiredResourceStates.emplace_back(m_pPointLightTiledShadowMap, m_pPointLightTiledShadowMap->GetWriteState());
	m_pRenderPointLightTiledShadowMapResources->m_RequiredResourceStates.emplace_back(m_pShadowCastingPointLightIndexBuffer, m_pShadowCastingPointLightIndexBuffer->GetReadState());
	m_pRenderPointLightTiledShadowMapResources->m_RequiredResourceStates.emplace_back(pLightBoundsBuffer, pLightBoundsBuffer->GetReadState());
	m_pRenderPointLightTiledShadowMapResources->m_RequiredResourceStates.emplace_back(m_pPointLightViewTileProjMatrixBuffer, m_pPointLightViewTileProjMatrixBuffer->GetReadState());
	m_pRenderPointLightTiledShadowMapResources->m_RequiredResourceStates.emplace_back(pLightFrustumBuffer, pLightFrustumBuffer->GetReadState());
	m_pRenderPointLightTiledShadowMapResources->m_RequiredResourceStates.emplace_back(m_pDrawPointLightShadowCasterCommandBuffer, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
	m_pRenderPointLightTiledShadowMapResources->m_RequiredResourceStates.emplace_back(m_pNumDrawPointLightShadowCastersBuffer, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);

	m_pRenderPointLightTiledShadowMapResources->m_DSVHeapStart = m_pPointLightTiledShadowMap->GetDSVHandle();
	m_pRenderPointLightTiledShadowMapResources->m_SRVHeapStart = m_pShaderVisibleSRVHeap->Allocate();

	m_pDevice->CopyDescriptor(m_pRenderPointLightTiledShadowMapResources->m_SRVHeapStart, m_pObjectTransformBuffer->GetCBVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), m_pShadowCastingPointLightIndexBuffer->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), pLightBoundsBuffer->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), m_pPointLightViewTileProjMatrixBuffer->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), pLightFrustumBuffer->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	*/
}

void DXApplication::InitPropagateLightPass()
{
	assert(false);
	/*
	PropagateLightPass::InitParams initParams;
	initParams.m_pRenderEnv = m_pRenderEnv;
	initParams.m_NumGridCellsX = kNumGridCellsX;
	initParams.m_NumGridCellsY = kNumGridCellsY;
	initParams.m_NumGridCellsZ = kNumGridCellsZ;

	m_pPropagateLightPass = new PropagateLightPass(&initParams);

	for (u8 index = 0; index < 2; ++index)
	{
		ColorTexture* pPrevIntensityRCoeffsTexture = m_IntensityRCoeffsTextures[index];
		ColorTexture* pPrevIntensityGCoeffsTexture = m_IntensityGCoeffsTextures[index];
		ColorTexture* pPrevIntensityBCoeffsTexture = m_IntensityBCoeffsTextures[index];
		
		const u8 outTexIndex = (index + 1) % 2;
		ColorTexture* pIntensityRCoeffsTexture = m_IntensityRCoeffsTextures[outTexIndex];
		ColorTexture* pIntensityGCoeffsTexture = m_IntensityGCoeffsTextures[outTexIndex];
		ColorTexture* pIntensityBCoeffsTexture = m_IntensityBCoeffsTextures[outTexIndex];

		m_PropagateLightResources[index]->m_RequiredResourceStates.emplace_back(m_pGridBuffer, m_pGridBuffer->GetReadState());

		m_PropagateLightResources[index]->m_RequiredResourceStates.emplace_back(pPrevIntensityRCoeffsTexture, pPrevIntensityRCoeffsTexture->GetReadState());
		m_PropagateLightResources[index]->m_RequiredResourceStates.emplace_back(pPrevIntensityGCoeffsTexture, pPrevIntensityGCoeffsTexture->GetReadState());
		m_PropagateLightResources[index]->m_RequiredResourceStates.emplace_back(pPrevIntensityBCoeffsTexture, pPrevIntensityBCoeffsTexture->GetReadState());
				
		m_PropagateLightResources[index]->m_RequiredResourceStates.emplace_back(pIntensityRCoeffsTexture, pIntensityRCoeffsTexture->GetWriteState());
		m_PropagateLightResources[index]->m_RequiredResourceStates.emplace_back(pIntensityGCoeffsTexture, pIntensityGCoeffsTexture->GetWriteState());
		m_PropagateLightResources[index]->m_RequiredResourceStates.emplace_back(pIntensityBCoeffsTexture, pIntensityBCoeffsTexture->GetWriteState());

		m_PropagateLightResources[index]->m_RequiredResourceStates.emplace_back(m_pAccumIntensityRCoeffsTexture, m_pAccumIntensityRCoeffsTexture->GetWriteState());
		m_PropagateLightResources[index]->m_RequiredResourceStates.emplace_back(m_pAccumIntensityGCoeffsTexture, m_pAccumIntensityGCoeffsTexture->GetWriteState());
		m_PropagateLightResources[index]->m_RequiredResourceStates.emplace_back(m_pAccumIntensityBCoeffsTexture, m_pAccumIntensityBCoeffsTexture->GetWriteState());

		m_PropagateLightResources[index]->m_SRVHeapStart = m_pShaderVisibleSRVHeap->Allocate();
		m_pDevice->CopyDescriptor(m_PropagateLightResources[index]->m_SRVHeapStart, m_pGridConfigDataBuffer->GetCBVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), m_pGridBuffer->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), pPrevIntensityRCoeffsTexture->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), pPrevIntensityGCoeffsTexture->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), pPrevIntensityBCoeffsTexture->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		
		m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), pIntensityRCoeffsTexture->GetUAVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), pIntensityGCoeffsTexture->GetUAVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), pIntensityBCoeffsTexture->GetUAVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), m_pAccumIntensityRCoeffsTexture->GetUAVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), m_pAccumIntensityGCoeffsTexture->GetUAVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), m_pAccumIntensityBCoeffsTexture->GetUAVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	}
	*/
}

void DXApplication::InitVisualizeSpotLightTiledShadowMapPass()
{
	assert(false);
	/*
	assert(m_pSpotLightTiledShadowMap != nullptr);

	VisualizeTexturePass::InitParams initParams;
	initParams.m_pRenderEnv = m_pRenderEnv;
	initParams.m_RTVFormat = GetRenderTargetViewFormat(m_pSwapChain->GetBackBuffer(m_BackBufferIndex)->GetFormat());
	initParams.m_TextureType = VisualizeTexturePass::TextureType_Depth;

	m_pVisualizeSpotLightTiledShadowMapPass = new VisualizeTexturePass(&initParams);

	for (u8 index = 0; index < kNumBackBuffers; ++index)
	{
		ColorTexture* pRenderTarget = m_pSwapChain->GetBackBuffer(index);

		m_VisualizeSpotLightTiledShadowMapResources[index]->m_RequiredResourceStates.emplace_back(pRenderTarget, pRenderTarget->GetWriteState());
		m_VisualizeSpotLightTiledShadowMapResources[index]->m_RequiredResourceStates.emplace_back(m_pSpotLightTiledShadowMap, m_pSpotLightTiledShadowMap->GetReadState());

		m_VisualizeSpotLightTiledShadowMapResources[index]->m_RTVHeapStart = pRenderTarget->GetRTVHandle();
		m_VisualizeSpotLightTiledShadowMapResources[index]->m_SRVHeapStart = m_pShaderVisibleSRVHeap->Allocate();

		m_pDevice->CopyDescriptor(m_VisualizeSpotLightTiledShadowMapResources[index]->m_SRVHeapStart, m_pVisualizeTextureDataBuffer->GetCBVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), m_pSpotLightTiledShadowMap->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	}
	*/
}

void DXApplication::InitVisualizePointLightTiledShadowMapPass()
{
	assert(false);
	/*
	assert(m_pPointLightTiledShadowMap != nullptr);

	VisualizeTexturePass::InitParams initParams;
	initParams.m_pRenderEnv = m_pRenderEnv;
	initParams.m_RTVFormat = GetRenderTargetViewFormat(m_pSwapChain->GetBackBuffer(m_BackBufferIndex)->GetFormat());
	initParams.m_TextureType = VisualizeTexturePass::TextureType_Depth;

	m_pVisualizePointLightTiledShadowMapPass = new VisualizeTexturePass(&initParams);

	for (u8 index = 0; index < kNumBackBuffers; ++index)
	{
		ColorTexture* pRenderTarget = m_pSwapChain->GetBackBuffer(index);

		m_VisualizePointLightTiledShadowMapResources[index]->m_RequiredResourceStates.emplace_back(pRenderTarget, pRenderTarget->GetWriteState());
		m_VisualizePointLightTiledShadowMapResources[index]->m_RequiredResourceStates.emplace_back(m_pPointLightTiledShadowMap, m_pPointLightTiledShadowMap->GetReadState());

		m_VisualizePointLightTiledShadowMapResources[index]->m_RTVHeapStart = pRenderTarget->GetRTVHandle();
		m_VisualizePointLightTiledShadowMapResources[index]->m_SRVHeapStart = m_pShaderVisibleSRVHeap->Allocate();

		m_pDevice->CopyDescriptor(m_VisualizePointLightTiledShadowMapResources[index]->m_SRVHeapStart, m_pVisualizeTextureDataBuffer->GetCBVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), m_pPointLightTiledShadowMap->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	}
	*/
}

CommandList* DXApplication::RecordUpdateCreateRenderShadowMapCommandsArgumentBufferPass()
{
	assert(false);
	/*
	CommandList* pCommandList = m_pCommandListPool->Create(L"pUpdateCreateRenderShadowMapCommandsArgumentBufferCommandList");
	
	pCommandList->Begin();
	pCommandList->SetRequiredResourceStates(&m_pCreateRenderShadowMapCommandsArgumentBufferResources->m_RequiredResourceStates);
	pCommandList->CopyBufferRegion(m_pCreateRenderShadowMapCommandsArgumentBuffer, 0, m_pNumVisibleMeshesBuffer, 0, sizeof(u32));
	pCommandList->End();

	return pCommandList;
	*/
	return nullptr;
}

CommandList* DXApplication::RecordCreateRenderShadowMapCommandsPass()
{
	assert(false);
	/*
	CreateRenderShadowMapCommandsPass::RenderParams renderParams;
	renderParams.m_pRenderEnv = m_pRenderEnv;
	renderParams.m_pCommandList = m_pCommandListPool->Create(L"pCreateRenderShadowMapCommandsCommandList");
	renderParams.m_pResources = m_pCreateRenderShadowMapCommandsResources;
	renderParams.m_pIndirectArgumentBuffer = m_pCreateRenderShadowMapCommandsArgumentBuffer;
	renderParams.m_pNumShadowCastingPointLightsBuffer = m_pNumShadowCastingPointLightsBuffer;
	renderParams.m_pNumDrawPointLightShadowCastersBuffer = m_pNumDrawPointLightShadowCastersBuffer;
	renderParams.m_pNumShadowCastingSpotLightsBuffer = m_pNumShadowCastingSpotLightsBuffer;
	renderParams.m_pNumDrawSpotLightShadowCastersBuffer = m_pNumDrawSpotLightShadowCastersBuffer;

	m_pCreateRenderShadowMapCommandsPass->Record(&renderParams);
	return renderParams.m_pCommandList;
	*/
	return nullptr;
}

CommandList* DXApplication::RecordSetupSpotLightTiledShadowMapPass()
{
	assert(false);
	/*
	SetupTiledShadowMapPass::RenderParams renderParams;
	renderParams.m_pRenderEnv = m_pRenderEnv;
	renderParams.m_pCommandList = m_pCommandListPool->Create(L"pSetupSpotLightTiledShadowMapCommandList");
	renderParams.m_pResources = m_pSetupSpotLightTiledShadowMapResources;

	m_pSetupSpotLightTiledShadowMapPass->Record(&renderParams);
	return renderParams.m_pCommandList;
	*/
	return nullptr;
}

CommandList* DXApplication::RecordSetupPointLightTiledShadowMapPass()
{
	assert(false);
	/*
	SetupTiledShadowMapPass::RenderParams renderParams;
	renderParams.m_pRenderEnv = m_pRenderEnv;
	renderParams.m_pCommandList = m_pCommandListPool->Create(L"pSetupPointLightTiledShadowMapCommandList");
	renderParams.m_pResources = m_pSetupPointLightTiledShadowMapResources;

	m_pSetupPointLightTiledShadowMapPass->Record(&renderParams);
	return renderParams.m_pCommandList;
	*/
	return nullptr;
}

CommandList* DXApplication::RecordRenderSpotLightTiledShadowMapPass()
{
	assert(false);
	/*
	RenderTiledShadowMapPass::RenderParams renderParams;
	renderParams.m_pRenderEnv = m_pRenderEnv;
	renderParams.m_pCommandList = m_pCommandListPool->Create(L"pRenderSpotLightTiledShadowMapCommandList");
	renderParams.m_pResources = m_pRenderSpotLightTiledShadowMapResources;
	renderParams.m_pViewport = m_pSpotLightTiledShadowMapViewport;
	renderParams.m_pMeshBatch = m_pMeshRenderResources;
	renderParams.m_pDrawShadowCasterCommandBuffer = m_pDrawSpotLightShadowCasterCommandBuffer;
	renderParams.m_pNumDrawShadowCastersBuffer = m_pNumDrawSpotLightShadowCastersBuffer;

	m_pRenderSpotLightTiledShadowMapPass->Record(&renderParams);
	return renderParams.m_pCommandList;
	*/
	return nullptr;
}

CommandList* DXApplication::RecordRenderPointLightTiledShadowMapPass()
{
	assert(false);
	/*
	RenderTiledShadowMapPass::RenderParams renderParams;
	renderParams.m_pRenderEnv = m_pRenderEnv;
	renderParams.m_pCommandList = m_pCommandListPool->Create(L"pRenderPointLightTiledShadowMapCommandList");
	renderParams.m_pResources = m_pRenderPointLightTiledShadowMapResources;
	renderParams.m_pViewport = m_pPointLightTiledShadowMapViewport;
	renderParams.m_pMeshBatch = m_pMeshRenderResources;
	renderParams.m_pDrawShadowCasterCommandBuffer = m_pDrawPointLightShadowCasterCommandBuffer;
	renderParams.m_pNumDrawShadowCastersBuffer = m_pNumDrawPointLightShadowCastersBuffer;

	m_pRenderPointLightTiledShadowMapPass->Record(&renderParams);
	return renderParams.m_pCommandList;
	*/
	return nullptr;
}

CommandList* DXApplication::RecordPropagateLightPass()
{
	assert(false);
	/*
	PropagateLightPass::RenderParams renderParams;
	renderParams.m_pRenderEnv = m_pRenderEnv;
	renderParams.m_pCommandList = m_pCommandListPool->Create(L"pPropagateLightCommandList");
	renderParams.m_ppResources = m_PropagateLightResources;
	renderParams.m_NumIterations = m_NumPropagationIterations;

	m_pPropagateLightPass->Record(&renderParams);
	return renderParams.m_pCommandList;
	*/
	return nullptr;
}

CommandList* DXApplication::RecordVisualizeSpotLightTiledShadowMapPass()
{
	assert(false);
	/*
	VisualizeTexturePass::RenderParams renderParams;
	renderParams.m_pRenderEnv = m_pRenderEnv;
	renderParams.m_pCommandList = m_pCommandListPool->Create(L"pVisualizeSpotLightShadowMapCommandList");
	renderParams.m_pResources = m_VisualizeSpotLightTiledShadowMapResources[m_BackBufferIndex];
	renderParams.m_pViewport = m_pBackBufferViewport;

	m_pVisualizeSpotLightTiledShadowMapPass->Record(&renderParams);
	return renderParams.m_pCommandList;
	*/
	return nullptr;
}

CommandList* DXApplication::RecordVisualizePointLightTiledShadowMapPass()
{
	assert(false);
	/*
	VisualizeTexturePass::RenderParams renderParams;
	renderParams.m_pRenderEnv = m_pRenderEnv;
	renderParams.m_pCommandList = m_pCommandListPool->Create(L"pVisualizePointLightShadowMapCommandList");
	renderParams.m_pResources = m_VisualizePointLightTiledShadowMapResources[m_BackBufferIndex];
	renderParams.m_pViewport = m_pBackBufferViewport;

	m_pVisualizePointLightTiledShadowMapPass->Record(&renderParams);
	return renderParams.m_pCommandList;
	*/
	return nullptr;
}

CommandList* DXApplication::RecordDisplayResultPass()
{
	if (m_DisplayResult == DisplayResult::ShadingResult)
		return RecordVisualizeAccumLightPass();

	if (m_DisplayResult == DisplayResult::DepthBuffer)
		return RecordVisualizeDepthBufferPass();

	if (m_DisplayResult == DisplayResult::ReprojectedDepthBuffer)
		return RecordVisualizeReprojectedDepthBufferPass();
				
	if (m_DisplayResult == DisplayResult::NormalBuffer)
		return RecordVisualizeNormalBufferPass();

	if (m_DisplayResult == DisplayResult::TexCoordBuffer)
		return RecordVisualizeTexCoordBufferPass();

	if (m_DisplayResult == DisplayResult::DepthBufferWithMeshType)
		return RecordVisualizeDepthBufferWithMeshTypePass();

	if (m_DisplayResult == DisplayResult::VoxelRelectance)
		return RecordVisualizeVoxelReflectancePass();
		
	assert(false);
	return nullptr;
}

CommandList* DXApplication::RecordPostRenderPass()
{
	CommandList* pCommandList = m_pCommandListPool->Create(L"pPostRenderCommandList");
	pCommandList->Begin();
	
	{
		ColorTexture* pRenderTarget = m_pSwapChain->GetBackBuffer(m_BackBufferIndex);
		ResourceBarrier resourceBarrier(pRenderTarget, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
		pCommandList->ResourceBarrier(1, &resourceBarrier);
	}
	if (m_DisplayResult == DisplayResult::ShadingResult)
	{
		ResourceBarrier resourceBarrier(m_pAccumLightTexture, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
		pCommandList->ResourceBarrier(1, &resourceBarrier);
	}
	else if (m_DisplayResult == DisplayResult::ReprojectedDepthBuffer)
	{
		ResourceBarrier resourceBarrier(m_pDownscaleAndReprojectDepthPass->GetReprojectedDepthTexture(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_DEPTH_WRITE);
		pCommandList->ResourceBarrier(1, &resourceBarrier);
	}
	else if (m_DisplayResult == DisplayResult::DepthBufferWithMeshType)
	{
		const FillMeshTypeDepthBufferPass::ResourceStates* pResourceStates = m_pFillMeshTypeDepthBufferPass->GetOutputResourceStates();
		ResourceBarrier resourceBarrier(m_pFillMeshTypeDepthBufferPass->GetMeshTypeDepthTexture(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_DEPTH_WRITE);
		pCommandList->ResourceBarrier(1, &resourceBarrier);
	}
	else if (m_DisplayResult == DisplayResult::VoxelRelectance)
	{
		ResourceBarrier resourceBarrier(m_pVoxelizePass->GetVoxelReflectanceTexture(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		pCommandList->ResourceBarrier(1, &resourceBarrier);
	}
	
	pCommandList->End();
	return pCommandList;
}

void DXApplication::UpdateDisplayResult(DisplayResult displayResult)
{
	m_DisplayResult = displayResult;
	if (m_DisplayResult == DisplayResult::ShadingResult)
	{
		if (m_ShadingMode == TileShadingMode::DirectAndIndirectLight)
		{
			assert(false);
		}
		else if (m_ShadingMode == TileShadingMode::DirectLight)
		{
			m_pWindow->SetWindowText(L"Direct lighting");
		}
		else if (m_ShadingMode == TileShadingMode::IndirectLight)
		{
			assert(false);
		}
		else
		{
			assert(false);
		}
	}
	else if (m_DisplayResult == DisplayResult::TexCoordBuffer)
	{
		m_pWindow->SetWindowText(L"Texture coordinate buffer");
	}
	else if (m_DisplayResult == DisplayResult::NormalBuffer)
	{
		m_pWindow->SetWindowText(L"Normal buffer");
	}
	else if (m_DisplayResult == DisplayResult::DepthBuffer)
	{
		m_pWindow->SetWindowText(L"Depth buffer");
	}
	else if (m_DisplayResult == DisplayResult::ReprojectedDepthBuffer)
	{
		m_pWindow->SetWindowText(L"Reprojected depth buffer");
	}
	else if (m_DisplayResult == DisplayResult::SpotLightTiledShadowMap)
	{
		m_pWindow->SetWindowText(L"Spot light tiled shadow map");
	}
	else if (m_DisplayResult == DisplayResult::PointLightTiledShadowMap)
	{
		m_pWindow->SetWindowText(L"Point light tiled shadow map");
	}
	else if (m_DisplayResult == DisplayResult::VoxelRelectance)
	{
		m_pWindow->SetWindowText(L"Voxel reflectance");
	}
	else if (m_DisplayResult == DisplayResult::DepthBufferWithMeshType)
	{
		m_pWindow->SetWindowText(L"Depth buffer with mesh type");
	}
	else
	{
		assert(false);
	}
}

#ifdef DEBUG_RENDER_PASS
void DXApplication::OuputDebugRenderPassResult()
{
	static unsigned frameNumber = 0;
	static unsigned frameNumberForOutput = 5;

	++frameNumber;

	if (frameNumber == frameNumberForOutput)
	{
		struct DrawCommand
		{
			UINT m_InstanceOffset;
			UINT m_MaterialIndex;
			DrawIndexedArguments m_Args;
		};

		OutputDebugStringA("1.Debug =========================\n");
		using ElementType = DrawCommand;

		auto elementFormatter = [](const void* pElementData)
		{
			const ElementType* pElement = (ElementType*)pElementData;

			std::stringstream stringStream;
			stringStream << "m_InstanceOffset: " << pElement->m_InstanceOffset << "\n"
				<< "m_MaterialIndex: " << pElement->m_MaterialIndex << "\n"
				<< "m_IndexCountPerInstance: " << pElement->m_Args.m_IndexCountPerInstance << "\n"
				<< "m_InstanceCount: " << pElement->m_Args.m_InstanceCount << "\n"
				<< "m_StartIndexLocation: " << pElement->m_Args.m_StartIndexLocation << "\n"
				<< "m_BaseVertexLocation: " << pElement->m_Args.m_BaseVertexLocation << "\n"
				<< "m_StartInstanceLocation: " << pElement->m_Args.m_StartInstanceLocation << "\n";
			
			return stringStream.str();
		};
		OutputBufferContent(m_pRenderEnv,
			m_pCreateVoxelizeCommandsPass->GetVoxelizeCommandBuffer(),
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
			sizeof(ElementType),
			elementFormatter);

		OutputDebugStringA("2.Debug =========================\n");
	}
}
#endif // DEBUG_RENDER_PASS