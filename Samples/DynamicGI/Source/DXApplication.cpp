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
#include "D3DWrapper/GPUProfiler.h"
#include "RenderPasses/CreateShadowMapCommandsPass.h"
#include "RenderPasses/CreateVoxelizeCommandsPass.h"
#include "RenderPasses/ConvertTiledShadowMapPass.h"
#include "RenderPasses/PropagateLightPass.h"
#include "RenderPasses/RenderGBufferPass.h"
#include "RenderPasses/TiledLightCullingPass.h"
#include "RenderPasses/TiledShadingPass.h"
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
#include "Common/MaterialRenderResources.h"
#include "Common/Color.h"
#include "Common/Camera.h"
#include "Common/Scene.h"
#include "Common/SceneLoader.h"
#include "Common/CPUProfiler.h"
#include "Math/BasisAxes.h"
#include "Math/Cone.h"
#include "Math/Frustum.h"
#include "Math/Matrix4.h"
#include "Math/OverlapTest.h"
#include "Math/Sphere.h"
#include "Math/Transform.h"
#include "Math/Vector3.h"
#include "Math/Vector4.h"

/*
To do:
- I am using Sphere as bounding volume for SpotLights. Investigate if there are better alternatives. Check https://bartwronski.com/ implementation for the cone test.
- Hard-coded light screen area for now. Fix CalcScreenAreaAffectedByLight.
- Depth and shadow maps are using DXGI_FORMAT_R32_TYPELESS format. Check if 16 bit format would suffice
- Geometry popping when rotating the camera. See reprojected depth buffer for the result.
- Enable VoxelizePass. Fix calculation for frustum corners
- VoxelizationPass does not pass point/spot light count to the shader
- Move upload visible light data to separate render pass
- Point light data in the scene object has bad CPU cache coherence when doing frustum culling
- Spot light data in the scene object has bad CPU cache coherence when doing frustum culling
- Cannot use ROV and 1 render pass for voxelization. ROV is messed up when specifying projection dominant axis from geometry shader.
  When doing 3 render passes for voxelization should also be careful as the same triangle could be rendered multiple times,
  meaning that some voxel values will be over-represented. One of possible solutions is to still do 3 render passes,
  but in geometry shader check if primitive dominant projection axis is equal to the current view projection axis.
  If it is not, the primitive should not be emitted from the geometry shader.
- VoxelizePass is using 4th component for calculating how many objects overlap the voxel. Should be set to opacity instead.
- Last section of constants in AppData constant buffer is not properly padded.
- Directional light m_EnableDirectionalLight seems to be disabled on all render passes. Enable
- Verify voxel texture position is compatible with texture coordinates in VoxelizePS.hlsl and VisualizeVoxelGridPS.hlsl
- VoxelizePass and TiledShadingPass make copy of material descriptors. Reuse material descriptors between them
- When injecting reflected radiance into voxel grid, add shadow map contribution.
- Add support for mip levels. Search for keyword "KolyaMipLevels"
- Review light view matrix computation for shadow maps in LightRenderResources.
- OOB for a set of points mimics AABB. Improve implementation.
- When converting plane mesh to unit cube space, world matrix is not optimal for OOB.
For an example, plane in original coordinates is passing through point (0, 0, 0).
OOB will have coordinates expanding from -1 to 1 not merely passing through 0 when world matrix is applied.
- Check Torque3D engine for plane object implementation. mPlane.h and mPlane.cpp
- Make camera transform part of Scene object.
Can check format OpenGEX for inspiration - http://opengex.org/
- MeshRenderResources, LightRenderResources, MaterialRenderResources should not be part of Common folder
- Using std::experimental::filesystem::path from OBJFileLoader. Should be consistent with the code.
- Review to-dos
- Use output resource states to initialize input resource states from the previous pass.
- Use Task graph for resource state transition after each render pass.
https://patterns.eecs.berkeley.edu/?page_id=609
- Fix compilation warnings for x64 build
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
		Plane m_CameraWorldFrustumPlanes[Frustum::NumPlanes];
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
		f32 m_NotUsed5;
		f32 m_NotUsed6[12];
		f32 m_NotUsed7[16];
		f32 m_NotUsed8[16];
		f32 m_NotUsed9[16];
	};

	struct PointLightProps
	{
		Vector3f m_Color;
	};

	struct SpotLightProps
	{
		Vector3f m_Color;
		Vector3f m_WorldSpaceDir;
		f32 m_LightRange;
		f32 m_CosHalfInnerConeAngle;
		f32 m_CosHalfOuterConeAngle;
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
	
	u32 CalcScreenAreaAffectedByLight(const Sphere& lightWorldBounds, const Vector2f& screenSize, const Camera& camera)
	{
		// Based on chapter Scissor Test from book "Practical Rendering and Computation with Direct3D 11"
		// by Jason Zink, Matt Pettineo, Jack Hoxley

		const Matrix4f& viewMatrix = camera.GetViewMatrix();
		const Matrix4f& projMatrix = camera.GetProjMatrix();
		const f32 distToNearClipPlane = camera.GetNearClipPlane();
		const f32 distToFarClipPlane = camera.GetFarClipPlane();

		const Vector3f viewSpaceCenter = TransformPoint(lightWorldBounds.m_Center, viewMatrix);
		const f32 radius = lightWorldBounds.m_Radius;

		Vector3f viewSpaceLeft = viewSpaceCenter - Vector3f(radius, 0.0f, 0.0f);
		Vector3f viewSpaceRight = viewSpaceCenter + Vector3f(radius, 0.0f, 0.0f);
		Vector3f viewSpaceBottom = viewSpaceCenter - Vector3f(0.0f, radius, 0.0f);
		Vector3f viewSpaceTop = viewSpaceCenter + Vector3f(0.0f, radius, 0.0f);

		viewSpaceLeft.m_Z = (viewSpaceLeft.m_X < 0.0f) ? (viewSpaceLeft.m_Z - radius) : (viewSpaceLeft.m_Z + radius);
		viewSpaceRight.m_Z = (viewSpaceRight.m_X < 0.0f) ? (viewSpaceRight.m_Z + radius) : (viewSpaceRight.m_Z - radius);
		viewSpaceBottom.m_Z = (viewSpaceBottom.m_Y < 0.0f) ? (viewSpaceBottom.m_Z - radius) : (viewSpaceBottom.m_Z + radius);
		viewSpaceTop.m_Z = (viewSpaceTop.m_Y < 0.0f) ? (viewSpaceTop.m_Z + radius) : (viewSpaceTop.m_Z - radius);

		viewSpaceLeft.m_Z = Clamp(distToNearClipPlane, distToFarClipPlane, viewSpaceLeft.m_Z);
		viewSpaceRight.m_Z = Clamp(distToNearClipPlane, distToFarClipPlane, viewSpaceRight.m_Z);
		viewSpaceBottom.m_Z = Clamp(distToNearClipPlane, distToFarClipPlane, viewSpaceBottom.m_Z);
		viewSpaceTop.m_Z = Clamp(distToNearClipPlane, distToFarClipPlane, viewSpaceTop.m_Z);

		f32 postWDivideProjSpaceLeft = viewSpaceLeft.m_X * projMatrix.m_00 / viewSpaceLeft.m_Z;
		f32 postWDivideProjSpaceRight = viewSpaceRight.m_X * projMatrix.m_00 / viewSpaceRight.m_Z;
		f32 postWDivideProjSpaceBottom = viewSpaceBottom.m_Y * projMatrix.m_11 / viewSpaceBottom.m_Z;
		f32 postWDivideProjSpaceTop = viewSpaceTop.m_Y * projMatrix.m_11 / viewSpaceTop.m_Z;

		f32 screenSpaceWidth = 0.5f * screenSize.m_X * (postWDivideProjSpaceRight - postWDivideProjSpaceLeft);
		f32 screenSpaceHeight = 0.5f * screenSize.m_Y * (postWDivideProjSpaceTop - postWDivideProjSpaceBottom);
		
		//return u32(Ceil(Max(screenSpaceWidth, screenSpaceHeight)));
		return 1024;
	}
	
	Matrix4f CreateShadowMapTileMatrix(const ShadowMapTile& shadowMapTile)
	{
		Vector2f clipSpaceTopLeft(2.0f * shadowMapTile.m_TexSpaceTopLeft.m_X - 1.0f, 1.0f - 2.0f * shadowMapTile.m_TexSpaceTopLeft.m_Y);
		f32 clipSpaceHalfSize = shadowMapTile.m_TexSpaceSize;
		
		return Matrix4f(clipSpaceHalfSize, 0.0f, 0.0f, 0.0f,
			0.0f, clipSpaceHalfSize, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f,
			clipSpaceTopLeft.m_X + clipSpaceHalfSize, clipSpaceTopLeft.m_Y - clipSpaceHalfSize, 0.0f, 1.0f);
	}
}

enum
{
	kTileSize = 16,
	kNumTilesX = 90,
	kNumTilesY = 60,
	kNumVoxelsInGridX = 128,
	kNumVoxelsInGridY = 128,
	kNumVoxelsInGridZ = 128,
	kBackBufferWidth = kNumTilesX * kTileSize,
	kBackBufferHeight = kNumTilesY * kTileSize,
	kPointLightShadowMapSize = 8 * 1024,
	kNumPointLightShadowMapLevels = 4,
	kSpotLightShadowMapSize = 1 * 1024,
	kNumSpotLightShadowMapLevels = 4
};

DXApplication::DXApplication(HINSTANCE hApp)
	: Application(hApp, L"Global Illumination", 0, 0, kBackBufferWidth, kBackBufferHeight)
	, m_pUploadHeapProps(new HeapProperties(D3D12_HEAP_TYPE_UPLOAD))
	, m_pDefaultHeapProps(new HeapProperties(D3D12_HEAP_TYPE_DEFAULT))
	, m_pReadbackHeapProps(new HeapProperties(D3D12_HEAP_TYPE_READBACK))
	, m_pRenderEnv(new RenderEnv())
{
	VerifyD3DResult(CoInitializeEx(nullptr, COINITBASE_MULTITHREADED));

	for (u8 index = 0; index < kNumBackBuffers; ++index)
		m_FrameCompletionFenceValues[index] = m_pRenderEnv->m_LastSubmissionFenceValue;
	
	UpdateDisplayResult(DisplayResult::ShadingResult);
}

DXApplication::~DXApplication()
{
	CoUninitialize();

	for (u8 index = 0; index < kNumBackBuffers; ++index)
	{
		SafeDelete(m_VisualizeAccumLightPasses[index]);
		SafeDelete(m_VisualizeNormalBufferPasses[index]);
		SafeDelete(m_VisualizeDepthBufferPasses[index]);
		SafeDelete(m_VisualizeReprojectedDepthBufferPasses[index]);
		SafeDelete(m_VisualizeTexCoordBufferPasses[index]);
		SafeDelete(m_VisualizeDepthBufferWithMeshTypePasses[index]);
		SafeDelete(m_VisualizePointLightTiledShadowMapPasses[index]);
		SafeDelete(m_VisualizeSpotLightTiledShadowMapPasses[index]);
		SafeDelete(m_VisualizeVoxelReflectancePasses[index]);

		m_UploadAppDataBuffers[index]->Unmap(0, nullptr);
		SafeDelete(m_UploadAppDataBuffers[index]);

		if (m_UploadActivePointLightWorldBoundsBuffers[index] != nullptr)
		{
			m_UploadActivePointLightWorldBoundsBuffers[index]->Unmap(0, nullptr);
			SafeDelete(m_UploadActivePointLightWorldBoundsBuffers[index]);
		}
		if (m_UploadActivePointLightPropsBuffers[index] != nullptr)
		{
			m_UploadActivePointLightPropsBuffers[index]->Unmap(0, nullptr);
			SafeDelete(m_UploadActivePointLightPropsBuffers[index]);
		}
		if (m_UploadActivePointLightWorldFrustumBuffers[index] != nullptr)
		{
			m_UploadActivePointLightWorldFrustumBuffers[index]->Unmap(0, nullptr);
			SafeDelete(m_UploadActivePointLightWorldFrustumBuffers[index]);
		}
		if (m_UploadActivePointLightViewProjMatrixBuffers[index] != nullptr)
		{
			m_UploadActivePointLightViewProjMatrixBuffers[index]->Unmap(0, nullptr);
			SafeDelete(m_UploadActivePointLightViewProjMatrixBuffers[index]);
		}
		if (m_UploadActivePointLightShadowMapTileBuffers[index] != nullptr)
		{
			m_UploadActivePointLightShadowMapTileBuffers[index]->Unmap(0, nullptr);
			SafeDelete(m_UploadActivePointLightShadowMapTileBuffers[index]);
		}
		if (m_UploadActivePointLightConvertShadowMapParamsBuffers[index] != nullptr)
		{
			m_UploadActivePointLightConvertShadowMapParamsBuffers[index]->Unmap(0, nullptr);
			SafeDelete(m_UploadActivePointLightConvertShadowMapParamsBuffers[index]);
		}

		if (m_UploadActiveSpotLightWorldBoundsBuffers[index] != nullptr)
		{
			m_UploadActiveSpotLightWorldBoundsBuffers[index]->Unmap(0, nullptr);
			SafeDelete(m_UploadActiveSpotLightWorldBoundsBuffers[index]);
		}
		if (m_UploadActiveSpotLightPropsBuffers[index] != nullptr)
		{
			m_UploadActiveSpotLightPropsBuffers[index]->Unmap(0, nullptr);
			SafeDelete(m_UploadActiveSpotLightPropsBuffers[index]);
		}
		if (m_UploadActiveSpotLightWorldFrustumBuffers[index] != nullptr)
		{
			m_UploadActiveSpotLightWorldFrustumBuffers[index]->Unmap(0, nullptr);
			SafeDelete(m_UploadActiveSpotLightWorldFrustumBuffers[index]);
		}
		if (m_UploadActiveSpotLightViewProjMatrixBuffers[index] != nullptr)
		{
			m_UploadActiveSpotLightViewProjMatrixBuffers[index]->Unmap(0, nullptr);
			SafeDelete(m_UploadActiveSpotLightViewProjMatrixBuffers[index]);
		}
		if (m_UploadActiveSpotLightShadowMapTileBuffers[index] != nullptr)
		{
			m_UploadActiveSpotLightShadowMapTileBuffers[index]->Unmap(0, nullptr);
			SafeDelete(m_UploadActiveSpotLightShadowMapTileBuffers[index]);
		}
		if (m_UploadActiveSpotLightConvertShadowMapParamsBuffers[index] != nullptr)
		{
			m_UploadActiveSpotLightConvertShadowMapParamsBuffers[index]->Unmap(0, nullptr);
			SafeDelete(m_UploadActiveSpotLightConvertShadowMapParamsBuffers[index]);
		}
	}

	SafeArrayDelete(m_pPointLights);
	SafeArrayDelete(m_ppActivePointLights);
	SafeArrayDelete(m_pSpotLights);
	SafeArrayDelete(m_ppActiveSpotLights);
	SafeDelete(m_pCPUProfiler);
	SafeDelete(m_pGPUProfiler);
	SafeDelete(m_pCamera);
	SafeDelete(m_pGeometryBuffer);
	SafeDelete(m_pMeshRenderResources);
	SafeDelete(m_pMaterialRenderResources);
	
	SafeDelete(m_pActivePointLightWorldBoundsBuffer);
	SafeDelete(m_pActivePointLightPropsBuffer);
	SafeDelete(m_pActivePointLightWorldFrustumBuffer);
	SafeDelete(m_pActivePointLightViewProjMatrixBuffer);
	SafeDelete(m_pActivePointLightShadowMapTileBuffer);
	SafeDelete(m_pActivePointLightConvertShadowMapParamsBuffer);
	
	SafeDelete(m_pActiveSpotLightWorldBoundsBuffer);
	SafeDelete(m_pActiveSpotLightPropsBuffer);
	SafeDelete(m_pActiveSpotLightWorldFrustumBuffer);
	SafeDelete(m_pActiveSpotLightViewProjMatrixBuffer);
	SafeDelete(m_pActiveSpotLightShadowMapTileBuffer);
	SafeDelete(m_pActiveSpotLightConvertShadowMapParamsBuffer);
	
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
	SafeDelete(m_pTiledLightCullingPass);
	SafeDelete(m_pCreateShadowMapCommandsPass);
	SafeDelete(m_pRenderPointLightTiledShadowMapPass);
	SafeDelete(m_pPointLightShadowMapTileAllocator);
	SafeDelete(m_pConvertPointLightTiledShadowMapPass);
	SafeDelete(m_pRenderSpotLightTiledShadowMapPass);
	SafeDelete(m_pSpotLightShadowMapTileAllocator);
	SafeDelete(m_pConvertSpotLightTiledShadowMapPass);
	SafeDelete(m_pCreateVoxelizeCommandsPass);
	SafeDelete(m_pVoxelizePass);
	SafeDelete(m_pTiledShadingPass);
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
	SafeDelete(m_pDepthTexture);
	SafeDelete(m_pCommandQueue);
	SafeDelete(m_pCommandListPool);
	SafeDelete(m_pSwapChain);
	SafeDelete(m_pDevice);
	SafeDelete(m_pBackBufferViewport);
}

void DXApplication::OnInit()
{
	InitRenderEnv(kBackBufferWidth, kBackBufferHeight);
	InitScene(kBackBufferWidth, kBackBufferHeight);
	
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

	InitCreateShadowMapCommandsPass();
	if (m_NumPointLights > 0)
	{
		InitRenderPointLightTiledShadowMapPass();
		InitConvertPointLightTiledShadowMapPass();
		InitVisualizePointLightTiledShadowMapPass();
	}
	if (m_NumSpotLights > 0)
	{
		InitRenderSpotLightTiledShadowMapPass();
		InitConvertSpotLightTiledShadowMapPass();
		InitVisualizeSpotLightTiledShadowMapPass();
	}
		
	InitCreateVoxelizeCommandsPass();
	InitVoxelizePass();

	InitTiledLightCullingPass();
	InitTiledShadingPass();

	InitVisualizeAccumLightPass();
	InitVisualizeDepthBufferPass();
	InitVisualizeReprojectedDepthBufferPass();
	InitVisualizeNormalBufferPass();
	InitVisualizeTexCoordBufferPass();
	InitVisualizeDepthBufferWithMeshTypePass();
	InitVisualizeVoxelReflectancePass();
}

void DXApplication::OnUpdate()
{
	static Matrix4f prevViewProjMatrix = m_pCamera->GetViewMatrix() * m_pCamera->GetProjMatrix();
	static Matrix4f prevViewProjInvMatrix = Inverse(prevViewProjMatrix);

	const Vector3f& cameraWorldSpacePos = m_pCamera->GetTransform().GetPosition();
	
	AppData* pAppData = (AppData*)m_UploadAppData[m_BackBufferIndex];
	pAppData->m_ViewMatrix = m_pCamera->GetViewMatrix();
	pAppData->m_ViewInvMatrix = Inverse(pAppData->m_ViewMatrix);
	pAppData->m_ProjMatrix = m_pCamera->GetProjMatrix();
	pAppData->m_ProjInvMatrix = Inverse(pAppData->m_ProjMatrix);
	pAppData->m_ViewProjMatrix = m_pCamera->GetViewMatrix() * m_pCamera->GetProjMatrix();
	pAppData->m_ViewProjInvMatrix = Inverse(pAppData->m_ViewProjMatrix);
	pAppData->m_PrevViewProjMatrix = prevViewProjMatrix;
	pAppData->m_PrevViewProjInvMatrix = prevViewProjInvMatrix;

	prevViewProjMatrix = pAppData->m_ViewProjMatrix;
	prevViewProjInvMatrix = pAppData->m_ViewProjInvMatrix;

	pAppData->m_CameraWorldSpacePos = Vector4f(cameraWorldSpacePos.m_X, cameraWorldSpacePos.m_Y, cameraWorldSpacePos.m_Z, 1.0f);
	pAppData->m_CameraNearPlane = m_pCamera->GetNearClipPlane();
	pAppData->m_CameraFarPlane = m_pCamera->GetFarClipPlane();

	const Frustum cameraWorldFrustum(pAppData->m_ViewProjMatrix);
	for (u8 planeIndex = 0; planeIndex < Frustum::NumPlanes; ++planeIndex)
		pAppData->m_CameraWorldFrustumPlanes[planeIndex] = cameraWorldFrustum.m_Planes[planeIndex];

	pAppData->m_ScreenSize = Vector2u(kBackBufferWidth, kBackBufferHeight);
	pAppData->m_RcpScreenSize = Vector2f(1.0f / f32(pAppData->m_ScreenSize.m_X), 1.0f / f32(pAppData->m_ScreenSize.m_Y));
	pAppData->m_ScreenHalfSize = Vector2u(pAppData->m_ScreenSize.m_X >> 1, pAppData->m_ScreenSize.m_Y >> 1);
	pAppData->m_RcpScreenHalfSize = Vector2f(1.0f / f32(pAppData->m_ScreenHalfSize.m_X), 1.0f / f32(pAppData->m_ScreenHalfSize.m_Y));
	pAppData->m_ScreenQuarterSize = Vector2u(pAppData->m_ScreenHalfSize.m_X >> 1, pAppData->m_ScreenHalfSize.m_Y >> 1);
	pAppData->m_RcpScreenQuarterSize = Vector2f(1.0f / f32(pAppData->m_ScreenQuarterSize.m_X), 1.0f / f32(pAppData->m_ScreenQuarterSize.m_Y));
	pAppData->m_SunWorldSpaceDir = Vector4f(0.0f, 0.0f, 1.0f, 0.0f);
	pAppData->m_SunLightColor = Color::WHITE;
	pAppData->m_ScreenTileSize = Vector2u(kTileSize, kTileSize);
	pAppData->m_NumScreenTiles = Vector2u(kNumTilesX, kNumTilesY);

#ifdef ENABLE_VOXELIZATION
	const AxisAlignedBox cameraWorldAABB(Frustum::NumCorners, cameraWorldFrustum.m_Corners);

	const Vector3f voxelGridWorldCenter = cameraWorldAABB.m_Center;
	const Vector3f voxelGridWorldRadius(m_pCamera->GetFarClipPlane());
	const Vector3f numVoxelsInGrid(kNumVoxelsInGridX, kNumVoxelsInGridY, kNumVoxelsInGridZ);
	const Vector3f voxelGridWorldSize = 2.0f * voxelGridWorldRadius;
	
	pAppData->m_VoxelGridWorldMinPoint = voxelGridWorldCenter - voxelGridWorldRadius;
	pAppData->m_VoxelGridWorldMaxPoint = voxelGridWorldCenter + voxelGridWorldRadius;
	pAppData->m_VoxelRcpSize = numVoxelsInGrid / voxelGridWorldSize;
	
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

	pAppData->m_VoxelGridViewProjMatrices[0] = voxelGridCameraAlongX.GetViewMatrix() * voxelGridCameraAlongX.GetProjMatrix();
	pAppData->m_VoxelGridViewProjMatrices[1] = voxelGridCameraAlongY.GetViewMatrix() * voxelGridCameraAlongY.GetProjMatrix();
	pAppData->m_VoxelGridViewProjMatrices[2] = voxelGridCameraAlongZ.GetViewMatrix() * voxelGridCameraAlongZ.GetProjMatrix();
#endif

	if (m_NumPointLights > 0)
		SetupPointLightDataForUpload(cameraWorldFrustum);

	if (m_NumSpotLights > 0)
		SetupSpotLightDataForUpload(cameraWorldFrustum);
}

void DXApplication::OnRender()
{
#ifdef ENABLE_PROFILING
	m_pCPUProfiler->StartFrame();
	m_pGPUProfiler->StartFrame(m_BackBufferIndex);
#endif // ENABLE_PROFILING

	static CommandList* commandListBatch[MAX_NUM_COMMAND_LISTS_IN_BATCH];

	u8 commandListBatchSize = 0;
	commandListBatch[commandListBatchSize++] = RecordDownscaleAndReprojectDepthPass();
	commandListBatch[commandListBatchSize++] = RecordPreRenderPass();
	commandListBatch[commandListBatchSize++] = RecordFrustumMeshCullingPass();
	commandListBatch[commandListBatchSize++] = RecordFillVisibilityBufferMainPass();
	commandListBatch[commandListBatchSize++] = RecordCreateMainDrawCommandsPass();
	commandListBatch[commandListBatchSize++] = RecordRenderGBufferMainPass();
	commandListBatch[commandListBatchSize++] = RecordFillVisibilityBufferFalseNegativePass();
	commandListBatch[commandListBatchSize++] = RecordCreateFalseNegativeDrawCommandsPass();
	commandListBatch[commandListBatchSize++] = RecordRenderGBufferFalseNegativePass();
	commandListBatch[commandListBatchSize++] = RecordCalcShadingRectanglesPass();
	commandListBatch[commandListBatchSize++] = RecordFillMeshTypeDepthBufferPass();
	commandListBatch[commandListBatchSize++] = RecordUploadLightDataPass();
	
	commandListBatch[commandListBatchSize++] = RecordCreateShadowMapCommandsPass();
	if (m_NumPointLights > 0)
	{
		commandListBatch[commandListBatchSize++] = RecordRenderPointLightTiledShadowMapPass();
		commandListBatch[commandListBatchSize++] = RecordConvertPointLightTiledShadowMapPass();
	}
	if (m_NumSpotLights > 0)
	{
		commandListBatch[commandListBatchSize++] = RecordRenderSpotLightTiledShadowMapPass();
		commandListBatch[commandListBatchSize++] = RecordConvertSpotLightTiledShadowMapPass();
	}
		
	commandListBatch[commandListBatchSize++] = RecordCreateVoxelizeCommandsPass();
	commandListBatch[commandListBatchSize++] = RecordVoxelizePass();
	commandListBatch[commandListBatchSize++] = RecordTiledLightCullingPass();
	commandListBatch[commandListBatchSize++] = RecordTiledShadingPass();
	commandListBatch[commandListBatchSize++] = RecordVisualizeDisplayResultPass();
	commandListBatch[commandListBatchSize++] = RecordPostRenderPass();
				
	++m_pRenderEnv->m_LastSubmissionFenceValue;
	m_pCommandQueue->ExecuteCommandLists(commandListBatchSize, commandListBatch, m_pFence, m_pRenderEnv->m_LastSubmissionFenceValue);
	ColorTexture* pRenderTarget = m_pSwapChain->GetBackBuffer(m_BackBufferIndex);

	++m_pRenderEnv->m_LastSubmissionFenceValue;
	m_pSwapChain->Present(1, 0);
	m_pCommandQueue->Signal(m_pFence, m_pRenderEnv->m_LastSubmissionFenceValue);

#ifdef ENABLE_PROFILING
	m_pCPUProfiler->EndFrame();
	m_pGPUProfiler->EndFrame(m_pCommandQueue);

	m_pCPUProfiler->OutputToConsole();
	m_pGPUProfiler->OutputToConsole();
#endif // #ifdef ENABLE_PROFILING

#ifdef DEBUG_RENDER_PASS
	m_pFence->WaitForSignalOnCPU(m_pRenderEnv->m_LastSubmissionFenceValue);
	OuputDebugRenderPassResult();
#endif // DEBUG_RENDER_PASS

	m_FrameCompletionFenceValues[m_BackBufferIndex] = m_pRenderEnv->m_LastSubmissionFenceValue;
	m_BackBufferIndex = m_pSwapChain->GetCurrentBackBufferIndex();
	m_pFence->WaitForSignalOnCPU(m_FrameCompletionFenceValues[m_BackBufferIndex]);
}

void DXApplication::OnDestroy()
{
	m_pCommandQueue->Signal(m_pFence, m_pRenderEnv->m_LastSubmissionFenceValue);
	m_pFence->WaitForSignalOnCPU(m_pRenderEnv->m_LastSubmissionFenceValue);
}

void DXApplication::OnKeyDown(UINT8 key)
{
	const f32 cameraMoveSpeed = 3.0f;
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
			if (m_NumPointLights > 0)
				UpdateDisplayResult(DisplayResult::PointLightTiledShadowMap);
			break;
		}
		case '8':
		{
			if (m_NumSpotLights > 0)
				UpdateDisplayResult(DisplayResult::SpotLightTiledShadowMap);
			break;
		}
		case '9':
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

#ifdef ENABLE_PROFILING
	m_pCPUProfiler = new CPUProfiler(25/*maxNumProfiles*/);
	m_pGPUProfiler = new GPUProfiler(m_pRenderEnv, 50/*maxNumProfiles*/, kNumBackBuffers);
#endif // ENABLE_PROFILING
	m_pRenderEnv->m_pGPUProfiler = m_pGPUProfiler;

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

	ResourceTransitionBarrier initResourceBarrier(m_pDepthTexture, D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	CommandList* pInitCommandList = m_pCommandListPool->Create(L"pInitResourcesCommandList");
	pInitCommandList->Begin();
	pInitCommandList->ClearDepthView(m_pDepthTexture->GetDSVHandle(), 1.0f);
	pInitCommandList->ResourceBarrier(1, &initResourceBarrier);
	pInitCommandList->End();

	++m_pRenderEnv->m_LastSubmissionFenceValue;
	m_pCommandQueue->ExecuteCommandLists(1, &pInitCommandList, m_pFence, m_pRenderEnv->m_LastSubmissionFenceValue);
	m_pFence->WaitForSignalOnCPU(m_pRenderEnv->m_LastSubmissionFenceValue);
}

void DXApplication::InitScene(UINT backBufferWidth, UINT backBufferHeight)
{
	Scene* pScene = SceneLoader::LoadSponza();

	assert(m_pCamera == nullptr);
	f32 aspectRatio = FLOAT(backBufferWidth) / FLOAT(backBufferHeight);
	m_pCamera = new Camera(Camera::ProjType_Perspective,
		pScene->GetCamera()->GetNearClipPlane(),
		pScene->GetCamera()->GetFarClipPlane(),
		aspectRatio);
	
	m_pCamera->GetTransform().SetPosition(pScene->GetCamera()->GetTransform().GetPosition());
	m_pCamera->GetTransform().SetRotation(pScene->GetCamera()->GetTransform().GetRotation());

	MemoryRange readRange(0, 0);
	ConstantBufferDesc appDataBufferDesc(sizeof(AppData));
	for (u8 index = 0; index < kNumBackBuffers; ++index)
	{
		m_UploadAppDataBuffers[index] = new Buffer(m_pRenderEnv, m_pRenderEnv->m_pUploadHeapProps,
			&appDataBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, L"m_pUploadAppDataBuffer");
		m_UploadAppData[index] = m_UploadAppDataBuffers[index]->Map(0, &readRange);
	}

	if (pScene->GetNumMeshBatches() > 0)
		m_pMeshRenderResources = new MeshRenderResources(m_pRenderEnv, pScene->GetNumMeshBatches(), pScene->GetMeshBatches());

	if (pScene->GetNumMaterials() > 0)
		m_pMaterialRenderResources = new MaterialRenderResources(m_pRenderEnv, pScene->GetNumMaterials(), pScene->GetMaterials());

	if (pScene->GetNumPointLights() > 0)
		InitPointLightRenderResources(pScene);

	if (pScene->GetNumSpotLights() > 0)
		InitSpotLightRenderResources(pScene);

	SafeDelete(pScene);
}

void DXApplication::InitDownscaleAndReprojectDepthPass()
{
	assert(m_pDownscaleAndReprojectDepthPass == nullptr);

	DownscaleAndReprojectDepthPass::InitParams params;
	params.m_pName = "DownscaleAndReprojectDepthPass";
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
	params.m_pAppDataBuffer = m_UploadAppDataBuffers[m_BackBufferIndex];

	m_pDownscaleAndReprojectDepthPass->Record(&params);
	return params.m_pCommandList;
}

CommandList* DXApplication::RecordPreRenderPass()
{
	const FLOAT clearColor[] = {0.0f, 0.0f, 0.0f, 0.0f};
		
	CommandList* pCommandList = m_pCommandListPool->Create(L"pClearResourcesCommandList");
	pCommandList->Begin();
#ifdef ENABLE_PROFILING
	u32 profileIndex = m_pGPUProfiler->StartProfile(pCommandList, "PreRenderPass");
#endif // ENABLE_PROFILING

	pCommandList->ClearRenderTargetView(m_pAccumLightTexture->GetRTVHandle(), clearColor);

#ifdef ENABLE_PROFILING
	m_pGPUProfiler->EndProfile(pCommandList, profileIndex);
#endif // ENABLE_PROFILING
	pCommandList->End();
	
	return pCommandList;
}

void DXApplication::InitFrustumMeshCullingPass()
{
	assert(m_pMeshRenderResources != nullptr);
	assert(m_pFrustumMeshCullingPass == nullptr);

	FrustumMeshCullingPass::InitParams params;
	params.m_pName = "FrustumMeshCullingPass";
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
	params.m_pAppDataBuffer = m_UploadAppDataBuffers[m_BackBufferIndex];

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
	params.m_pName = "FillVisibilityBufferMainPass";
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
	params.m_pAppDataBuffer = m_UploadAppDataBuffers[m_BackBufferIndex];
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
	params.m_pName = "CreateMainDrawCommandsPass";
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
	params.m_pName = "RenderGBufferMainPass";
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
	params.m_pAppDataBuffer = m_UploadAppDataBuffers[m_BackBufferIndex];
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
	params.m_pName = "FillVisibilityBufferFalseNegativePass";
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
	params.m_pAppDataBuffer = m_UploadAppDataBuffers[m_BackBufferIndex];
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
	params.m_pName = "CreateFalseNegativeDrawCommandsPass";
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
	params.m_pName = "RenderGBufferFalseNegativePass";
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
	params.m_pAppDataBuffer = m_UploadAppDataBuffers[m_BackBufferIndex];
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
	params.m_pName = "CalcShadingRectanglesPass";
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
	params.m_pAppDataBuffer = m_UploadAppDataBuffers[m_BackBufferIndex];

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
	params.m_pName = "FillMeshTypeDepthBufferPass";
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
		params.m_pName = "VisualizeDepthBufferPass";
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
	params.m_pAppDataBuffer = m_UploadAppDataBuffers[m_BackBufferIndex];
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
		params.m_pName = "VisualizeReprojectedDepthBufferPass";
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
	params.m_pAppDataBuffer = m_UploadAppDataBuffers[m_BackBufferIndex];
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
		params.m_pName = "VisualizeNormalBufferPass";
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
	params.m_pAppDataBuffer = m_UploadAppDataBuffers[m_BackBufferIndex];
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
		params.m_pName = "VisualizeTexCoordBufferPass";
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
	params.m_pAppDataBuffer = m_UploadAppDataBuffers[m_BackBufferIndex];
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
		params.m_pName = "VisualizeDepthBufferWithMeshTypePass";
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
	params.m_pAppDataBuffer = m_UploadAppDataBuffers[m_BackBufferIndex];
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
		assert(m_VisualizeAccumLightPasses[index] == nullptr);

		VisualizeTexturePass::InitParams params;
		params.m_pName = "VisualizeAccumLightPass";
		params.m_pRenderEnv = m_pRenderEnv;
		params.m_InputResourceStates.m_InputTextureState = pTiledShadingPassStates->m_AccumLightTextureState;
		params.m_InputResourceStates.m_BackBufferState = D3D12_RESOURCE_STATE_PRESENT;
		params.m_pInputTexture = m_pAccumLightTexture;
		params.m_InputTextureSRV = m_pAccumLightTexture->GetSRVHandle();
		params.m_pBackBuffer = m_pSwapChain->GetBackBuffer(index);
		params.m_TextureType = VisualizeTexturePass::TextureType_Other;

		m_VisualizeAccumLightPasses[index] = new VisualizeTexturePass(&params);
	}
}

CommandList* DXApplication::RecordVisualizeAccumLightPass()
{
	VisualizeTexturePass* pVisualizeTexturePass = m_VisualizeAccumLightPasses[m_BackBufferIndex];
	assert(pVisualizeTexturePass != nullptr);

	VisualizeTexturePass::RenderParams params;
	params.m_pRenderEnv = m_pRenderEnv;
	params.m_pCommandList = m_pCommandListPool->Create(L"pVisualizeAccumLightCommandList");
	params.m_pAppDataBuffer = m_UploadAppDataBuffers[m_BackBufferIndex];
	params.m_pViewport = m_pBackBufferViewport;

	pVisualizeTexturePass->Record(&params);
	return params.m_pCommandList;
}

void DXApplication::InitVisualizePointLightTiledShadowMapPass()
{
	assert(m_pRenderPointLightTiledShadowMapPass != nullptr);
	
	const RenderTiledShadowMapPass::ResourceStates* pRenderTiledShadowMapPassStates =
		m_pRenderPointLightTiledShadowMapPass->GetOutputResourceStates();
	
	DepthTexture* pTiledShadowMap = m_pRenderPointLightTiledShadowMapPass->GetTiledShadowMap();
	for (u8 index = 0; index < kNumBackBuffers; ++index)
	{
		assert(m_VisualizePointLightTiledShadowMapPasses[index] == nullptr);

		VisualizeTexturePass::InitParams params;
		params.m_pName = "VisualizePointLightTiledShadowMapPass";
		params.m_pRenderEnv = m_pRenderEnv;
		params.m_InputResourceStates.m_InputTextureState = pRenderTiledShadowMapPassStates->m_TiledShadowMapState;
		params.m_InputResourceStates.m_BackBufferState = D3D12_RESOURCE_STATE_PRESENT;
		params.m_pInputTexture = pTiledShadowMap;
		params.m_InputTextureSRV = pTiledShadowMap->GetSRVHandle();
		params.m_pBackBuffer = m_pSwapChain->GetBackBuffer(index);
		params.m_TextureType = VisualizeTexturePass::TextureType_Depth;

		m_VisualizePointLightTiledShadowMapPasses[index] = new VisualizeTexturePass(&params);
	}
}

CommandList* DXApplication::RecordVisualizePointLightTiledShadowMapPass()
{
	VisualizeTexturePass* pVisualizeTexturePass = m_VisualizePointLightTiledShadowMapPasses[m_BackBufferIndex];
	assert(pVisualizeTexturePass != nullptr);

	VisualizeTexturePass::RenderParams params;
	params.m_pRenderEnv = m_pRenderEnv;
	params.m_pCommandList = m_pCommandListPool->Create(L"pVisualizePointLightTiledShadowMapCommandList");
	params.m_pAppDataBuffer = m_UploadAppDataBuffers[m_BackBufferIndex];
	params.m_pViewport = m_pBackBufferViewport;

	pVisualizeTexturePass->Record(&params);
	return params.m_pCommandList;
}

void DXApplication::InitVisualizeSpotLightTiledShadowMapPass()
{
	assert(m_pRenderSpotLightTiledShadowMapPass != nullptr);

	const RenderTiledShadowMapPass::ResourceStates* pRenderTiledShadowMapPassStates =
		m_pRenderSpotLightTiledShadowMapPass->GetOutputResourceStates();

	DepthTexture* pTiledShadowMap = m_pRenderSpotLightTiledShadowMapPass->GetTiledShadowMap();
	for (u8 index = 0; index < kNumBackBuffers; ++index)
	{
		assert(m_VisualizeSpotLightTiledShadowMapPasses[index] == nullptr);

		VisualizeTexturePass::InitParams params;
		params.m_pName = "VisualizeSpotLightTiledShadowMapPass";
		params.m_pRenderEnv = m_pRenderEnv;
		params.m_InputResourceStates.m_InputTextureState = pRenderTiledShadowMapPassStates->m_TiledShadowMapState;
		params.m_InputResourceStates.m_BackBufferState = D3D12_RESOURCE_STATE_PRESENT;
		params.m_pInputTexture = pTiledShadowMap;
		params.m_InputTextureSRV = pTiledShadowMap->GetSRVHandle();
		params.m_pBackBuffer = m_pSwapChain->GetBackBuffer(index);
		params.m_TextureType = VisualizeTexturePass::TextureType_Depth;

		m_VisualizeSpotLightTiledShadowMapPasses[index] = new VisualizeTexturePass(&params);
	}
}

CommandList* DXApplication::RecordVisualizeSpotLightTiledShadowMapPass()
{
	VisualizeTexturePass* pVisualizeTexturePass = m_VisualizeSpotLightTiledShadowMapPasses[m_BackBufferIndex];
	assert(pVisualizeTexturePass != nullptr);

	VisualizeTexturePass::RenderParams params;
	params.m_pRenderEnv = m_pRenderEnv;
	params.m_pCommandList = m_pCommandListPool->Create(L"pVisualizeSpotLightTiledShadowMapCommandList");
	params.m_pAppDataBuffer = m_UploadAppDataBuffers[m_BackBufferIndex];
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
		params.m_pName = "VisualizeVoxelReflectancePass";
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
	params.m_pAppDataBuffer = m_UploadAppDataBuffers[m_BackBufferIndex];
	params.m_pViewport = m_pBackBufferViewport;

	pVisualizeVoxelReflectancePass->Record(&params);
	return params.m_pCommandList;
}

CommandList* DXApplication::RecordUploadLightDataPass()
{
	assert(m_pTiledShadingPass != nullptr);

	const TiledShadingPass::ResourceStates* pTiledShadingPassStates =
		m_pTiledShadingPass->GetOutputResourceStates();
	
	CommandList* pCommandList = m_pCommandListPool->Create(L"pUploadVisibleLightDataCommandList");
	pCommandList->Begin();
#ifdef ENABLE_PROFILING
	u32 profileIndex = m_pGPUProfiler->StartProfile(pCommandList, "UploadLightDataPass");
#endif // ENABLE_PROFILING

	static ResourceTransitionBarrier resourceBarriers[12];
	u8 numBarriers = 0;

	if (m_NumPointLights > 0)
	{
		assert(m_pRenderPointLightTiledShadowMapPass != nullptr);
		const RenderTiledShadowMapPass::ResourceStates* pRenderTiledShadowMapPassStates =
			m_pRenderPointLightTiledShadowMapPass->GetOutputResourceStates();

		assert(m_pConvertPointLightTiledShadowMapPass != nullptr);
		const ConvertTiledShadowMapPass::ResourceStates* pConvertShadowMapPassStates =
			m_pConvertPointLightTiledShadowMapPass->GetOutputResourceStates();

		resourceBarriers[numBarriers++] = ResourceTransitionBarrier(
			m_pActivePointLightWorldBoundsBuffer,
			pTiledShadingPassStates->m_PointLightWorldBoundsBufferState,
			D3D12_RESOURCE_STATE_COPY_DEST);

		resourceBarriers[numBarriers++] = ResourceTransitionBarrier(
			m_pActivePointLightPropsBuffer,
			pTiledShadingPassStates->m_PointLightPropsBufferState,
			D3D12_RESOURCE_STATE_COPY_DEST);

		resourceBarriers[numBarriers++] = ResourceTransitionBarrier(
			m_pActivePointLightConvertShadowMapParamsBuffer,
			pConvertShadowMapPassStates->m_ConvertShadowMapParamsBufferState,
			D3D12_RESOURCE_STATE_COPY_DEST);

		resourceBarriers[numBarriers++] = ResourceTransitionBarrier(
			m_pActivePointLightWorldFrustumBuffer,
			pRenderTiledShadowMapPassStates->m_LightWorldFrustumBufferState,
			D3D12_RESOURCE_STATE_COPY_DEST);

		resourceBarriers[numBarriers++] = ResourceTransitionBarrier(
			m_pActivePointLightViewProjMatrixBuffer,
			pRenderTiledShadowMapPassStates->m_LightViewProjMatrixBufferState,
			D3D12_RESOURCE_STATE_COPY_DEST);

		resourceBarriers[numBarriers++] = ResourceTransitionBarrier(
			m_pActivePointLightShadowMapTileBuffer,
			pConvertShadowMapPassStates->m_ShadowMapTileBufferState,
			D3D12_RESOURCE_STATE_COPY_DEST);
	}
	if (m_NumSpotLights > 0)
	{
		assert(m_pRenderSpotLightTiledShadowMapPass != nullptr);
		const RenderTiledShadowMapPass::ResourceStates* pRenderTiledShadowMapPassStates =
			m_pRenderSpotLightTiledShadowMapPass->GetOutputResourceStates();

		assert(m_pConvertSpotLightTiledShadowMapPass != nullptr);
		const ConvertTiledShadowMapPass::ResourceStates* pConvertShadowMapPassStates =
			m_pConvertSpotLightTiledShadowMapPass->GetOutputResourceStates();

		resourceBarriers[numBarriers++] = ResourceTransitionBarrier(
			m_pActiveSpotLightWorldBoundsBuffer,
			pTiledShadingPassStates->m_SpotLightWorldBoundsBufferState,
			D3D12_RESOURCE_STATE_COPY_DEST);

		resourceBarriers[numBarriers++] = ResourceTransitionBarrier(
			m_pActiveSpotLightPropsBuffer,
			pTiledShadingPassStates->m_SpotLightPropsBufferState,
			D3D12_RESOURCE_STATE_COPY_DEST);

		resourceBarriers[numBarriers++] = ResourceTransitionBarrier(
			m_pActiveSpotLightConvertShadowMapParamsBuffer,
			pConvertShadowMapPassStates->m_ConvertShadowMapParamsBufferState,
			D3D12_RESOURCE_STATE_COPY_DEST);

		resourceBarriers[numBarriers++] = ResourceTransitionBarrier(
			m_pActiveSpotLightWorldFrustumBuffer,
			pRenderTiledShadowMapPassStates->m_LightWorldFrustumBufferState,
			D3D12_RESOURCE_STATE_COPY_DEST);

		resourceBarriers[numBarriers++] = ResourceTransitionBarrier(
			m_pActiveSpotLightViewProjMatrixBuffer,
			pRenderTiledShadowMapPassStates->m_LightViewProjMatrixBufferState,
			D3D12_RESOURCE_STATE_COPY_DEST);

		resourceBarriers[numBarriers++] = ResourceTransitionBarrier(
			m_pActiveSpotLightShadowMapTileBuffer,
			pTiledShadingPassStates->m_SpotLightShadowMapTileBufferState,
			D3D12_RESOURCE_STATE_COPY_DEST);
	}
	pCommandList->ResourceBarrier(numBarriers, resourceBarriers);
	
	if (m_NumPointLights > 0)
	{
		pCommandList->CopyBufferRegion(m_pActivePointLightWorldBoundsBuffer, 0,
			m_UploadActivePointLightWorldBoundsBuffers[m_BackBufferIndex], 0,
			m_NumActivePointLights * sizeof(Sphere));

		pCommandList->CopyBufferRegion(m_pActivePointLightPropsBuffer, 0,
			m_UploadActivePointLightPropsBuffers[m_BackBufferIndex], 0,
			m_NumActivePointLights * sizeof(PointLightProps));

		pCommandList->CopyBufferRegion(m_pActivePointLightConvertShadowMapParamsBuffer, 0,
			m_UploadActivePointLightConvertShadowMapParamsBuffers[m_BackBufferIndex], 0,
			m_NumActivePointLights * sizeof(ConvertShadowMapParams));

		pCommandList->CopyBufferRegion(m_pActivePointLightWorldFrustumBuffer, 0,
			m_UploadActivePointLightWorldFrustumBuffers[m_BackBufferIndex], 0,
			kNumCubeMapFaces * m_NumActivePointLights * sizeof(LightFrustum));

		pCommandList->CopyBufferRegion(m_pActivePointLightViewProjMatrixBuffer, 0,
			m_UploadActivePointLightViewProjMatrixBuffers[m_BackBufferIndex], 0,
			kNumCubeMapFaces * m_NumActivePointLights * sizeof(Matrix4f));

		pCommandList->CopyBufferRegion(m_pActivePointLightShadowMapTileBuffer, 0,
			m_UploadActivePointLightShadowMapTileBuffers[m_BackBufferIndex], 0,
			kNumCubeMapFaces * m_NumActivePointLights * sizeof(ShadowMapTile));
	}
	if (m_NumSpotLights > 0)
	{
		pCommandList->CopyBufferRegion(m_pActiveSpotLightWorldBoundsBuffer, 0,
			m_UploadActiveSpotLightWorldBoundsBuffers[m_BackBufferIndex], 0,
			m_NumActiveSpotLights * sizeof(Sphere));

		pCommandList->CopyBufferRegion(m_pActiveSpotLightPropsBuffer, 0,
			m_UploadActiveSpotLightPropsBuffers[m_BackBufferIndex], 0,
			m_NumActiveSpotLights * sizeof(SpotLightProps));

		pCommandList->CopyBufferRegion(m_pActiveSpotLightConvertShadowMapParamsBuffer, 0,
			m_UploadActiveSpotLightConvertShadowMapParamsBuffers[m_BackBufferIndex], 0,
			m_NumActiveSpotLights * sizeof(ConvertShadowMapParams));

		pCommandList->CopyBufferRegion(m_pActiveSpotLightWorldFrustumBuffer, 0,
			m_UploadActiveSpotLightWorldFrustumBuffers[m_BackBufferIndex], 0,
			m_NumActiveSpotLights * sizeof(LightFrustum));

		pCommandList->CopyBufferRegion(m_pActiveSpotLightViewProjMatrixBuffer, 0,
			m_UploadActiveSpotLightViewProjMatrixBuffers[m_BackBufferIndex], 0,
			m_NumActiveSpotLights * sizeof(Matrix4f));

		pCommandList->CopyBufferRegion(m_pActiveSpotLightShadowMapTileBuffer, 0,
			m_UploadActiveSpotLightShadowMapTileBuffers[m_BackBufferIndex], 0,
			m_NumActiveSpotLights * sizeof(ShadowMapTile));
	}

#ifdef ENABLE_PROFILING
	m_pGPUProfiler->EndProfile(pCommandList, profileIndex);
#endif // ENABLE_PROFILING
	pCommandList->End();

	return pCommandList;
}

void DXApplication::InitTiledLightCullingPass()
{
	assert(m_pTiledLightCullingPass == nullptr);
	assert(m_pRenderGBufferFalseNegativePass != nullptr);
	assert(m_pVoxelizePass != nullptr);

	const RenderGBufferPass::ResourceStates* pRenderGBufferFalseNegativePassStates =
		m_pRenderGBufferFalseNegativePass->GetOutputResourceStates();

	const VoxelizePass::ResourceStates* pVoxelizePassStates =
		m_pVoxelizePass->GetOutputResourceStates();

	TiledLightCullingPass::InitParams params;
	params.m_pName = "TiledLightCullingPass";
	params.m_pRenderEnv = m_pRenderEnv;
	params.m_pDepthTexture = m_pDepthTexture;
	
	params.m_InputResourceStates.m_DepthTextureState = pRenderGBufferFalseNegativePassStates->m_DepthTextureState;
	params.m_InputResourceStates.m_PointLightWorldBoundsBufferState = pVoxelizePassStates->m_PointLightWorldBoundsBufferState;
	params.m_InputResourceStates.m_PointLightIndexPerTileBufferState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	params.m_InputResourceStates.m_PointLightRangePerTileBufferState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	params.m_InputResourceStates.m_SpotLightWorldBoundsBufferState = pVoxelizePassStates->m_SpotLightWorldBoundsBufferState;
	params.m_InputResourceStates.m_SpotLightIndexPerTileBufferState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	params.m_InputResourceStates.m_SpotLightRangePerTileBufferState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;

	params.m_MaxNumPointLights = m_NumPointLights;
	params.m_pPointLightWorldBoundsBuffer = m_pActivePointLightWorldBoundsBuffer;
	params.m_MaxNumSpotLights = m_NumSpotLights;
	params.m_pSpotLightWorldBoundsBuffer = m_pActiveSpotLightWorldBoundsBuffer;
	
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
	params.m_pAppDataBuffer = m_UploadAppDataBuffers[m_BackBufferIndex];
	params.m_NumPointLights = m_NumActivePointLights;
	params.m_NumSpotLights = m_NumActiveSpotLights;
		
	m_pTiledLightCullingPass->Record(&params);
	return params.m_pCommandList;
}

void DXApplication::InitCreateShadowMapCommandsPass()
{
	assert(m_pCreateShadowMapCommandsPass == nullptr);
	assert(m_pCreateMainDrawCommandsPass != nullptr);
	assert(m_pCreateFalseNegativeDrawCommandsPass != nullptr);
	assert(m_pFrustumMeshCullingPass != nullptr);
	assert(m_pMeshRenderResources != nullptr);

	const FrustumMeshCullingPass::ResourceStates* pFrustumMeshCullingPassStates =
		m_pFrustumMeshCullingPass->GetOutputResourceStates();

	const CreateMainDrawCommandsPass::ResourceStates* pCreateMainDrawCommandsPassStates =
		m_pCreateMainDrawCommandsPass->GetOutputResourceStates();

	const CreateFalseNegativeDrawCommandsPass::ResourceStates* pCreateFalseNegativeDrawCommandsPassStates =
		m_pCreateFalseNegativeDrawCommandsPass->GetOutputResourceStates();

	CreateShadowMapCommandsPass::InitParams params;
	params.m_pName = "CreateShadowMapCommandsPass";
	params.m_pRenderEnv = m_pRenderEnv;
	
	params.m_InputResourceStates.m_NumMeshesBufferState = pCreateFalseNegativeDrawCommandsPassStates->m_NumMeshesBufferState;
	params.m_InputResourceStates.m_MeshInfoBufferState = pCreateFalseNegativeDrawCommandsPassStates->m_MeshInfoBufferState;
	params.m_InputResourceStates.m_MeshInstanceWorldAABBBufferState = pFrustumMeshCullingPassStates->m_InstanceWorldAABBBufferState;
	params.m_InputResourceStates.m_MeshInstanceIndexBufferState = pCreateMainDrawCommandsPassStates->m_InstanceIndexBufferState;
	params.m_InputResourceStates.m_PointLightWorldBoundsBufferState = D3D12_RESOURCE_STATE_COPY_DEST;
	params.m_InputResourceStates.m_NumPointLightMeshInstancesBufferState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
	params.m_InputResourceStates.m_PointLightIndexForMeshInstanceBufferState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
	params.m_InputResourceStates.m_MeshInstanceIndexForPointLightBufferState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
	params.m_InputResourceStates.m_NumPointLightCommandsBufferState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
	params.m_InputResourceStates.m_PointLightCommandBufferState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
	params.m_InputResourceStates.m_SpotLightWorldBoundsBufferState = D3D12_RESOURCE_STATE_COPY_DEST;
	params.m_InputResourceStates.m_NumSpotLightMeshInstancesBufferState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
	params.m_InputResourceStates.m_SpotLightIndexForMeshInstanceBufferState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
	params.m_InputResourceStates.m_MeshInstanceIndexForSpotLightBufferState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
	params.m_InputResourceStates.m_NumSpotLightCommandsBufferState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
	params.m_InputResourceStates.m_SpotLightCommandBufferState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;

	params.m_MaxNumMeshes = m_pMeshRenderResources->GetTotalNumMeshes();
	params.m_MaxNumInstances = m_pMeshRenderResources->GetTotalNumInstances();
	params.m_MaxNumInstancesPerMesh = m_pMeshRenderResources->GetMaxNumInstancesPerMesh();
	params.m_pNumMeshesBuffer = m_pFrustumMeshCullingPass->GetNumVisibleMeshesBuffer();
	params.m_pMeshInfoBuffer = m_pFrustumMeshCullingPass->GetVisibleMeshInfoBuffer();
	params.m_pMeshInstanceIndexBuffer = m_pFrustumMeshCullingPass->GetVisibleInstanceIndexBuffer();
	params.m_pMeshInstanceWorldAABBBuffer = m_pMeshRenderResources->GetInstanceWorldAABBBuffer();
		
	params.m_MaxNumPointLights = m_NumPointLights;
	params.m_pPointLightWorldBoundsBuffer = m_pActivePointLightWorldBoundsBuffer;
	params.m_MaxNumSpotLights = m_NumSpotLights;
	params.m_pSpotLightWorldBoundsBuffer = m_pActiveSpotLightWorldBoundsBuffer;

	m_pCreateShadowMapCommandsPass = new CreateShadowMapCommandsPass(&params);
}

CommandList* DXApplication::RecordCreateShadowMapCommandsPass()
{
	assert(m_pCreateShadowMapCommandsPass != nullptr);

	CreateShadowMapCommandsPass::RenderParams params;
	params.m_pRenderEnv = m_pRenderEnv;
	params.m_pCommandList = m_pCommandListPool->Create(L"pCreateShadowMapCommandsCommandList");;
	params.m_pNumMeshesBuffer = m_pFrustumMeshCullingPass->GetNumVisibleMeshesBuffer();
	params.m_NumPointLights = m_NumActivePointLights;
	params.m_NumSpotLights = m_NumActiveSpotLights;

	m_pCreateShadowMapCommandsPass->Record(&params);
	return params.m_pCommandList;
}

void DXApplication::InitRenderPointLightTiledShadowMapPass()
{
	assert(m_pPointLightShadowMapTileAllocator == nullptr);
	m_pPointLightShadowMapTileAllocator = new ShadowMapTileAllocator(kPointLightShadowMapSize, kNumPointLightShadowMapLevels);

	assert(m_pRenderPointLightTiledShadowMapPass == nullptr);
	assert(m_pCreateShadowMapCommandsPass != nullptr);
	assert(m_pRenderGBufferFalseNegativePass != nullptr);
	
	const RenderGBufferPass::ResourceStates* pRenderGBufferFalseNegativePassStates =
		m_pRenderGBufferFalseNegativePass->GetOutputResourceStates();

	const CreateShadowMapCommandsPass::ResourceStates* pCreateShadowMapCommandsPassStates =
		m_pCreateShadowMapCommandsPass->GetOutputResourceStates();

	RenderTiledShadowMapPass::InitParams params;
	params.m_pName = "RenderPointLightTiledShadowMapPass";
	params.m_pRenderEnv = m_pRenderEnv;
	params.m_LightType = LightType_Point;
	params.m_ShadowMapSize = kPointLightShadowMapSize;
	params.m_pMeshRenderResources = m_pMeshRenderResources;
	params.m_pMeshInstanceWorldMatrixBuffer = m_pMeshRenderResources->GetInstanceWorldMatrixBuffer();
	params.m_pLightWorldBoundsOrPropsBuffer = m_pActivePointLightWorldBoundsBuffer;
	params.m_pLightWorldFrustumBuffer = m_pActivePointLightWorldFrustumBuffer;
	params.m_pLightViewProjMatrixBuffer = m_pActivePointLightViewProjMatrixBuffer;
	params.m_pLightIndexForMeshInstanceBuffer = m_pCreateShadowMapCommandsPass->GetPointLightIndexForMeshInstanceBuffer();
	params.m_pMeshInstanceIndexForLightBuffer = m_pCreateShadowMapCommandsPass->GetMeshInstanceIndexForPointLightBuffer();
	params.m_pNumShadowMapCommandsBuffer = m_pCreateShadowMapCommandsPass->GetNumPointLightCommandsBuffer();
	params.m_pShadowMapCommandBuffer = m_pCreateShadowMapCommandsPass->GetPointLightCommandBuffer();
			
	params.m_InputResourceStates.m_TiledShadowMapState = D3D12_RESOURCE_STATE_DEPTH_WRITE;
	params.m_InputResourceStates.m_MeshInstanceWorldMatrixBufferState = pRenderGBufferFalseNegativePassStates->m_InstanceWorldMatrixBufferState;
	params.m_InputResourceStates.m_LightWorldBoundsOrPropsBufferState = pCreateShadowMapCommandsPassStates->m_PointLightWorldBoundsBufferState;
	params.m_InputResourceStates.m_LightWorldFrustumBufferState = D3D12_RESOURCE_STATE_COPY_DEST;
	params.m_InputResourceStates.m_LightViewProjMatrixBufferState = D3D12_RESOURCE_STATE_COPY_DEST;
	params.m_InputResourceStates.m_LightIndexForMeshInstanceBufferState = pCreateShadowMapCommandsPassStates->m_PointLightIndexForMeshInstanceBufferState;
	params.m_InputResourceStates.m_MeshInstanceIndexForLightBufferState = pCreateShadowMapCommandsPassStates->m_MeshInstanceIndexForPointLightBufferState;
	params.m_InputResourceStates.m_NumShadowMapCommandsBufferState = pCreateShadowMapCommandsPassStates->m_NumPointLightCommandsBufferState;
	params.m_InputResourceStates.m_ShadowMapCommandBufferState = pCreateShadowMapCommandsPassStates->m_PointLightCommandBufferState;
		
	m_pRenderPointLightTiledShadowMapPass = new RenderTiledShadowMapPass(&params);
}

CommandList* DXApplication::RecordRenderPointLightTiledShadowMapPass()
{
	assert(m_pRenderPointLightTiledShadowMapPass != nullptr);
	RenderTiledShadowMapPass::RenderParams params;
	params.m_pRenderEnv = m_pRenderEnv;
	params.m_pCommandList = m_pCommandListPool->Create(L"pRenderPointLightTiledShadowMapCommandList");
	params.m_pMeshRenderResources = m_pMeshRenderResources;
	params.m_pNumShadowMapCommandsBuffer = m_pCreateShadowMapCommandsPass->GetNumPointLightCommandsBuffer();
	params.m_pShadowMapCommandBuffer = m_pCreateShadowMapCommandsPass->GetPointLightCommandBuffer();

	m_pRenderPointLightTiledShadowMapPass->Record(&params);
	return params.m_pCommandList;
}

void DXApplication::InitConvertPointLightTiledShadowMapPass()
{
	assert(m_pConvertPointLightTiledShadowMapPass == nullptr);
	assert(m_pRenderPointLightTiledShadowMapPass != nullptr);

	ConvertTiledShadowMapPass::InitParams params;
	params.m_pName = "ConvertPointLightTiledShadowMapPass";
	params.m_pRenderEnv = m_pRenderEnv;
	
	assert(false);
	//params.m_InputResourceStates.m_TiledShadowMapState;
	//params.m_InputResourceStates.m_ShadowMapTileBufferState;
	//params.m_InputResourceStates.m_ConvertShadowMapParamsBufferState;
	//params.m_InputResourceStates.m_TiledVarianceShadowMapState;
	
	params.m_LightType = LightType_Point;
	params.m_pTiledShadowMap = m_pRenderPointLightTiledShadowMapPass->GetTiledShadowMap();
	
	assert(false);
	//params.m_pShadowMapTileBuffer;
	//params.m_pConvertShadowMapParamsBuffer;

	m_pConvertPointLightTiledShadowMapPass = new ConvertTiledShadowMapPass(&params);
}

CommandList* DXApplication::RecordConvertPointLightTiledShadowMapPass()
{
	assert(m_pConvertPointLightTiledShadowMapPass != nullptr);
	assert(m_NumPointLights > 0);

	ConvertTiledShadowMapPass::RenderParams params;
	params.m_pRenderEnv = m_pRenderEnv;
	params.m_pCommandList = m_pCommandListPool->Create(L"pConvertPointLightTiledShadowMapCommandList");;
	params.m_NumShadowMapTiles = kNumCubeMapFaces * m_NumActivePointLights;

	m_pConvertPointLightTiledShadowMapPass->Record(&params);
	return params.m_pCommandList;
}

void DXApplication::InitRenderSpotLightTiledShadowMapPass()
{
	assert(m_NumSpotLights > 0);
	
	assert(m_pSpotLightShadowMapTileAllocator == nullptr);
	m_pSpotLightShadowMapTileAllocator = new ShadowMapTileAllocator(kSpotLightShadowMapSize, kNumSpotLightShadowMapLevels);

	assert(m_pRenderSpotLightTiledShadowMapPass == nullptr);
	assert(m_pCreateShadowMapCommandsPass != nullptr);
	
	const CreateShadowMapCommandsPass::ResourceStates* pCreateShadowMapCommandsPassStates =
		m_pCreateShadowMapCommandsPass->GetOutputResourceStates();

	RenderTiledShadowMapPass::InitParams params;
	params.m_pName = "RenderSpotLightTiledShadowMapPass";
	params.m_pRenderEnv = m_pRenderEnv;
	params.m_LightType = LightType_Spot;
	params.m_ShadowMapSize = kSpotLightShadowMapSize;
	params.m_pMeshRenderResources = m_pMeshRenderResources;
	params.m_pMeshInstanceWorldMatrixBuffer = m_pMeshRenderResources->GetInstanceWorldMatrixBuffer();
	params.m_pLightWorldBoundsOrPropsBuffer = m_pActiveSpotLightPropsBuffer;
	params.m_pLightWorldFrustumBuffer = m_pActiveSpotLightWorldFrustumBuffer;
	params.m_pLightViewProjMatrixBuffer = m_pActiveSpotLightViewProjMatrixBuffer;
	params.m_pLightIndexForMeshInstanceBuffer = m_pCreateShadowMapCommandsPass->GetSpotLightIndexForMeshInstanceBuffer();
	params.m_pMeshInstanceIndexForLightBuffer = m_pCreateShadowMapCommandsPass->GetMeshInstanceIndexForSpotLightBuffer();
	params.m_pNumShadowMapCommandsBuffer = m_pCreateShadowMapCommandsPass->GetNumSpotLightCommandsBuffer();
	params.m_pShadowMapCommandBuffer = m_pCreateShadowMapCommandsPass->GetSpotLightCommandBuffer();

	params.m_InputResourceStates.m_TiledShadowMapState = D3D12_RESOURCE_STATE_DEPTH_WRITE;

	if (m_NumPointLights > 0)
	{
		assert(m_pRenderPointLightTiledShadowMapPass != nullptr);
		const RenderTiledShadowMapPass::ResourceStates* pRenderPointLightTiledShadowMapStates =
			m_pRenderPointLightTiledShadowMapPass->GetOutputResourceStates();

		params.m_InputResourceStates.m_MeshInstanceWorldMatrixBufferState = pRenderPointLightTiledShadowMapStates->m_MeshInstanceWorldMatrixBufferState;
	}
	else
	{
		assert(m_pRenderGBufferFalseNegativePass != nullptr);
		const RenderGBufferPass::ResourceStates* pRenderGBufferFalseNegativePassStates =
			m_pRenderGBufferFalseNegativePass->GetOutputResourceStates();

		params.m_InputResourceStates.m_MeshInstanceWorldMatrixBufferState = pRenderGBufferFalseNegativePassStates->m_InstanceWorldMatrixBufferState;
	}

	params.m_InputResourceStates.m_LightWorldBoundsOrPropsBufferState = D3D12_RESOURCE_STATE_COPY_DEST;
	params.m_InputResourceStates.m_LightWorldFrustumBufferState = D3D12_RESOURCE_STATE_COPY_DEST;
	params.m_InputResourceStates.m_LightViewProjMatrixBufferState = D3D12_RESOURCE_STATE_COPY_DEST;
	params.m_InputResourceStates.m_LightIndexForMeshInstanceBufferState = pCreateShadowMapCommandsPassStates->m_SpotLightIndexForMeshInstanceBufferState;
	params.m_InputResourceStates.m_MeshInstanceIndexForLightBufferState = pCreateShadowMapCommandsPassStates->m_MeshInstanceIndexForSpotLightBufferState;
	params.m_InputResourceStates.m_NumShadowMapCommandsBufferState = pCreateShadowMapCommandsPassStates->m_NumSpotLightCommandsBufferState;
	params.m_InputResourceStates.m_ShadowMapCommandBufferState = pCreateShadowMapCommandsPassStates->m_SpotLightCommandBufferState;

	m_pRenderSpotLightTiledShadowMapPass = new RenderTiledShadowMapPass(&params);
}

CommandList* DXApplication::RecordRenderSpotLightTiledShadowMapPass()
{
	assert(m_pRenderSpotLightTiledShadowMapPass != nullptr);
	RenderTiledShadowMapPass::RenderParams params;
	params.m_pRenderEnv = m_pRenderEnv;
	params.m_pCommandList = m_pCommandListPool->Create(L"pRenderSpotLightTiledShadowMapCommandList");
	params.m_pMeshRenderResources = m_pMeshRenderResources;
	params.m_pNumShadowMapCommandsBuffer = m_pCreateShadowMapCommandsPass->GetNumSpotLightCommandsBuffer();
	params.m_pShadowMapCommandBuffer = m_pCreateShadowMapCommandsPass->GetSpotLightCommandBuffer();

	m_pRenderSpotLightTiledShadowMapPass->Record(&params);
	return params.m_pCommandList;
}

void DXApplication::InitConvertSpotLightTiledShadowMapPass()
{
	assert(m_pConvertSpotLightTiledShadowMapPass == nullptr);
	assert(m_pRenderSpotLightTiledShadowMapPass != nullptr);

	ConvertTiledShadowMapPass::InitParams params;
	params.m_pName = "ConvertSpotLightTiledShadowMapPass";
	params.m_pRenderEnv = m_pRenderEnv;
	
	assert(false);
	//params.m_InputResourceStates.m_TiledShadowMapState;
	//params.m_InputResourceStates.m_ShadowMapTileBufferState;
	//params.m_InputResourceStates.m_ConvertShadowMapParamsBufferState;
	//params.m_InputResourceStates.m_TiledVarianceShadowMapState;

	params.m_LightType = LightType_Spot;
	params.m_pTiledShadowMap = m_pRenderSpotLightTiledShadowMapPass->GetTiledShadowMap();
	
	assert(false);
	//params.m_pShadowMapTileBuffer;
	//params.m_pConvertShadowMapParamsBuffer;

	m_pConvertSpotLightTiledShadowMapPass = new ConvertTiledShadowMapPass(&params);
}

CommandList* DXApplication::RecordConvertSpotLightTiledShadowMapPass()
{
	assert(m_pConvertSpotLightTiledShadowMapPass != nullptr);
	assert(m_NumSpotLights > 0);

	ConvertTiledShadowMapPass::RenderParams params;
	params.m_pRenderEnv = m_pRenderEnv;
	params.m_pCommandList = m_pCommandListPool->Create(L"pConvertSpotLightTiledShadowMapCommandList");
	params.m_NumShadowMapTiles = m_NumActiveSpotLights;

	m_pConvertSpotLightTiledShadowMapPass->Record(&params);
	return params.m_pCommandList;
}

void DXApplication::InitCreateVoxelizeCommandsPass()
{	
	assert(m_pCreateVoxelizeCommandsPass == nullptr);
	assert(m_pFrustumMeshCullingPass != nullptr);
	assert(m_pCreateShadowMapCommandsPass != nullptr);
	assert(m_pMeshRenderResources != nullptr);
	
	const CreateShadowMapCommandsPass::ResourceStates* pCreateShadowMapCommandsPassStates =
		m_pCreateShadowMapCommandsPass->GetOutputResourceStates();

	CreateVoxelizeCommandsPass::InitParams params;
	params.m_pName = "CreateVoxelizeCommandsPass";
	params.m_pRenderEnv = m_pRenderEnv;
	
	params.m_InputResourceStates.m_NumMeshesBufferState = pCreateShadowMapCommandsPassStates->m_NumMeshesBufferState;
	params.m_InputResourceStates.m_MeshInfoBufferState = pCreateShadowMapCommandsPassStates->m_MeshInfoBufferState;
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
	assert(m_pFrustumMeshCullingPass != nullptr);
	assert(m_pCreateMainDrawCommandsPass != nullptr);
	assert(m_pCreateVoxelizeCommandsPass != nullptr);
	assert(m_pCreateShadowMapCommandsPass != nullptr);
	assert(m_pRenderGBufferFalseNegativePass != nullptr);
		
	const CreateMainDrawCommandsPass::ResourceStates* pCreateMainDrawCommandsPassStates =
		m_pCreateMainDrawCommandsPass->GetOutputResourceStates();

	const CreateVoxelizeCommandsPass::ResourceStates* pCreateVoxelizeCommandsPassStates =
		m_pCreateVoxelizeCommandsPass->GetOutputResourceStates();

	const RenderGBufferPass::ResourceStates* pRenderGBufferFalseNegativePassStates =
		m_pRenderGBufferFalseNegativePass->GetOutputResourceStates();
		
	const CreateShadowMapCommandsPass::ResourceStates* pCreateShadowMapCommandsPassStates =
		m_pCreateShadowMapCommandsPass->GetOutputResourceStates();
		
	VoxelizePass::InitParams params;
	params.m_pName = "VoxelizePass";
	params.m_pRenderEnv = m_pRenderEnv;
	
	params.m_InputResourceStates.m_NumCommandsPerMeshTypeBufferState = pCreateVoxelizeCommandsPassStates->m_NumCommandsPerMeshTypeBufferState;
	params.m_InputResourceStates.m_VoxelizeCommandBufferState = pCreateVoxelizeCommandsPassStates->m_VoxelizeCommandBufferState;
	params.m_InputResourceStates.m_InstanceIndexBufferState = pCreateMainDrawCommandsPassStates->m_InstanceIndexBufferState;
		
	if (m_NumPointLights > 0)
	{
		assert(m_pRenderPointLightTiledShadowMapPass != nullptr);
		const RenderTiledShadowMapPass::ResourceStates* pRenderPointLightTiledShadowMapStates =
			m_pRenderPointLightTiledShadowMapPass->GetOutputResourceStates();

		params.m_InputResourceStates.m_InstanceWorldMatrixBufferState = pRenderPointLightTiledShadowMapStates->m_MeshInstanceWorldMatrixBufferState;
		params.m_InputResourceStates.m_PointLightWorldBoundsBufferState = pRenderPointLightTiledShadowMapStates->m_LightWorldBoundsOrPropsBufferState;
		params.m_InputResourceStates.m_PointLightPropsBufferState = D3D12_RESOURCE_STATE_COPY_DEST;
	}
	if (m_NumSpotLights > 0)
	{
		assert(m_pRenderSpotLightTiledShadowMapPass != nullptr);
		const RenderTiledShadowMapPass::ResourceStates* pRenderSpotLightTiledShadowMapStates =
			m_pRenderSpotLightTiledShadowMapPass->GetOutputResourceStates();

		params.m_InputResourceStates.m_InstanceWorldMatrixBufferState = pRenderSpotLightTiledShadowMapStates->m_MeshInstanceWorldMatrixBufferState;
		params.m_InputResourceStates.m_SpotLightWorldBoundsBufferState = pCreateShadowMapCommandsPassStates->m_SpotLightWorldBoundsBufferState;
		params.m_InputResourceStates.m_SpotLightPropsBufferState = pRenderSpotLightTiledShadowMapStates->m_LightWorldBoundsOrPropsBufferState;
	}
	
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
	params.m_EnablePointLights = m_NumPointLights > 0;
	params.m_pPointLightWorldBoundsBuffer = m_pActivePointLightWorldBoundsBuffer;
	params.m_pPointLightPropsBuffer = m_pActivePointLightPropsBuffer;
	params.m_EnableSpotLights = m_NumSpotLights > 0;
	params.m_pSpotLightWorldBoundsBuffer = m_pActiveSpotLightWorldBoundsBuffer;
	params.m_pSpotLightPropsBuffer = m_pActiveSpotLightPropsBuffer;

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
	params.m_pAppDataBuffer = m_UploadAppDataBuffers[m_BackBufferIndex];
	params.m_NumPointLights = m_NumActivePointLights;
	params.m_NumSpotLights = m_NumActiveSpotLights;

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
	params.m_pName = "TiledShadingPass";
	params.m_pRenderEnv = m_pRenderEnv;
	params.m_ShadingMode = ShadingMode_BlinnPhong;

	params.m_InputResourceStates.m_AccumLightTextureState = D3D12_RESOURCE_STATE_RENDER_TARGET;
	params.m_InputResourceStates.m_MeshTypeDepthTextureState = pFillMeshTypeDepthBufferPassStates->m_MeshTypeDepthTextureState;
	params.m_InputResourceStates.m_ShadingRectangleMinPointBufferState = pCalcShadingRectanglesPassStates->m_ShadingRectangleMinPointBufferState;
	params.m_InputResourceStates.m_ShadingRectangleMaxPointBufferState = pCalcShadingRectanglesPassStates->m_ShadingRectangleMaxPointBufferState;
	params.m_InputResourceStates.m_DepthTextureState = pTiledLightCullingPassStates->m_DepthTextureState;
	params.m_InputResourceStates.m_TexCoordTextureState = pRenderGBufferFalseNegativePassStates->m_TexCoordTextureState;
	params.m_InputResourceStates.m_NormalTextureState = pRenderGBufferFalseNegativePassStates->m_NormalTextureState;
	params.m_InputResourceStates.m_MaterialIDTextureState = pFillMeshTypeDepthBufferPassStates->m_MaterialIDTextureState;
	params.m_InputResourceStates.m_FirstResourceIndexPerMaterialIDBufferState = pVoxelizePassStates->m_FirstResourceIndexPerMaterialIDBufferState;
	params.m_InputResourceStates.m_PointLightWorldBoundsBufferState = pVoxelizePassStates->m_PointLightWorldBoundsBufferState;
	params.m_InputResourceStates.m_PointLightPropsBufferState = pVoxelizePassStates->m_PointLightPropsBufferState;
	params.m_InputResourceStates.m_PointLightIndexPerTileBufferState = pTiledLightCullingPassStates->m_PointLightIndexPerTileBufferState;
	params.m_InputResourceStates.m_PointLightRangePerTileBufferState = pTiledLightCullingPassStates->m_PointLightRangePerTileBufferState;
	
	assert(false);
	//params.m_InputResourceStates.m_PointLightTiledVarianceShadowMapState;
	//params.m_InputResourceStates.m_PointLightViewProjMatrixBufferState;

	params.m_InputResourceStates.m_SpotLightWorldBoundsBufferState = pVoxelizePassStates->m_SpotLightWorldBoundsBufferState;
	params.m_InputResourceStates.m_SpotLightPropsBufferState = pVoxelizePassStates->m_SpotLightPropsBufferState;
	params.m_InputResourceStates.m_SpotLightIndexPerTileBufferState = pTiledLightCullingPassStates->m_SpotLightIndexPerTileBufferState;
	params.m_InputResourceStates.m_SpotLightRangePerTileBufferState = pTiledLightCullingPassStates->m_SpotLightRangePerTileBufferState;

	assert(false);
	//params.m_InputResourceStates.m_SpotLightTiledVarianceShadowMapState;
	//params.m_InputResourceStates.m_SpotLightViewProjMatrixBufferState;
	//params.m_InputResourceStates.m_SpotLightShadowMapTileBufferState;
	
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

	params.m_EnablePointLights = m_NumPointLights > 0;
	params.m_pPointLightWorldBoundsBuffer = m_pActivePointLightWorldBoundsBuffer;
	params.m_pPointLightPropsBuffer = m_pActivePointLightPropsBuffer;
	params.m_pPointLightIndexPerTileBuffer = m_pTiledLightCullingPass->GetPointLightIndexPerTileBuffer();
	params.m_pPointLightRangePerTileBuffer = m_pTiledLightCullingPass->GetPointLightRangePerTileBuffer();

	assert(false);
	//params.m_pPointLightTiledVarianceShadowMap;
	//params.m_pPointLightViewProjMatrixBuffer;
	
	params.m_EnableSpotLights = m_NumSpotLights > 0;
	params.m_pSpotLightWorldBoundsBuffer = m_pActiveSpotLightWorldBoundsBuffer;
	params.m_pSpotLightPropsBuffer = m_pActiveSpotLightPropsBuffer;
	params.m_pSpotLightIndexPerTileBuffer = m_pTiledLightCullingPass->GetSpotLightIndexPerTileBuffer();
	params.m_pSpotLightRangePerTileBuffer = m_pTiledLightCullingPass->GetSpotLightRangePerTileBuffer();

	assert(false);
	//params.m_pSpotLightTiledVarianceShadowMap;
	//params.m_pSpotLightViewProjMatrixBuffer;
	//params.m_pSpotLightShadowMapTileBuffer;
	
	m_pTiledShadingPass = new TiledShadingPass(&params);
}

CommandList* DXApplication::RecordTiledShadingPass()
{
	assert(m_pTiledShadingPass != nullptr);

	TiledShadingPass::RenderParams params;
	params.m_pRenderEnv = m_pRenderEnv;
	params.m_pCommandList = m_pCommandListPool->Create(L"pTiledShadingCommandList");
	params.m_pAppDataBuffer = m_UploadAppDataBuffers[m_BackBufferIndex];
	params.m_pViewport = m_pBackBufferViewport;

	m_pTiledShadingPass->Record(&params);
	return params.m_pCommandList;
}

CommandList* DXApplication::RecordVisualizeDisplayResultPass()
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

	if (m_DisplayResult == DisplayResult::PointLightTiledShadowMap)
		return RecordVisualizePointLightTiledShadowMapPass();

	if (m_DisplayResult == DisplayResult::SpotLightTiledShadowMap)
		return RecordVisualizeSpotLightTiledShadowMapPass();

	if (m_DisplayResult == DisplayResult::VoxelRelectance)
		return RecordVisualizeVoxelReflectancePass();
		
	assert(false);
	return nullptr;
}

CommandList* DXApplication::RecordPostRenderPass()
{
	static ResourceTransitionBarrier resourceBarriers[2];
	u8 numBarriers = 0;

	CommandList* pCommandList = m_pCommandListPool->Create(L"pPostRenderCommandList");
	pCommandList->Begin();
#ifdef ENABLE_PROFILING
	u32 profileIndex = m_pGPUProfiler->StartProfile(pCommandList, "PostRenderPass");
#endif // ENABLE_PROFILING
	
	ColorTexture* pRenderTarget = m_pSwapChain->GetBackBuffer(m_BackBufferIndex);
	resourceBarriers[numBarriers++] = ResourceTransitionBarrier(pRenderTarget,
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_PRESENT);

	if (m_DisplayResult == DisplayResult::ShadingResult)
	{
		resourceBarriers[numBarriers++] = ResourceTransitionBarrier(m_pAccumLightTexture,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
			D3D12_RESOURCE_STATE_RENDER_TARGET);
	}
	else if (m_DisplayResult == DisplayResult::ReprojectedDepthBuffer)
	{
		DepthTexture* pReprojectedDepthTexture = m_pDownscaleAndReprojectDepthPass->GetReprojectedDepthTexture();
		resourceBarriers[numBarriers++] = ResourceTransitionBarrier(pReprojectedDepthTexture,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
			D3D12_RESOURCE_STATE_DEPTH_WRITE);
	}
	else if (m_DisplayResult == DisplayResult::DepthBufferWithMeshType)
	{
		const FillMeshTypeDepthBufferPass::ResourceStates* pResourceStates =
			m_pFillMeshTypeDepthBufferPass->GetOutputResourceStates();

		DepthTexture* pMeshTypeDepthTexture = m_pFillMeshTypeDepthBufferPass->GetMeshTypeDepthTexture();
		resourceBarriers[numBarriers++] = ResourceTransitionBarrier(pMeshTypeDepthTexture,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
			D3D12_RESOURCE_STATE_DEPTH_WRITE);
	}
	else if (m_DisplayResult == DisplayResult::PointLightTiledShadowMap)
	{
		DepthTexture* pTiledShadowMap = m_pRenderPointLightTiledShadowMapPass->GetTiledShadowMap();
		resourceBarriers[numBarriers++] = ResourceTransitionBarrier(pTiledShadowMap,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
			D3D12_RESOURCE_STATE_DEPTH_WRITE);
	}
	else if (m_DisplayResult == DisplayResult::SpotLightTiledShadowMap)
	{
		DepthTexture* pTiledShadowMap = m_pRenderSpotLightTiledShadowMapPass->GetTiledShadowMap();
		resourceBarriers[numBarriers++] = ResourceTransitionBarrier(pTiledShadowMap,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
			D3D12_RESOURCE_STATE_DEPTH_WRITE);
	}
	else if (m_DisplayResult == DisplayResult::VoxelRelectance)
	{
		ColorTexture* pVoxelReflectanceTexture = m_pVoxelizePass->GetVoxelReflectanceTexture();
		resourceBarriers[numBarriers++] = ResourceTransitionBarrier(pVoxelReflectanceTexture,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	}

	pCommandList->ResourceBarrier(numBarriers, resourceBarriers);

#ifdef ENABLE_PROFILING
	m_pGPUProfiler->EndProfile(pCommandList, profileIndex);
#endif // ENABLE_PROFILING
	pCommandList->End();

	return pCommandList;
}

void DXApplication::InitPointLightRenderResources(Scene* pScene)
{
	const MemoryRange readRange(0, 0);

	PointLight** ppPointLights = pScene->GetPointLights();
	m_NumPointLights = u32(pScene->GetNumPointLights());

	m_pPointLights = new PointLightData[m_NumPointLights];
	m_ppActivePointLights = new PointLightData*[m_NumPointLights];

	Vector3f lookAtDir[kNumCubeMapFaces];
	lookAtDir[kCubeMapFacePosX] = Vector3f::RIGHT;
	lookAtDir[kCubeMapFaceNegX] = Vector3f::LEFT;
	lookAtDir[kCubeMapFacePosY] = Vector3f::UP;
	lookAtDir[kCubeMapFaceNegY] = Vector3f::DOWN;
	lookAtDir[kCubeMapFacePosZ] = Vector3f::FORWARD;
	lookAtDir[kCubeMapFaceNegZ] = Vector3f::BACK;

	Vector3f upDir[kNumCubeMapFaces];
	upDir[kCubeMapFacePosX] = Vector3f::UP;
	upDir[kCubeMapFaceNegX] = Vector3f::UP;
	upDir[kCubeMapFacePosY] = Vector3f::FORWARD;
	upDir[kCubeMapFaceNegY] = Vector3f::BACK;
	upDir[kCubeMapFacePosZ] = Vector3f::UP;
	upDir[kCubeMapFaceNegZ] = Vector3f::UP;

	for (decltype(m_NumPointLights) lightIndex = 0; lightIndex < m_NumPointLights; ++lightIndex)
	{
		const PointLight* pLight = ppPointLights[lightIndex];

		const Transform& lightWorldSpaceTransform = pLight->GetTransform();
		const Vector3f& lightWorldSpacePos = lightWorldSpaceTransform.GetPosition();

		Matrix4f projMatrix = CreatePerspectiveFovProjMatrix(PI_DIV_2, 1.0f, pLight->GetShadowNearPlane(), pLight->GetRange());

		m_pPointLights[lightIndex].m_Color = pLight->GetColor();
		m_pPointLights[lightIndex].m_WorldBounds = Sphere(lightWorldSpacePos, pLight->GetRange());
		
		m_pPointLights[lightIndex].m_LightViewNearPlane = pLight->GetShadowNearPlane();
		m_pPointLights[lightIndex].m_LightRcpViewClipRange = Rcp(pLight->GetRange() - pLight->GetShadowNearPlane());
		m_pPointLights[lightIndex].m_LightProjMatrix43 = projMatrix.m_32;
		m_pPointLights[lightIndex].m_LightProjMatrix33 = projMatrix.m_22;

		m_pPointLights[lightIndex].m_AffectedScreenArea = 0;

		for (u8 faceIndex = 0; faceIndex < kNumCubeMapFaces; ++faceIndex)
		{
			Matrix4f viewMatrix = CreateLookAtMatrix(lightWorldSpacePos, lightWorldSpacePos + lookAtDir[faceIndex], upDir[faceIndex]);

			Matrix4f viewProjMatrix = viewMatrix * projMatrix;
			m_pPointLights[lightIndex].m_ViewProjMatrices[faceIndex] = viewProjMatrix;

			Frustum lightWorldFrustum(viewProjMatrix);
			m_pPointLights[lightIndex].m_WorldFrustums[faceIndex].m_LeftPlane = lightWorldFrustum.m_Planes[Frustum::LeftPlane];
			m_pPointLights[lightIndex].m_WorldFrustums[faceIndex].m_RightPlane = lightWorldFrustum.m_Planes[Frustum::RightPlane];;
			m_pPointLights[lightIndex].m_WorldFrustums[faceIndex].m_TopPlane = lightWorldFrustum.m_Planes[Frustum::TopPlane];;
			m_pPointLights[lightIndex].m_WorldFrustums[faceIndex].m_BottomPlane = lightWorldFrustum.m_Planes[Frustum::BottomPlane];;
		}		

		m_ppActivePointLights[lightIndex] = nullptr;
	}

	StructuredBufferDesc lightWorldBoundsBufferDesc(m_NumPointLights, sizeof(Sphere), true, false);
	StructuredBufferDesc lightPropsBufferDesc(m_NumPointLights, sizeof(PointLightProps), true, false);
	StructuredBufferDesc convertShadowMapParamsBufferDesc(m_NumPointLights, sizeof(ConvertShadowMapParams), true, false);
	StructuredBufferDesc lightWorldFrustumBufferDesc(kNumCubeMapFaces * m_NumPointLights, sizeof(LightFrustum), true, false);
	StructuredBufferDesc lightViewProjMatrixBufferDesc(kNumCubeMapFaces * m_NumPointLights, sizeof(Matrix4f), true, false);
	StructuredBufferDesc lightShadowMapTileBufferDesc(kNumCubeMapFaces * m_NumPointLights, sizeof(ShadowMapTile), true, false);
	
	for (u8 index = 0; index < kNumBackBuffers; ++index)
	{
		assert(m_UploadActivePointLightWorldBoundsBuffers[index] == nullptr);
		m_UploadActivePointLightWorldBoundsBuffers[index] = new Buffer(m_pRenderEnv, m_pRenderEnv->m_pUploadHeapProps,
			&lightWorldBoundsBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, L"m_pUploadActivePointLightWorldBoundsBuffer");
		m_UploadActivePointLightWorldBounds[index] = m_UploadActivePointLightWorldBoundsBuffers[index]->Map(0, &readRange);

		assert(m_UploadActivePointLightPropsBuffers[index] == nullptr);
		m_UploadActivePointLightPropsBuffers[index] = new Buffer(m_pRenderEnv, m_pRenderEnv->m_pUploadHeapProps,
			&lightPropsBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, L"m_pUploadActivePointLightPropsBuffer");
		m_UploadActivePointLightProps[index] = m_UploadActivePointLightPropsBuffers[index]->Map(0, &readRange);

		assert(m_UploadActivePointLightWorldFrustumBuffers[index] == nullptr);
		m_UploadActivePointLightWorldFrustumBuffers[index] = new Buffer(m_pRenderEnv, m_pRenderEnv->m_pUploadHeapProps,
			&lightWorldFrustumBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, L"m_pUploadActivePointLightWorldFrustumBuffer");
		m_UploadActivePointLightWorldFrustums[index] = m_UploadActivePointLightWorldFrustumBuffers[index]->Map(0, &readRange);

		assert(m_UploadActivePointLightViewProjMatrixBuffers[index] == nullptr);
		m_UploadActivePointLightViewProjMatrixBuffers[index] = new Buffer(m_pRenderEnv, m_pRenderEnv->m_pUploadHeapProps,
			&lightViewProjMatrixBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, L"m_pUploadActivePointLightViewProjMatrixBuffer");
		m_UploadActivePointLightViewProjMatrices[index] = m_UploadActivePointLightViewProjMatrixBuffers[index]->Map(0, &readRange);

		assert(m_UploadActivePointLightShadowMapTileBuffers[index] == nullptr);
		m_UploadActivePointLightShadowMapTileBuffers[index] = new Buffer(m_pRenderEnv, m_pRenderEnv->m_pUploadHeapProps,
			&lightShadowMapTileBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, L"m_pUploadActivePointLightShadowMapTileBuffer");
		m_UploadActivePointLightShadowMapTiles[index] = m_UploadActivePointLightShadowMapTileBuffers[index]->Map(0, &readRange);

		assert(m_UploadActivePointLightConvertShadowMapParamsBuffers[index] == nullptr);
		m_UploadActivePointLightConvertShadowMapParamsBuffers[index] = new Buffer(m_pRenderEnv, m_pRenderEnv->m_pUploadHeapProps,
			&convertShadowMapParamsBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, L"m_pUploadActivePointLightConvertShadowMapParamsBuffer");
		m_UploadActivePointLightConvertShadowMapParams[index] = m_UploadActivePointLightConvertShadowMapParamsBuffers[index]->Map(0, &readRange);
	}

	assert(m_pActivePointLightWorldBoundsBuffer == nullptr);
	m_pActivePointLightWorldBoundsBuffer = new Buffer(m_pRenderEnv, m_pRenderEnv->m_pDefaultHeapProps,
		&lightWorldBoundsBufferDesc, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, L"m_pActivePointLightWorldBoundsBuffer");

	assert(m_pActivePointLightPropsBuffer == nullptr);
	m_pActivePointLightPropsBuffer = new Buffer(m_pRenderEnv, m_pRenderEnv->m_pDefaultHeapProps,
		&lightPropsBufferDesc, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, L"m_pActivePointLightPropsBuffer");

	assert(m_pActivePointLightConvertShadowMapParamsBuffer == nullptr);
	m_pActivePointLightConvertShadowMapParamsBuffer = new Buffer(m_pRenderEnv, m_pRenderEnv->m_pDefaultHeapProps,
		&convertShadowMapParamsBufferDesc, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, L"m_pActivePointLightConvertShadowMapParamsBuffer");

	assert(m_pActivePointLightWorldFrustumBuffer == nullptr);
	m_pActivePointLightWorldFrustumBuffer = new Buffer(m_pRenderEnv, m_pRenderEnv->m_pDefaultHeapProps,
		&lightWorldFrustumBufferDesc, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, L"m_pActivePointLightWorldFrustumBuffer");

	assert(m_pActivePointLightViewProjMatrixBuffer == nullptr);
	m_pActivePointLightViewProjMatrixBuffer = new Buffer(m_pRenderEnv, m_pRenderEnv->m_pDefaultHeapProps,
		&lightViewProjMatrixBufferDesc, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, L"m_pActivePointLightViewProjMatrixBuffer");
	
	assert(m_pActivePointLightShadowMapTileBuffer == nullptr);
	m_pActivePointLightShadowMapTileBuffer = new Buffer(m_pRenderEnv, m_pRenderEnv->m_pDefaultHeapProps,
		&lightShadowMapTileBufferDesc, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, L"m_pActivePointLightShadowMapTileBuffer");
}

void DXApplication::InitSpotLightRenderResources(Scene* pScene)
{
	const MemoryRange readRange(0, 0);

	SpotLight** ppSpotLights = pScene->GetSpotLights();
	m_NumSpotLights = u32(pScene->GetNumSpotLights());

	m_pSpotLights = new SpotLightData[m_NumSpotLights];
	m_ppActiveSpotLights = new SpotLightData*[m_NumSpotLights];

	for (decltype(m_NumSpotLights) lightIndex = 0; lightIndex < m_NumSpotLights; ++lightIndex)
	{
		SpotLight* pLight = ppSpotLights[lightIndex];
		
		const Transform& lightWorldSpaceTransform = pLight->GetTransform();
		const Vector3f& lightWorldSpacePos = lightWorldSpaceTransform.GetPosition();
		const BasisAxes lightWorldSpaceBasis = ExtractBasisAxes(lightWorldSpaceTransform.GetRotation());

		Matrix4f projMatrix = CreatePerspectiveFovProjMatrix(pLight->GetOuterConeAngle(), 1.0f, pLight->GetShadowNearPlane(), pLight->GetRange());

		assert(IsNormalized(lightWorldSpaceBasis.m_ZAxis));
		const Vector3f& lightWorldSpaceDir = lightWorldSpaceBasis.m_ZAxis;
		assert(IsNormalized(lightWorldSpaceBasis.m_YAxis));
		const Vector3f& lightWorldSpaceUpDir = lightWorldSpaceBasis.m_YAxis;

		Cone lightWorldCone(lightWorldSpacePos, pLight->GetOuterConeAngle(), lightWorldSpaceDir, pLight->GetRange());
		m_pSpotLights[lightIndex].m_Color = pLight->GetColor();
		m_pSpotLights[lightIndex].m_WorldSpaceDir = lightWorldSpaceDir;
		m_pSpotLights[lightIndex].m_WorldBounds = ExtractBoundingSphere(lightWorldCone);
		
		m_pSpotLights[lightIndex].m_LightViewNearPlane = pLight->GetShadowNearPlane();
		m_pSpotLights[lightIndex].m_LightRcpViewClipRange = Rcp(pLight->GetRange() - pLight->GetShadowNearPlane());
		m_pSpotLights[lightIndex].m_LightProjMatrix43 = projMatrix.m_32;
		m_pSpotLights[lightIndex].m_LightProjMatrix33 = projMatrix.m_22;

		m_pSpotLights[lightIndex].m_LightRange = pLight->GetRange();
		m_pSpotLights[lightIndex].m_CosHalfInnerConeAngle = Cos(0.5f * pLight->GetInnerConeAngle());
		m_pSpotLights[lightIndex].m_CosHalfOuterConeAngle = Cos(0.5f * pLight->GetOuterConeAngle());
		m_pSpotLights[lightIndex].m_AffectedScreenArea = 0;

		Matrix4f viewMatrix = CreateLookAtMatrix(lightWorldSpacePos, lightWorldSpacePos + lightWorldSpaceDir, lightWorldSpaceUpDir);
		Matrix4f viewProjMatrix = viewMatrix * projMatrix;
		m_pSpotLights[lightIndex].m_ViewProjMatrix = viewProjMatrix;

		Frustum lightWorldFrustum(viewProjMatrix);
		m_pSpotLights[lightIndex].m_WorldFrustum.m_LeftPlane = lightWorldFrustum.m_Planes[Frustum::LeftPlane];
		m_pSpotLights[lightIndex].m_WorldFrustum.m_RightPlane = lightWorldFrustum.m_Planes[Frustum::RightPlane];
		m_pSpotLights[lightIndex].m_WorldFrustum.m_TopPlane = lightWorldFrustum.m_Planes[Frustum::TopPlane];
		m_pSpotLights[lightIndex].m_WorldFrustum.m_BottomPlane = lightWorldFrustum.m_Planes[Frustum::BottomPlane];

		m_ppActiveSpotLights[lightIndex] = nullptr;
	}

	StructuredBufferDesc lightWorldBoundsBufferDesc(m_NumSpotLights, sizeof(Sphere), true, false);
	StructuredBufferDesc lightPropsBufferDesc(m_NumSpotLights, sizeof(SpotLightProps), true, false);
	StructuredBufferDesc convertShadowMapParamsBufferDesc(m_NumSpotLights, sizeof(ConvertShadowMapParams), true, false);
	StructuredBufferDesc lightWorldFrustumBufferDesc(m_NumSpotLights, sizeof(LightFrustum), true, false);
	StructuredBufferDesc lightViewProjMatrixBufferDesc(m_NumSpotLights, sizeof(Matrix4f), true, false);
	StructuredBufferDesc lightShadowMapTileBufferDesc(m_NumSpotLights, sizeof(ShadowMapTile), true, false);
	
	for (u8 index = 0; index < kNumBackBuffers; ++index)
	{
		assert(m_UploadActiveSpotLightWorldBoundsBuffers[index] == nullptr);
		m_UploadActiveSpotLightWorldBoundsBuffers[index] = new Buffer(m_pRenderEnv, m_pRenderEnv->m_pUploadHeapProps,
			&lightWorldBoundsBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, L"m_pUploadActiveSpotLightWorldBoundsBuffer");
		m_UploadActiveSpotLightWorldBounds[index] = m_UploadActiveSpotLightWorldBoundsBuffers[index]->Map(0, &readRange);

		assert(m_UploadActiveSpotLightPropsBuffers[index] == nullptr);
		m_UploadActiveSpotLightPropsBuffers[index] = new Buffer(m_pRenderEnv, m_pRenderEnv->m_pUploadHeapProps,
			&lightPropsBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, L"m_pUploadActiveSpotLightPropsBuffer");
		m_UploadActiveSpotLightProps[index] = m_UploadActiveSpotLightPropsBuffers[index]->Map(0, &readRange);

		assert(m_UploadActiveSpotLightConvertShadowMapParamsBuffers[index] == nullptr);
		m_UploadActiveSpotLightConvertShadowMapParamsBuffers[index] = new Buffer(m_pRenderEnv, m_pRenderEnv->m_pUploadHeapProps,
			&convertShadowMapParamsBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, L"m_pUploadActiveSpotLightConvertShadowMapParamsBuffer");
		m_UploadActiveSpotLightConvertShadowMapParams[index] = m_UploadActiveSpotLightConvertShadowMapParamsBuffers[index]->Map(0, &readRange);

		assert(m_UploadActiveSpotLightWorldFrustumBuffers[index] == nullptr);
		m_UploadActiveSpotLightWorldFrustumBuffers[index] = new Buffer(m_pRenderEnv, m_pRenderEnv->m_pUploadHeapProps,
			&lightWorldFrustumBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, L"m_pUploadActiveSpotLightWorldFrustumBuffer");
		m_UploadActiveSpotLightWorldFrustums[index] = m_UploadActiveSpotLightWorldFrustumBuffers[index]->Map(0, &readRange);

		assert(m_UploadActiveSpotLightViewProjMatrixBuffers[index] == nullptr);
		m_UploadActiveSpotLightViewProjMatrixBuffers[index] = new Buffer(m_pRenderEnv, m_pRenderEnv->m_pUploadHeapProps,
			&lightViewProjMatrixBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, L"m_pUploadActiveSpotLightViewProjMatrixBuffer");
		m_UploadActiveSpotLightViewProjMatrices[index] = m_UploadActiveSpotLightViewProjMatrixBuffers[index]->Map(0, &readRange);

		assert(m_UploadActiveSpotLightShadowMapTileBuffers[index] == nullptr);
		m_UploadActiveSpotLightShadowMapTileBuffers[index] = new Buffer(m_pRenderEnv, m_pRenderEnv->m_pUploadHeapProps,
			&lightShadowMapTileBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, L"m_pUploadActiveSpotLightShadowMapTileBuffer");
		m_UploadActiveSpotLightShadowMapTiles[index] = m_UploadActiveSpotLightShadowMapTileBuffers[index]->Map(0, &readRange);
	}

	assert(m_pActiveSpotLightWorldBoundsBuffer == nullptr);
	m_pActiveSpotLightWorldBoundsBuffer = new Buffer(m_pRenderEnv, m_pRenderEnv->m_pDefaultHeapProps,
		&lightWorldBoundsBufferDesc, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, L"m_pActiveSpotLightWorldBoundsBuffer");

	assert(m_pActiveSpotLightPropsBuffer == nullptr);
	m_pActiveSpotLightPropsBuffer = new Buffer(m_pRenderEnv, m_pRenderEnv->m_pDefaultHeapProps,
		&lightPropsBufferDesc, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, L"m_pActiveSpotLightPropsBuffer");
	
	assert(m_pActiveSpotLightConvertShadowMapParamsBuffer == nullptr);
	m_pActiveSpotLightConvertShadowMapParamsBuffer = new Buffer(m_pRenderEnv, m_pRenderEnv->m_pDefaultHeapProps,
		&convertShadowMapParamsBufferDesc, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, L"m_pActiveSpotLightConvertShadowMapParamsBuffer");

	assert(m_pActiveSpotLightWorldFrustumBuffer == nullptr);
	m_pActiveSpotLightWorldFrustumBuffer = new Buffer(m_pRenderEnv, m_pRenderEnv->m_pDefaultHeapProps,
		&lightWorldFrustumBufferDesc, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, L"m_pActiveSpotLightWorldFrustumBuffer");

	assert(m_pActiveSpotLightViewProjMatrixBuffer == nullptr);
	m_pActiveSpotLightViewProjMatrixBuffer = new Buffer(m_pRenderEnv, m_pRenderEnv->m_pDefaultHeapProps,
		&lightViewProjMatrixBufferDesc, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, L"m_pActiveSpotLightViewProjMatrixBuffer");

	assert(m_pActiveSpotLightShadowMapTileBuffer == nullptr);
	m_pActiveSpotLightShadowMapTileBuffer = new Buffer(m_pRenderEnv, m_pRenderEnv->m_pDefaultHeapProps,
		&lightShadowMapTileBufferDesc, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, L"m_pActiveSpotLightShadowMapTileBuffer");
}

void DXApplication::SetupPointLightDataForUpload(const Frustum& cameraWorldFrustum)
{
#ifdef ENABLE_PROFILING
	u32 profileIndex = m_pCPUProfiler->StartProfile("SetupPointLightDataForUpload");
#endif // ENABLE_PROFILING
	
	decltype(m_NumPointLights) numVisiblePointLights = 0;
	for (decltype(m_NumPointLights) lightIndex = 0; lightIndex < m_NumPointLights; ++lightIndex)
	{
		PointLightData& lightData = m_pPointLights[lightIndex];
		if (TestSphereAgainstFrustum(cameraWorldFrustum, lightData.m_WorldBounds))
			m_ppActivePointLights[numVisiblePointLights++] = &lightData;
	}

	const Vector2f screenSize(m_pBackBufferViewport->Width, m_pBackBufferViewport->Height);
	for (decltype(numVisiblePointLights) lightIndex = 0; lightIndex < numVisiblePointLights; ++lightIndex)
	{
		PointLightData* pLightData = m_ppActivePointLights[lightIndex];
		pLightData->m_AffectedScreenArea = CalcScreenAreaAffectedByLight(pLightData->m_WorldBounds, screenSize, *m_pCamera);
	}

	auto hasLargerSizeOnScreen = [](const PointLightData* pLightData1, const PointLightData* pLightData2)
	{
		return (pLightData1->m_AffectedScreenArea > pLightData2->m_AffectedScreenArea);
	};
	std::sort(m_ppActivePointLights, m_ppActivePointLights + numVisiblePointLights, hasLargerSizeOnScreen);

	m_NumActivePointLights = 0;
	assert(m_pPointLightShadowMapTileAllocator != nullptr);
	m_pPointLightShadowMapTileAllocator->Reset();

	while (m_NumActivePointLights < numVisiblePointLights)
	{
		PointLightData* pLightData = m_ppActivePointLights[m_NumActivePointLights];

		u8 faceIndex = 0;
		ShadowMapTile shadowMapTile;
		while (faceIndex < kNumCubeMapFaces)
		{
			if (m_pPointLightShadowMapTileAllocator->Allocate(pLightData->m_AffectedScreenArea, &shadowMapTile))
				pLightData->m_ShadowMapTiles[faceIndex++] = shadowMapTile;
			else
				break;
		}

		if (faceIndex == kNumCubeMapFaces)
			++m_NumActivePointLights;
		else
			break;
	}
	
	Sphere* pUploadActiveLightWorldBounds = (Sphere*)m_UploadActivePointLightWorldBounds[m_BackBufferIndex];
	PointLightProps* pUploadActiveLightProps = (PointLightProps*)m_UploadActivePointLightProps[m_BackBufferIndex];
	LightFrustum* pUploadActiveLightWorldFrustums = (LightFrustum*)m_UploadActivePointLightWorldFrustums[m_BackBufferIndex];
	Matrix4f* pUploadActiveLightViewProjMatrices = (Matrix4f*)m_UploadActivePointLightViewProjMatrices[m_BackBufferIndex];
	ShadowMapTile* pUploadActiveLightShadowMapTiles = (ShadowMapTile*)m_UploadActivePointLightShadowMapTiles[m_BackBufferIndex];
	ConvertShadowMapParams* pUploadActiveLightConvertShadowMapParams = (ConvertShadowMapParams*)m_UploadActivePointLightConvertShadowMapParams[m_BackBufferIndex];

	for (decltype(m_NumActivePointLights) lightIndex = 0; lightIndex < m_NumActivePointLights; ++lightIndex)
	{
		const PointLightData* pLightData = m_ppActivePointLights[lightIndex];

		pUploadActiveLightWorldBounds[lightIndex] = pLightData->m_WorldBounds;
		pUploadActiveLightProps[lightIndex].m_Color = pLightData->m_Color;

		pUploadActiveLightConvertShadowMapParams[lightIndex].m_LightViewNearPlane = pLightData->m_LightViewNearPlane;
		pUploadActiveLightConvertShadowMapParams[lightIndex].m_LightRcpViewClipRange = pLightData->m_LightRcpViewClipRange;
		pUploadActiveLightConvertShadowMapParams[lightIndex].m_LightProjMatrix43 = pLightData->m_LightProjMatrix43;
		pUploadActiveLightConvertShadowMapParams[lightIndex].m_LightProjMatrix33 = pLightData->m_LightProjMatrix33;

		for (decltype(m_NumActivePointLights) localFaceIndex = 0; localFaceIndex < kNumCubeMapFaces; ++localFaceIndex)
		{
			decltype(localFaceIndex) globalFaceIndex = kNumCubeMapFaces * lightIndex + localFaceIndex;

			pUploadActiveLightWorldFrustums[globalFaceIndex] = pLightData->m_WorldFrustums[localFaceIndex];
			pUploadActiveLightShadowMapTiles[globalFaceIndex] = pLightData->m_ShadowMapTiles[localFaceIndex];
			
			const Matrix4f& viewProjMatrix = pLightData->m_ViewProjMatrices[localFaceIndex];
			Matrix4f shadowMapTileMatrix = CreateShadowMapTileMatrix(pLightData->m_ShadowMapTiles[localFaceIndex]);

			pUploadActiveLightViewProjMatrices[globalFaceIndex] = viewProjMatrix * shadowMapTileMatrix;
		}
	}

#ifdef ENABLE_PROFILING
	m_pCPUProfiler->EndProfile(profileIndex);
#endif // ENABLE_PROFILING
}

void DXApplication::SetupSpotLightDataForUpload(const Frustum& cameraWorldFrustum)
{
#ifdef ENABLE_PROFILING
	u32 profileIndex = m_pCPUProfiler->StartProfile("SetupSpotLightDataForUpload");
#endif // ENABLE_PROFILING

	decltype(m_NumSpotLights) numVisibleSpotLights = 0;
	for (decltype(m_NumSpotLights) lightIndex = 0; lightIndex < m_NumSpotLights; ++lightIndex)
	{
		SpotLightData& lightData = m_pSpotLights[lightIndex];
		if (TestSphereAgainstFrustum(cameraWorldFrustum, lightData.m_WorldBounds))
			m_ppActiveSpotLights[numVisibleSpotLights++] = &lightData;
	}

	const Vector2f screenSize(m_pBackBufferViewport->Width, m_pBackBufferViewport->Height);
	for (decltype(numVisibleSpotLights) lightIndex = 0; lightIndex < numVisibleSpotLights; ++lightIndex)
	{
		SpotLightData* pLightData = m_ppActiveSpotLights[lightIndex];
		pLightData->m_AffectedScreenArea = CalcScreenAreaAffectedByLight(pLightData->m_WorldBounds, screenSize, *m_pCamera);
	}

	auto hasLargerSizeOnScreen = [](const SpotLightData* pLightData1, const SpotLightData* pLightData2)
	{
		return (pLightData1->m_AffectedScreenArea > pLightData2->m_AffectedScreenArea);
	};
	std::sort(m_ppActiveSpotLights, m_ppActiveSpotLights + numVisibleSpotLights, hasLargerSizeOnScreen);
	
	m_NumActiveSpotLights = 0;
	assert(m_pSpotLightShadowMapTileAllocator != nullptr);
	m_pSpotLightShadowMapTileAllocator->Reset();

	while (m_NumActiveSpotLights < numVisibleSpotLights)
	{
		SpotLightData* pLightData = m_ppActiveSpotLights[m_NumActiveSpotLights];

		ShadowMapTile shadowMapTile;
		if (m_pSpotLightShadowMapTileAllocator->Allocate(pLightData->m_AffectedScreenArea, &shadowMapTile))
		{
			pLightData->m_ShadowMapTile = shadowMapTile;
			++m_NumActiveSpotLights;
		}
		else
		{
			break;
		}
	}

	Sphere* pUploadActiveLightWorldBounds = (Sphere*)m_UploadActiveSpotLightWorldBounds[m_BackBufferIndex];
	SpotLightProps* pUploadActiveLightProps = (SpotLightProps*)m_UploadActiveSpotLightProps[m_BackBufferIndex];
	LightFrustum* pUploadActiveLightWorldFrustums = (LightFrustum*)m_UploadActiveSpotLightWorldFrustums[m_BackBufferIndex];
	Matrix4f* pUploadActiveLightViewProjMatrices = (Matrix4f*)m_UploadActiveSpotLightViewProjMatrices[m_BackBufferIndex];
	ShadowMapTile* pUploadActiveLightShadowMapTiles = (ShadowMapTile*)m_UploadActiveSpotLightShadowMapTiles[m_BackBufferIndex];
	ConvertShadowMapParams* pUploadActiveLightConvertShadowMapParams = (ConvertShadowMapParams*)m_UploadActiveSpotLightConvertShadowMapParams[m_BackBufferIndex];

	for (decltype(m_NumActiveSpotLights) lightIndex = 0; lightIndex < m_NumActiveSpotLights; ++lightIndex)
	{
		const SpotLightData* pLightData = m_ppActiveSpotLights[lightIndex];
		pUploadActiveLightWorldBounds[lightIndex] = pLightData->m_WorldBounds;
		pUploadActiveLightWorldFrustums[lightIndex] = pLightData->m_WorldFrustum;
		pUploadActiveLightShadowMapTiles[lightIndex] = pLightData->m_ShadowMapTile;
		
		pUploadActiveLightProps[lightIndex].m_Color = pLightData->m_Color;
		pUploadActiveLightProps[lightIndex].m_WorldSpaceDir = pLightData->m_WorldSpaceDir;
		pUploadActiveLightProps[lightIndex].m_LightRange = pLightData->m_LightRange;
		pUploadActiveLightProps[lightIndex].m_CosHalfInnerConeAngle = pLightData->m_CosHalfInnerConeAngle;
		pUploadActiveLightProps[lightIndex].m_CosHalfOuterConeAngle = pLightData->m_CosHalfOuterConeAngle;

		pUploadActiveLightConvertShadowMapParams[lightIndex].m_LightViewNearPlane = pLightData->m_LightViewNearPlane;
		pUploadActiveLightConvertShadowMapParams[lightIndex].m_LightRcpViewClipRange = pLightData->m_LightRcpViewClipRange;
		pUploadActiveLightConvertShadowMapParams[lightIndex].m_LightProjMatrix43 = pLightData->m_LightProjMatrix43;
		pUploadActiveLightConvertShadowMapParams[lightIndex].m_LightProjMatrix33 = pLightData->m_LightProjMatrix33;
		
		Matrix4f shadowMapTileMatrix = CreateShadowMapTileMatrix(pLightData->m_ShadowMapTile);
		pUploadActiveLightViewProjMatrices[lightIndex] = pLightData->m_ViewProjMatrix * shadowMapTileMatrix;
	}

#ifdef ENABLE_PROFILING
	m_pCPUProfiler->EndProfile(profileIndex);
#endif // ENABLE_PROFILING
}

void DXApplication::UpdateDisplayResult(DisplayResult displayResult)
{
	m_DisplayResult = displayResult;
	if (m_DisplayResult == DisplayResult::ShadingResult)
		m_pWindow->SetWindowText(L"Direct lighting");
	else if (m_DisplayResult == DisplayResult::TexCoordBuffer)
		m_pWindow->SetWindowText(L"Texture coordinate buffer");
	else if (m_DisplayResult == DisplayResult::NormalBuffer)
		m_pWindow->SetWindowText(L"Normal buffer");
	else if (m_DisplayResult == DisplayResult::DepthBuffer)
		m_pWindow->SetWindowText(L"Depth buffer");
	else if (m_DisplayResult == DisplayResult::ReprojectedDepthBuffer)
		m_pWindow->SetWindowText(L"Reprojected depth buffer");
	else if (m_DisplayResult == DisplayResult::PointLightTiledShadowMap)
		m_pWindow->SetWindowText(L"Tiled shadow map for point lights");
	else if (m_DisplayResult == DisplayResult::SpotLightTiledShadowMap)
		m_pWindow->SetWindowText(L"Tiled shadow map for spot lights");
	else if (m_DisplayResult == DisplayResult::VoxelRelectance)
		m_pWindow->SetWindowText(L"Voxel reflectance");
	else if (m_DisplayResult == DisplayResult::DepthBufferWithMeshType)
		m_pWindow->SetWindowText(L"Depth buffer with mesh type");
	else
		assert(false);
}

#ifdef DEBUG_RENDER_PASS
void DXApplication::OuputDebugRenderPassResult()
{
	static unsigned frameNumber = 0;
	static unsigned frameNumberForOutput = 10;

	++frameNumber;

	if (frameNumber == frameNumberForOutput)
	{
		struct ShadowMapCommand
		{
			UINT m_DataOffset;
			DrawIndexedArguments m_Args;
		};

		OutputDebugStringA("1.Debug =========================\n");
		using ElementType = ShadowMapCommand;
		
		auto elementFormatter = [](const void* pElementData)
		{
			const ElementType* pElement = (ElementType*)pElementData;

			std::stringstream stringStream;
			stringStream << "m_DataOffset: " << pElement->m_DataOffset
				<< "\nm_IndexCountPerInstance: " << pElement->m_Args.m_IndexCountPerInstance
				<< "\nm_InstanceCount: " << pElement->m_Args.m_InstanceCount
				<< "\nm_StartIndexLocation: " << pElement->m_Args.m_StartIndexLocation
				<< "\nm_BaseVertexLocation: " << pElement->m_Args.m_BaseVertexLocation
				<< "\nm_StartInstanceLocation: " << pElement->m_Args.m_StartInstanceLocation;
			return stringStream.str();
		};
		OutputBufferContent(m_pRenderEnv,
			m_pCreateShadowMapCommandsPass->GetPointLightCommandBuffer(),
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
			sizeof(ElementType),
			elementFormatter);

		OutputDebugStringA("2.Debug =========================\n");
	}
}
#endif // DEBUG_RENDER_PASS