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
#include "RenderPasses/ClearVoxelGridPass.h"
#include "RenderPasses/CreateVoxelGridPass.h"
#include "RenderPasses/CreateRenderGBufferCommandsPass.h"
#include "RenderPasses/CreateRenderShadowMapCommandsPass.h"
#include "RenderPasses/InjectVirtualPointLightsPass.h"
#include "RenderPasses/PropagateLightPass.h"
#include "RenderPasses/RenderGBufferPass.h"
#include "RenderPasses/RenderTiledShadowMapPass.h"
#include "RenderPasses/SetupTiledShadowMapPass.h"
#include "RenderPasses/TiledLightCullingPass.h"
#include "RenderPasses/TiledShadingPass.h"
#include "RenderPasses/ViewFrustumCullingPass.h"
#include "RenderPasses/VisualizeTexturePass.h"
#include "RenderPasses/VisualizeVoxelGridPass.h"
#include "RenderPasses/VisualizeIntensityPass.h"
#include "RenderPasses/DownscaleAndReprojectDepthPass.h"
#include "RenderPasses/FrustumMeshCullingPass.h"
#include "RenderPasses/FillVisibilityBufferPass.h"
#include "Common/Mesh.h"
#include "Common/MeshBatch.h"
#include "Common/MeshRenderResources.h"
#include "Common/LightRenderResources.h"
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
Render frame overview

As a prerequisite, vertex and index data for all meshes should be merged into one vertex and index buffers correspondingly.

1.The render frame starts with detecting visible meshes from camera using compute shader.
Each spawned thread tests mesh AABB against camera frustum.
As a result of the pass, we populate two buffers: the first one containing number of visible meshes and the second one - visible mesh IDs.

2.After, we detect point lights which affect visible meshes from camera using compute shader.
Again, each spawned thread tests point light affecting volume (bounding sphere) against camera frustum.
As a result of the pass, we generate two buffers: the first one containing number of visible point lights and the second one - visible light IDs.

3.We detect spot lights which affect visible meshes from camera using compute shader.
Each spawned thread tests spot light affecting volume against camera frustum.
As a spot light volume, bounding sphere around spot light cone is used.
As a result of the pass, we produce two buffers: the first one containing number of visible spot lights and the second one - visible light IDs.

4.Based on detected visible meshes we generate indirect draw commands to render g-buffer using compute shader.
The indirect draw argument struct looks the following way:

struct DrawIndexedArgs
{
	uint indexCountPerInstance;
	uint instanceCount;
	uint startIndexLocation;
	int  baseVertexLocation;
	uint startInstanceLocation;
};

struct DrawMeshCommand
{
	uint root32BitConstant;
	DrawIndexedArgs drawArgs;
};

root32BitConstant is used to pass material ID associated with a particular mesh.
The rest of the arguments should be self-explanatory.

5.Based on generated indirect draw commands we render G-buffer using execute indirect.

6.After rendering G-buffer, we proceed to tiled light culling using compute shader.
Specifically, we split our screen into tiles 16x16 and test each tile overlap against bounding volumes of visible lights.
As a result, we generate two buffers: the first buffer containing overlapping light IDs and the second one -
light IDs range associated with each tile from the previous buffer.

7.As soon as we have completed rendering G-Buffer, we start preparing indirect draw commands to render shadow maps for light sources using compute shader.
In particular, we spawn as many thread groups as we have detected visible meshes.
Each thread group tests mesh AABB against all visible point and spot light bounds,
outputting overlapping point and spot light IDs for that shadow caster to the buffer.
When the list of overlapping lights has been populated, one of the threads from the thread group will create indirect draw command to draw the shadow caster.

The indirect draw argument struct looks the following way:

struct DrawIndexedArgs
{
	uint indexCountPerInstance;
	uint instanceCount;
	uint startIndexLocation;
	int  baseVertexLocation;
	uint startInstanceLocation;
};

struct DrawMeshCommand
{
	uint root32BitConstant;
	DrawIndexedArgs drawArgs;
};

root32BitConstant is used to transfer light IDs offset for the shadow caster in the resulting buffer.
instanceCount is used to specify how many instances of the shadow caster you would like to draw.
In our case, it will be equal to number of lights affecting that shadow caster.

8.To render shadow maps, Tiled Shadow Map approach from [5] is utilized.
Tiled Shadow Map technique allows to render shadow maps for light sources in one indirect draw call.
As the name suggests, the shadow map is split into tiles, where each tile defines shadow map region for one light.
To render geometry to a particular shadow map tile, additional clip space translation to the vertex position is applied,
after it has been transformed into clip space. Apart from this, we need to ensure that while rendering geometry to a concrete tile,
we do not overwrite data of the neighboring tiles by the geometry expanding beyond the light view frustum.
As proposed in [5], I exploit programmable clipping by specifying custom clipping plane distance (SV_ClipDistance) to clip the geometry outside light view frustum.

*Step 6 mainly involves doing depth test and writing into the depth texture, while step 7 is primarily heavy on arithmetic instructions usage. 
These two steps are very good candidates to be performed in parallel using async compute queue.
In my current implementation, they are executed on the same graphics queue but the intention is to move step 7 to compute queue and execute in parallel.

9.After, I apply tiled shading based on detected lights per each screen tile, using Phong shading mode.

10.To calculate indirect lighting, voxel grid of the scene is generated as described in [1] and [4].
Specifically, we rasterize scene geometry with disabled color writing and depth test.
In the geometry shader we select view matrix (either along X, Y or Z axis) for which triangle has the biggest area
to get the highest number of rasterized pixels for the primitive. The rasterized pixels are output to the buffer,
representing grid cells, from pixel shader. To avoid race conditions between multiple threads writing to the same grid cell,
I take advantage of rasterized ordered view, which provides atomic and ordered access to the resource.
Not to lose details of pixels which are only partially covered by the triangle, I enable conservative rasterization,
which guarantees that pixel shader will be invoked for the pixel even if the primitive does not cover pixel center location.
After voxel grid construction, the voxels are illuminated by each light and converted into virtual point lights.
Finally, the virtual point lights are propagated within the grid to generate indirect illumination,
which is later combined with already computed direct illumination.

Current sample is missing complete shadows and indirect lighting implementation. This work is still ongoing.

Used resources:

[1] OpenGL Insights. Cyril Crassin and Simon Green, Octree-Based Sparse Voxelization Using the GPU Hardware Rasterization
[2] Section on comulative moving average https://en.wikipedia.org/wiki/Moving_average
[3] The Basics of GPU Voxelization https://developer.nvidia.com/content/basics-gpu-voxelization
[4] GPU Pro 4. Hawar Doghramachi, Rasterized Voxel-Based Dynamic Global Illumination
[5] GPU Pro 6. Hawar Doghramachi, Tile-Based Omnidirectional Shadows
*/

/*
To do:
1.Check that inside CreateRenderShadowMapCommands.hlsl we are checking the bound
against MAX_NUM_SPOT_LIGHTS_PER_SHADOW_CASTER and MAX_NUM_POINT_LIGHTS_PER_SHADOW_CASTER
while writing data to the local storage
*/

namespace
{
	struct AppData
	{
		Matrix4f m_ViewProjMatrix;
		Matrix4f m_ViewProjInvMatrix;
		Matrix4f m_PrevViewProjMatrix;
		Matrix4f m_PrevViewProjInvMatrix;
		Vector4f m_CameraWorldFrustumPlanes[Frustum::NumPlanes];
		Vector2u m_ScreenSize;
		Vector2f m_RcpScreenSize;
		Vector2u m_ScreenHalfSize;
		Vector2f m_RcpScreenHalfSize;
		Vector2u m_ScreenQuarterSize;
		Vector2f m_RcpScreenQuarterSize;
		Vector4f m_NotUsed[7];
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
	kNumTilesX = 40,
	kNumTilesY = 40,

	kGridSizeX = 640,
	kGridSizeY = 640,
	kGridSizeZ = 640,
		
	kNumGridCellsX = 16,
	kNumGridCellsY = 16,
	kNumGridCellsZ = 16,

	kMinNumPropagationIterations = 0,
	kMaxNumPropagationIterations = 64,

	kShadowMapTileSize = 512,
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
	Vector4f m_Size;
	Vector4f m_RcpSize;
	Vector4f m_CellSize;
	Vector4f m_RcpCellSize;
	Vector4i m_NumCells;
	Vector4f m_RcpNumCells;
	f32 m_FluxWeight;
	f32 m_BlockerPotentialValue;
	Vector2f m_NotUsed1;
	Vector4f m_NotUsed2[8];
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
	f32 m_NumOccluders;
	Vector3f m_DiffuseColor;
	Vector3f m_WorldSpaceNormal;
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

struct VisualizeTextureData
{
	Matrix4f m_CameraProjMatrix;
	f32 m_CameraNearPlane;
	f32 m_CameraFarPlane;
	f32 m_NotUsed[46];
};

DXApplication::DXApplication(HINSTANCE hApp)
	: Application(hApp, L"Global Illumination", 0, 0, kTileSize * kNumTilesX, kTileSize * kNumTilesY)
	, m_DisplayResult(DisplayResult::Unspecified)
	, m_ShadingMode(TileShadingMode::DirectAndIndirectLight)
	, m_IndirectLightIntensity(IndirectLightIntensity_Accumulated)
	, m_IndirectLightComponent(IndirectLightComponent_Red)
	, m_NumPropagationIterations(kNumGridCellsX)
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
	, m_pPointLightTiledShadowMap(nullptr)
	, m_pDiffuseTexture(nullptr)
	, m_pNormalTexture(nullptr)
	, m_pSpecularTexture(nullptr)
	, m_pAccumLightTexture(nullptr)
	, m_pAccumIntensityRCoeffsTexture(nullptr)
	, m_pAccumIntensityGCoeffsTexture(nullptr)
	, m_pAccumIntensityBCoeffsTexture(nullptr)
	, m_pBackBufferViewport(nullptr)
	, m_pSpotLightTiledShadowMapViewport(nullptr)
	, m_pPointLightTiledShadowMapViewport(nullptr)
	, m_pObjectTransformBuffer(nullptr)
	, m_pCameraTransformBuffer(nullptr)
	, m_pGridBuffer(nullptr)
	, m_pGridConfigDataBuffer(nullptr)
	, m_pViewFrustumMeshCullingDataBuffer(nullptr)
	, m_pViewFrustumSpotLightCullingDataBuffer(nullptr)
	, m_pViewFrustumPointLightCullingDataBuffer(nullptr)
	, m_pTiledLightCullingDataBuffer(nullptr)
	, m_pTiledShadingDataBuffer(nullptr)
	, m_pVisualizeTextureDataBuffer(nullptr)
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
	, m_pPointLightShadowMapDataBuffer(nullptr)
	, m_pPointLightShadowMapTileBuffer(nullptr)
	, m_pPointLightViewTileProjMatrixBuffer(nullptr)
	, m_pRenderEnv(new RenderEnv())
	, m_pFence(nullptr)
	, m_BackBufferIndex(0)
	, m_pRenderGBufferPass(nullptr)
	, m_pTiledLightCullingPass(nullptr)
	, m_pTiledShadingPass(nullptr)
	, m_pTiledDirectLightShadingPass(nullptr)
	, m_pTiledIndirectLightShadingPass(nullptr)
	, m_pClearVoxelGridPass(nullptr)
	, m_pCreateVoxelGridPass(nullptr)
	, m_pInjectVirtualPointLightsPass(nullptr)
	, m_pPropagateLightPass(nullptr)
	, m_pVisualizeVoxelGridDiffusePass(nullptr)
	, m_pVisualizeVoxelGridNormalPass(nullptr)
	, m_pDetectVisibleMeshesPass(nullptr)
	, m_pDetectVisiblePointLightsPass(nullptr)
	, m_pDetectVisibleSpotLightsPass(nullptr)
	, m_pCreateRenderGBufferCommandsPass(nullptr)
	, m_pCreateRenderShadowMapCommandsPass(nullptr)
	, m_pCreateRenderShadowMapCommandsArgumentBuffer(nullptr)
	, m_pRenderSpotLightTiledShadowMapPass(nullptr)
	, m_pRenderPointLightTiledShadowMapPass(nullptr)
	, m_pSetupSpotLightTiledShadowMapPass(nullptr)
	, m_pSetupPointLightTiledShadowMapPass(nullptr)
	, m_pVisualizeAccumLightPass(nullptr)
	, m_pVisualizeDiffuseBufferPass(nullptr)
	, m_pVisualizeSpecularBufferPass(nullptr)
	, m_pVisualizeNormalBufferPass(nullptr)
	, m_pVisualizeDepthBufferPass(nullptr)
	, m_pVisualizeSpotLightTiledShadowMapPass(nullptr)
	, m_pVisualizePointLightTiledShadowMapPass(nullptr)
	, m_pVisualizeIntensityPass(nullptr)
	, m_pMeshRenderResources(nullptr)
	, m_pPointLightRenderResources(nullptr)
	, m_pNumVisiblePointLightsBuffer(nullptr)
	, m_pVisiblePointLightIndexBuffer(nullptr)
	, m_pSpotLightRenderResources(nullptr)
	, m_pNumVisibleSpotLightsBuffer(nullptr)
	, m_pVisibleSpotLightIndexBuffer(nullptr)
	, m_pCamera(nullptr)
	, m_pDownscaleAndReprojectDepthPass(nullptr)
	, m_pFrustumMeshCullingPass(nullptr)
	, m_pFillVisibilityBufferPass(nullptr)
	, m_pAppDataBuffer(nullptr)
{
	for (u8 index = 0; index < kNumBackBuffers; ++index)
		m_FrameCompletionFenceValues[index] = m_pRenderEnv->m_LastSubmissionFenceValue;
	
	for (u8 index = 0; index < 2; ++index)
	{
		m_IntensityRCoeffsTextures[index] = nullptr;
		m_IntensityGCoeffsTextures[index] = nullptr;
		m_IntensityBCoeffsTextures[index] = nullptr;
	}

	UpdateDisplayResult(DisplayResult::ShadingResult);
}

DXApplication::~DXApplication()
{
	SafeDelete(m_pDownscaleAndReprojectDepthPass);
	SafeDelete(m_pFrustumMeshCullingPass);
	SafeDelete(m_pFillVisibilityBufferPass);
	SafeDelete(m_pAppDataBuffer);

	// Old
	for (u8 index = 0; index < 2; ++index)
	{
		SafeDelete(m_IntensityRCoeffsTextures[index]);
		SafeDelete(m_IntensityGCoeffsTextures[index]);
		SafeDelete(m_IntensityBCoeffsTextures[index]);
	}	

	SafeDelete(m_pVisualizeAccumLightPass);
	SafeDelete(m_pVisualizeDiffuseBufferPass);
	SafeDelete(m_pVisualizeSpecularBufferPass);
	SafeDelete(m_pVisualizeNormalBufferPass);
	SafeDelete(m_pVisualizeDepthBufferPass);
	SafeDelete(m_pVisualizeSpotLightTiledShadowMapPass);
	SafeDelete(m_pVisualizePointLightTiledShadowMapPass);
	SafeDelete(m_pVisualizeIntensityPass);
	SafeDelete(m_pSpotLightShadowMapDataBuffer);
	SafeDelete(m_pSpotLightShadowMapTileBuffer);
	SafeDelete(m_pSpotLightViewTileProjMatrixBuffer);
	SafeDelete(m_pPointLightShadowMapDataBuffer);
	SafeDelete(m_pPointLightShadowMapTileBuffer);
	SafeDelete(m_pPointLightViewTileProjMatrixBuffer);
	SafeDelete(m_pSetupSpotLightTiledShadowMapPass);
	SafeDelete(m_pSetupPointLightTiledShadowMapPass);
	SafeDelete(m_pCommandListPool);
	SafeDelete(m_pCamera);
	SafeDelete(m_pPointLightRenderResources);
	SafeDelete(m_pNumVisiblePointLightsBuffer);
	SafeDelete(m_pVisiblePointLightIndexBuffer);
	SafeDelete(m_pSpotLightRenderResources);
	SafeDelete(m_pNumVisibleSpotLightsBuffer);
	SafeDelete(m_pVisibleSpotLightIndexBuffer);
	SafeDelete(m_pMeshRenderResources);
	SafeDelete(m_pShadowCastingPointLightIndexBuffer);
	SafeDelete(m_pNumShadowCastingPointLightsBuffer);
	SafeDelete(m_pDrawPointLightShadowCasterCommandBuffer);
	SafeDelete(m_pNumDrawPointLightShadowCastersBuffer);
	SafeDelete(m_pShadowCastingSpotLightIndexBuffer);
	SafeDelete(m_pNumShadowCastingSpotLightsBuffer);
	SafeDelete(m_pDrawSpotLightShadowCasterCommandBuffer);
	SafeDelete(m_pNumDrawSpotLightShadowCastersBuffer);
	SafeDelete(m_pClearVoxelGridPass);
	SafeDelete(m_pCreateVoxelGridPass);
	SafeDelete(m_pInjectVirtualPointLightsPass);
	SafeDelete(m_pPropagateLightPass);
	SafeDelete(m_pVisualizeVoxelGridDiffusePass);
	SafeDelete(m_pVisualizeVoxelGridNormalPass);
	SafeDelete(m_pTiledLightCullingPass);
	SafeDelete(m_pTiledShadingPass);
	SafeDelete(m_pTiledDirectLightShadingPass);
	SafeDelete(m_pTiledIndirectLightShadingPass);
	SafeDelete(m_pRenderGBufferPass);
	SafeDelete(m_pCreateRenderGBufferCommandsPass);
	SafeDelete(m_pCreateRenderShadowMapCommandsPass);
	SafeDelete(m_pCreateRenderShadowMapCommandsArgumentBuffer);
	SafeDelete(m_pRenderSpotLightTiledShadowMapPass);
	SafeDelete(m_pRenderPointLightTiledShadowMapPass);
	SafeDelete(m_pDetectVisibleMeshesPass);
	SafeDelete(m_pDetectVisiblePointLightsPass);
	SafeDelete(m_pDetectVisibleSpotLightsPass);
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
	SafeDelete(m_pGridConfigDataBuffer);
	SafeDelete(m_pGridBuffer);
	SafeDelete(m_pCameraTransformBuffer);
	SafeDelete(m_pObjectTransformBuffer);
	SafeDelete(m_pDiffuseTexture);
	SafeDelete(m_pNormalTexture);
	SafeDelete(m_pSpecularTexture);
	SafeDelete(m_pAccumLightTexture);
	SafeDelete(m_pAccumIntensityRCoeffsTexture);
	SafeDelete(m_pAccumIntensityGCoeffsTexture);
	SafeDelete(m_pAccumIntensityBCoeffsTexture);
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
	SafeDelete(m_pVisualizeTextureDataBuffer);
}

void DXApplication::OnInit()
{
	const RECT backBufferRect = m_pWindow->GetClientRect();
	const UINT backBufferWidth = backBufferRect.right - backBufferRect.left;
	const UINT backBufferHeight = backBufferRect.bottom - backBufferRect.top;

	InitRenderEnv(backBufferWidth, backBufferHeight);
	
	Scene* pScene = SceneLoader::LoadCornellBox();
	InitScene(pScene, backBufferWidth, backBufferHeight);
	
	InitDownscaleAndReprojectDepthPass();
	InitFrustumMeshCullingPass();
	InitFillVisibilityBufferPass();
	InitConstantBuffers(pScene, backBufferWidth, backBufferHeight);
		
	SafeDelete(pScene);

	/*
	InitDetectVisibleMeshesPass();
	
	if (m_pPointLightRenderResources != nullptr)
		InitDetectVisiblePointLightsPass();
				
	if (m_pSpotLightRenderResources != nullptr)
		InitDetectVisibleSpotLightsPass();
	
	InitCreateRenderGBufferCommandsPass();
	InitRenderGBufferPass(backBufferWidth, backBufferHeight);
	InitTiledLightCullingPass();
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
	
	InitCreateVoxelGridPass();
	InitClearVoxelGridPass();
	InitInjectVirtualPointLightsPass();
	InitPropagateLightPass();
	
	InitTiledShadingPass();

	InitVisualizeVoxelGridDiffusePass();
	InitVisualizeVoxelGridNormalPass();

	InitVisualizeAccumLightPass();
	InitVisualizeDiffuseBufferPass();
	InitVisualizeSpecularBufferPass();
	InitVisualizeNormalBufferPass();
	InitVisualizeDepthBufferPass();
	InitVisualizePointLightTiledShadowMapPass();
	InitVisualizeIntensityPass();

#ifdef DEBUG_RENDER_PASS
	InitDebugRenderPass(pScene);
#endif // DEBUG_RENDER_PASS
	*/
}

void DXApplication::OnUpdate()
{
}

void DXApplication::OnRender()
{
	static const u8 commandListBatchCapacity = 30;
	static CommandList* commandListBatch[commandListBatchCapacity];
	
	u8 commandListBatchSize = 0;
	commandListBatch[commandListBatchSize++] = RecordDownscaleAndReprojectDepthPass();
	commandListBatch[commandListBatchSize++] = RecordClearBackBufferPass();
	commandListBatch[commandListBatchSize++] = RecordFrustumMeshCullingPass();
	commandListBatch[commandListBatchSize++] = RecordFillVisibilityBufferPass();
	commandListBatch[commandListBatchSize++] = RecordPresentResourceBarrierPass();

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
	submissionBatch.emplace_back(RecordDetectVisibleMeshesPass());
	
	if (m_pPointLightRenderResources != nullptr)
		submissionBatch.emplace_back(RecordDetectVisiblePointLightsPass());

	if (m_pSpotLightRenderResources != nullptr)
		submissionBatch.emplace_back(RecordDetectVisibleSpotLightsPass());

	submissionBatch.emplace_back(RecordCreateRenderGBufferCommandsPass());
	submissionBatch.emplace_back(RecordRenderGBufferPass());
	submissionBatch.emplace_back(RecordTiledLightCullingPass());

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
			
	submissionBatch.emplace_back(RecordClearVoxelGridPass());
	submissionBatch.emplace_back(RecordCreateVoxelGridPass());
	submissionBatch.emplace_back(RecordInjectVirtualPointLightsPass());

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
	switch (key)
	{
		case '1':
		{
			UpdateDisplayResult(DisplayResult::DiffuseBuffer);
			break;
		}
		case '2':
		{
			UpdateDisplayResult(DisplayResult::SpecularBuffer);
			break;
		}
		case '3':
		{
			UpdateDisplayResult(DisplayResult::NormalBuffer);
			break;
		}
		case '4':
		{
			UpdateDisplayResult(DisplayResult::DepthBuffer);
			break;
		}
		case '5':
		{
			UpdateDisplayResult(DisplayResult::VoxelGridDiffuse);
			break;
		}
		case '6':
		{
			UpdateDisplayResult(DisplayResult::VoxelGridNormal);
			break;
		}
		case '7':
		{
			UpdateDisplayResult(DisplayResult::SpotLightTiledShadowMap);
			break;
		}
		case '8':
		{
			UpdateDisplayResult(DisplayResult::PointLightTiledShadowMap);
			break;
		}
		case 'd':
		case 'D':
		{
			m_ShadingMode = TileShadingMode::DirectLight;
			UpdateDisplayResult(DisplayResult::ShadingResult);
			break;
		}
		case 'i':
		case 'I':
		{
			m_ShadingMode = TileShadingMode::IndirectLight;
			UpdateDisplayResult(DisplayResult::ShadingResult);
			break;
		}
		case 'f':
		case 'F':
		{
			m_ShadingMode = TileShadingMode::DirectAndIndirectLight;
			UpdateDisplayResult(DisplayResult::ShadingResult);
			break;
		}
		case 'p':
		case 'P':
		{
			m_IndirectLightIntensity = IndirectLightIntensity_Previous;
			UpdateDisplayResult(DisplayResult::IndirectLightIntensityResult);
			break;
		}
		case 'c':
		case 'C':
		{
			m_IndirectLightIntensity = IndirectLightIntensity_Current;
			UpdateDisplayResult(DisplayResult::IndirectLightIntensityResult);
			break;
		}
		case 'a':
		case 'A':
		{
			m_IndirectLightIntensity = IndirectLightIntensity_Accumulated;
			UpdateDisplayResult(DisplayResult::IndirectLightIntensityResult);
			break;
		}
		case 'r':
		case 'R':
		{
			m_IndirectLightComponent = IndirectLightComponent_Red;
			UpdateDisplayResult(DisplayResult::IndirectLightIntensityResult);
			break;
		}
		case 'g':
		case 'G':
		{
			m_IndirectLightComponent = IndirectLightComponent_Green;
			UpdateDisplayResult(DisplayResult::IndirectLightIntensityResult);
			break;
		}
		case 'b':
		case 'B':
		{
			m_IndirectLightComponent = IndirectLightComponent_Blue;
			UpdateDisplayResult(DisplayResult::IndirectLightIntensityResult);
			break;
		}
		case VK_UP:
		{
			if (m_NumPropagationIterations < kMaxNumPropagationIterations)
			{
				++m_NumPropagationIterations;
				if (m_DisplayResult == DisplayResult::IndirectLightIntensityResult)
					UpdateDisplayResult(DisplayResult::IndirectLightIntensityResult);
				else
					UpdateDisplayResult(DisplayResult::ShadingResult);
			}
			break;
		}
		case VK_DOWN:
		{
			if (m_NumPropagationIterations > kMinNumPropagationIterations)
			{
				--m_NumPropagationIterations;
				if (m_DisplayResult == DisplayResult::IndirectLightIntensityResult)
					UpdateDisplayResult(DisplayResult::IndirectLightIntensityResult);
				else
					UpdateDisplayResult(DisplayResult::ShadingResult);
			}
			break;
		}
	}
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

	DescriptorHeapDesc rtvHeapDesc(D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 10, false);
	m_pShaderInvisibleRTVHeap = new DescriptorHeap(m_pDevice, &rtvHeapDesc, L"m_pShaderInvisibleRTVHeap");

	DescriptorHeapDesc shaderVisibleSRVHeapDesc(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 180, true);
	m_pShaderVisibleSRVHeap = new DescriptorHeap(m_pDevice, &shaderVisibleSRVHeapDesc, L"m_pShaderVisibleSRVHeap");

	DescriptorHeapDesc shaderInvisibleSRVHeapDesc(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 80, false);
	m_pShaderInvisibleSRVHeap = new DescriptorHeap(m_pDevice, &shaderInvisibleSRVHeapDesc, L"m_pShaderInvisibleSRVHeap");

	DescriptorHeapDesc dsvHeapDesc(D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 2, false);
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
	m_pRenderEnv->m_pShaderVisibleSRVHeap = m_pShaderVisibleSRVHeap;

	SwapChainDesc swapChainDesc(kNumBackBuffers, m_pWindow->GetHWND(), backBufferWidth, backBufferHeight);
	m_pSwapChain = new SwapChain(&factory, m_pRenderEnv, &swapChainDesc, m_pCommandQueue);
	m_BackBufferIndex = m_pSwapChain->GetCurrentBackBufferIndex();

	DepthStencilValue optimizedClearDepth(1.0f);
	DepthTexture2DDesc depthTexDesc(DXGI_FORMAT_R32_TYPELESS, backBufferWidth, backBufferHeight, true, true);
	m_pDepthTexture = new DepthTexture(m_pRenderEnv, m_pRenderEnv->m_pDefaultHeapProps, &depthTexDesc, D3D12_RESOURCE_STATE_DEPTH_WRITE, &optimizedClearDepth, L"m_pDepthTexture");

	CommandList* pInitCommandList = m_pCommandListPool->Create(L"pInitCommandList");
	pInitCommandList->Begin();
	pInitCommandList->ClearDepthView(m_pDepthTexture->GetDSVHandle(), 1.0f);
	pInitCommandList->End();

	++m_pRenderEnv->m_LastSubmissionFenceValue;
	m_pCommandQueue->ExecuteCommandLists(1, &pInitCommandList, m_pFence, m_pRenderEnv->m_LastSubmissionFenceValue);
	m_pFence->WaitForSignalOnCPU(m_pRenderEnv->m_LastSubmissionFenceValue);
}

void DXApplication::InitScene(Scene* pScene, UINT backBufferWidth, UINT backBufferHeight)
{
	m_pCamera = new Camera(Camera::ProjType_Perspective, 0.1f, 1300.0f, FLOAT(backBufferWidth) / FLOAT(backBufferHeight));
	m_pCamera->GetTransform().SetPosition(Vector3f(0.5f * 549.6f, 0.5f * 548.8f, 700.0f));
	m_pCamera->GetTransform().SetRotation(CreateRotationYQuaternion(PI));
	m_pCamera->SetBackgroundColor(Color::BISQUE);

	if (pScene->GetNumMeshBatches() > 0)
		m_pMeshRenderResources = new MeshRenderResources(m_pRenderEnv, pScene->GetNumMeshBatches(), pScene->GetMeshBatches());

	if (pScene->GetNumPointLights() > 0)
		m_pPointLightRenderResources = new LightRenderResources(m_pRenderEnv, pScene->GetNumPointLights(), pScene->GetPointLights());

	if (pScene->GetNumSpotLights() > 0)
		m_pSpotLightRenderResources = new LightRenderResources(m_pRenderEnv, pScene->GetNumSpotLights(), pScene->GetSpotLights());
		
	//Kolya. Fix me
	//Vector3u shadowMapCommandsArgumentBufferValues(0, 1, 1);
	//StructuredBufferDesc shadowMapCommandsArgumentBufferDesc(1, sizeof(shadowMapCommandsArgumentBufferValues), false, false);
	//m_pCreateRenderShadowMapCommandsArgumentBuffer = new Buffer(m_pRenderEnv, m_pRenderEnv->m_pDefaultHeapProps, &shadowMapCommandsArgumentBufferDesc, D3D12_RESOURCE_STATE_COPY_DEST, L"m_pCreateRenderShadowMapCommandsArgumentBuffer");
	//UploadData(m_pRenderEnv, m_pCreateRenderShadowMapCommandsArgumentBuffer, &shadowMapCommandsArgumentBufferDesc, &shadowMapCommandsArgumentBufferValues, sizeof(shadowMapCommandsArgumentBufferValues));

	//StructuredBufferDesc drawCommandBufferDesc(pMeshBatch->GetNumMeshes(), sizeof(DrawMeshCommand), true, true);
	//m_pDrawMeshCommandBuffer = new Buffer(m_pRenderEnv, m_pRenderEnv->m_pDefaultHeapProps, &drawCommandBufferDesc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, L"m_pDrawMeshCommandBuffer");
}

void DXApplication::InitConstantBuffers(const Scene* pScene, UINT backBufferWidth, UINT backBufferHeight)
{
	AppData appData;
	appData.m_ViewProjMatrix = m_pCamera->GetViewMatrix() * m_pCamera->GetProjMatrix();
	appData.m_ViewProjInvMatrix = Inverse(appData.m_ViewProjMatrix);
	appData.m_PrevViewProjMatrix = appData.m_ViewProjMatrix;
	appData.m_PrevViewProjInvMatrix = Inverse(appData.m_PrevViewProjMatrix);

	const Frustum cameraWorldFrustum = ExtractWorldFrustum(*m_pCamera);
	for (u8 planeIndex = 0; planeIndex < Frustum::NumPlanes; ++planeIndex)
		appData.m_CameraWorldFrustumPlanes[planeIndex] = ToVector4f(cameraWorldFrustum.m_Planes[planeIndex]);

	appData.m_ScreenSize = Vector2u(backBufferWidth, backBufferHeight);
	appData.m_RcpScreenSize = Vector2f(1.0f / f32(appData.m_ScreenSize.m_X), 1.0f / f32(appData.m_ScreenSize.m_Y));
	appData.m_ScreenHalfSize = Vector2u(appData.m_ScreenSize.m_X >> 1, appData.m_ScreenSize.m_Y >> 1);
	appData.m_RcpScreenHalfSize = Vector2f(1.0f / f32(appData.m_ScreenHalfSize.m_X), 1.0f / f32(appData.m_ScreenHalfSize.m_Y));
	appData.m_ScreenQuarterSize = Vector2u(appData.m_ScreenHalfSize.m_X >> 1, appData.m_ScreenHalfSize.m_Y >> 1);
	appData.m_RcpScreenQuarterSize = Vector2f(1.0f / f32(appData.m_ScreenQuarterSize.m_X), 1.0f / f32(appData.m_ScreenQuarterSize.m_Y));

	ConstantBufferDesc appDataBufferDesc(sizeof(appData));
	m_pAppDataBuffer = new Buffer(m_pRenderEnv, m_pRenderEnv->m_pUploadHeapProps, &appDataBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, L"m_pAppDataBuffer");
	m_pAppDataBuffer->Write(&appData, sizeof(appData));

	// Kolya. Fix me
	/*
	ObjectTransform objectTransform;
	objectTransform.m_WorldPositionMatrix = Matrix4f::IDENTITY;
	objectTransform.m_WorldNormalMatrix = Matrix4f::IDENTITY;
	objectTransform.m_WorldViewProjMatrix = mainViewProjMatrix;

	m_pObjectTransformBuffer->Write(&objectTransform, sizeof(objectTransform));

	const BasisAxes mainCameraBasis = ExtractBasisAxes(mainCameraRotation);
	assert(IsNormalized(mainCameraBasis.m_XAxis));
	assert(IsNormalized(mainCameraBasis.m_YAxis));
	assert(IsNormalized(mainCameraBasis.m_ZAxis));

	const Vector3f gridSize(kGridSizeX, kGridSizeY, kGridSizeZ);
	const Vector3f gridRcpSize(Rcp(gridSize));
	const Vector3f gridHalfSize(0.5f * gridSize);
	const Vector3f gridNumCells(kNumGridCellsX, kNumGridCellsY, kNumGridCellsZ);
	const Vector3f gridCellSize(gridSize / gridNumCells);
	const Vector3f gridRcpCellSize(Rcp(gridCellSize));

	// Kolya: Hard-coding grid center for now
	//const Vector3f gridCenter(mainCameraWorldPosition + (0.25f * gridSize.m_Z) * mainCameraBasis.m_ZAxis);
	const Vector3f gridCenter(0.5f * 549.6f, 0.5f * 548.8f, -0.5f * 562.0f);
	const Vector3f gridMinPoint = gridCenter - gridHalfSize;

	GridConfig gridConfig;
	gridConfig.m_WorldSpaceOrigin = Vector4f(gridMinPoint.m_X, gridMinPoint.m_Y, gridMinPoint.m_Z, 0.0f);
	gridConfig.m_Size = Vector4f(gridSize.m_X, gridSize.m_Y, gridSize.m_Z, 0.0f);
	gridConfig.m_RcpSize = Vector4f(gridRcpSize.m_X, gridRcpSize.m_Y, gridRcpSize.m_Z, 0.0f);
	gridConfig.m_CellSize = Vector4f(gridCellSize.m_X, gridCellSize.m_Y, gridCellSize.m_Z, 0.0f);
	gridConfig.m_RcpCellSize = Vector4f(gridRcpCellSize.m_X, gridRcpCellSize.m_Y, gridRcpCellSize.m_Z, 0.0f);
	gridConfig.m_NumCells = Vector4i(kNumGridCellsX, kNumGridCellsY, kNumGridCellsZ, 0);
	gridConfig.m_RcpNumCells = Rcp(Vector4f(kNumGridCellsX, kNumGridCellsY, kNumGridCellsZ, 0));
	gridConfig.m_FluxWeight = 4.0f * PI / 6.0f;//(4.0f * PI / 6.0f) * Rcp(2.0f * gridConfig.m_NumCells.m_X * gridConfig.m_NumCells.m_X);
	gridConfig.m_BlockerPotentialValue = gridConfig.m_FluxWeight;

	m_pGridConfigDataBuffer->Write(&gridConfig, sizeof(gridConfig));

	Camera xAxisCamera(Camera::ProjType_Ortho, 0.0f, gridSize.m_X, gridSize.m_Z / gridSize.m_Y);
	xAxisCamera.SetSizeY(gridSize.m_Y);
	xAxisCamera.GetTransform().SetPosition(gridCenter - gridHalfSize.m_X * mainCameraBasis.m_XAxis);
	xAxisCamera.GetTransform().SetRotation(mainCameraRotation * CreateRotationYQuaternion(PI_DIV_TWO));

	Camera yAxisCamera(Camera::ProjType_Ortho, 0.0f, gridSize.m_Y, gridSize.m_X / gridSize.m_Z);
	yAxisCamera.SetSizeY(gridSize.m_Z);
	yAxisCamera.GetTransform().SetPosition(gridCenter - gridHalfSize.m_Y * mainCameraBasis.m_YAxis);
	yAxisCamera.GetTransform().SetRotation(mainCameraRotation * CreateRotationXQuaternion(-PI_DIV_TWO));

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
	viewFrustumCullingData.m_ViewFrustumPlanes[planeIndex] = mainCameraWorldFrustum.m_Planes[planeIndex];

	viewFrustumCullingData.m_NumObjects = m_pMeshRenderResources->GetNumMeshes();
	m_pViewFrustumMeshCullingDataBuffer->Write(&viewFrustumCullingData, sizeof(viewFrustumCullingData));

	if (m_pViewFrustumPointLightCullingDataBuffer != nullptr)
	{
	viewFrustumCullingData.m_NumObjects = pScene->GetNumPointLights();
	m_pViewFrustumPointLightCullingDataBuffer->Write(&viewFrustumCullingData, sizeof(viewFrustumCullingData));
	}
	if (m_pViewFrustumSpotLightCullingDataBuffer != nullptr)
	{
	viewFrustumCullingData.m_NumObjects = pScene->GetNumSpotLights();
	m_pViewFrustumSpotLightCullingDataBuffer->Write(&viewFrustumCullingData, sizeof(viewFrustumCullingData));
	}

	TiledLightCullingData tiledLightCullingData;
	tiledLightCullingData.m_RcpScreenSize = Rcp(Vector2f((f32)backBufferWidth, (f32)backBufferHeight));
	tiledLightCullingData.m_ViewMatrix = m_pCamera->GetViewMatrix();
	tiledLightCullingData.m_ProjMatrix = m_pCamera->GetProjMatrix();
	tiledLightCullingData.m_ProjInvMatrix = Inverse(m_pCamera->GetProjMatrix());

	m_pTiledLightCullingDataBuffer->Write(&tiledLightCullingData, sizeof(tiledLightCullingData));

	TiledShadingData tiledShadingData;
	tiledShadingData.m_RcpScreenSize = Rcp(Vector2f((f32)backBufferWidth, (f32)backBufferHeight));
	tiledShadingData.m_WorldSpaceCameraPos = mainCameraWorldPosition;
	tiledShadingData.m_ViewProjInvMatrix = Inverse(m_pCamera->GetViewMatrix() * m_pCamera->GetProjMatrix());

	const DirectionalLight* pDirectionalLight = pScene->GetDirectionalLight();
	if (pDirectionalLight != nullptr)
	{
	const BasisAxes lightBasis = ExtractBasisAxes(pDirectionalLight->GetTransform().GetRotation());

	tiledShadingData.m_WorldSpaceLightDir = Normalize(lightBasis.m_ZAxis);
	tiledShadingData.m_LightColor = pDirectionalLight->GetColor();
	}
	m_pTiledShadingDataBuffer->Write(&tiledShadingData, sizeof(tiledShadingData));

	if (m_pPointLightTiledShadowMap != nullptr)
	{
	ShadowMapData shadowMapData;
	shadowMapData.m_TileTexSpaceSize = Vector2f((f32)kShadowMapTileSize, (f32)kShadowMapTileSize) / Vector2f((f32)m_pPointLightTiledShadowMap->GetWidth(), (f32)m_pPointLightTiledShadowMap->GetHeight());

	m_pPointLightShadowMapDataBuffer->Write(&shadowMapData, sizeof(shadowMapData));
	}
	if (m_pSpotLightTiledShadowMap != nullptr)
	{
	ShadowMapData shadowMapData;
	shadowMapData.m_TileTexSpaceSize = Vector2f((f32)kShadowMapTileSize, (f32)kShadowMapTileSize) / Vector2f((f32)m_pSpotLightTiledShadowMap->GetWidth(), (f32)m_pSpotLightTiledShadowMap->GetHeight());

	m_pSpotLightShadowMapDataBuffer->Write(&shadowMapData, sizeof(shadowMapData));
	}

	VisualizeTextureData visualizeTextureData;
	visualizeTextureData.m_CameraProjMatrix = m_pCamera->GetProjMatrix();
	visualizeTextureData.m_CameraNearPlane = m_pCamera->GetNearClipPlane();
	visualizeTextureData.m_CameraFarPlane = m_pCamera->GetFarClipPlane();

	m_pVisualizeTextureDataBuffer->Write(&visualizeTextureData, sizeof(visualizeTextureData));
	*/
}

void DXApplication::InitDownscaleAndReprojectDepthPass()
{
	assert(m_pDownscaleAndReprojectDepthPass == nullptr);

	DownscaleAndReprojectDepthPass::InitParams params;
	params.m_pRenderEnv = m_pRenderEnv;
	params.m_InputResourceStates.m_PrevDepthTextureState = D3D12_RESOURCE_STATE_DEPTH_WRITE;
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
	params.m_pAppDataBuffer = m_pAppDataBuffer;

	m_pDownscaleAndReprojectDepthPass->Record(&params);
	return params.m_pCommandList;
}

CommandList* DXApplication::RecordClearBackBufferPass()
{
	const Vector4f& backgroundColor = m_pCamera->GetBackgroundColor();
	ColorTexture* pRenderTarget = m_pSwapChain->GetBackBuffer(m_BackBufferIndex);

	ResourceBarrier resourceBarriers[] =
	{
		ResourceBarrier(pRenderTarget, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET),
		ResourceBarrier(m_pDepthTexture, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_DEPTH_WRITE)
	};

	CommandList* pCommandList = m_pCommandListPool->Create(L"pClearBackBufferCommandList");
	pCommandList->Begin();
	pCommandList->ResourceBarrier(ARRAYSIZE(resourceBarriers), resourceBarriers);
	pCommandList->ClearRenderTargetView(pRenderTarget->GetRTVHandle(), &backgroundColor.m_X);
	pCommandList->ClearDepthView(m_pDepthTexture->GetDSVHandle(), 1.0f);
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
	params.m_pMeshInstanceRangeBuffer = m_pMeshRenderResources->GetMeshInstanceRangeBuffer();
	params.m_MaxNumMeshes = m_pMeshRenderResources->GetTotalNumMeshes();
	params.m_MaxNumInstances = m_pMeshRenderResources->GetTotalNumInstances();
	params.m_MaxNumInstancesPerMesh = m_pMeshRenderResources->GetMaxNumInstancesPerMesh();
	params.m_InputResourceStates.m_MeshInstanceRangeBufferState = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
	params.m_InputResourceStates.m_InstanceWorldAABBBufferState = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
	params.m_InputResourceStates.m_VisibleInstanceRangeBufferState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
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
	params.m_pAppDataBuffer = m_pAppDataBuffer;

	m_pFrustumMeshCullingPass->Record(&params);
	return params.m_pCommandList;
}

void DXApplication::InitFillVisibilityBufferPass()
{
	assert(m_pFillVisibilityBufferPass == nullptr);
	assert(m_pDownscaleAndReprojectDepthPass != nullptr);
	assert(m_pFrustumMeshCullingPass != nullptr);
	assert(m_pMeshRenderResources);
		
	FillVisibilityBufferPass::InitParams params;
	params.m_pRenderEnv = m_pRenderEnv;

	params.m_InputResourceStates.m_InstanceIndexBufferState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
	params.m_InputResourceStates.m_InstanceWorldMatrixBufferState = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
	params.m_InputResourceStates.m_NumInstancesBufferState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
	params.m_InputResourceStates.m_DepthTextureState = D3D12_RESOURCE_STATE_DEPTH_WRITE;
	params.m_InputResourceStates.m_VisibilityBufferState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
	
	params.m_pInstanceIndexBuffer = m_pFrustumMeshCullingPass->GetVisibleInstanceIndexBuffer();
	params.m_pNumInstancesBuffer = m_pFrustumMeshCullingPass->GetNumVisibleInstancesBuffer();
	params.m_pInstanceWorldMatrixBuffer = m_pMeshRenderResources->GetInstanceWorldMatrixBuffer();
	params.m_pDepthTexture = m_pDownscaleAndReprojectDepthPass->GetReprojectedDepthTexture();
	params.m_ClampVerticesBehindCameraNearPlane = true;
	params.m_MaxNumInstances = m_pMeshRenderResources->GetTotalNumInstances();

	m_pFillVisibilityBufferPass = new FillVisibilityBufferPass(&params);
}

CommandList* DXApplication::RecordFillVisibilityBufferPass()
{
	assert(m_pFillVisibilityBufferPass != nullptr);
	assert(m_pFrustumMeshCullingPass != nullptr);
	
	FillVisibilityBufferPass::RenderParams params;
	params.m_pRenderEnv = m_pRenderEnv;
	params.m_pCommandList = m_pCommandListPool->Create(L"pFillVisibilityBufferCommandList");
	params.m_pAppDataBuffer = m_pAppDataBuffer;
	params.m_pNumInstancesBuffer = m_pFrustumMeshCullingPass->GetNumVisibleInstancesBuffer();

	m_pFillVisibilityBufferPass->Record(&params);
	return params.m_pCommandList;
}

void DXApplication::InitDetectVisibleMeshesPass()
{
	assert(false);
	/*
	FormattedBufferDesc numVisibleMeshesBufferDesc(1, DXGI_FORMAT_R32_UINT, true, true);
	m_pNumVisibleMeshesBuffer = new Buffer(m_pRenderEnv, m_pRenderEnv->m_pDefaultHeapProps, &numVisibleMeshesBufferDesc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, L"m_pNumVisibleMeshesBuffer");

	FormattedBufferDesc visibleMeshIndexBufferDesc(m_pMeshRenderResources->GetNumMeshes(), DXGI_FORMAT_R32_UINT, true, true);
	m_pVisibleMeshIndexBuffer = new Buffer(m_pRenderEnv, m_pRenderEnv->m_pDefaultHeapProps, &visibleMeshIndexBufferDesc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, L"m_pVisibleMeshIndexBuffer");

	ConstantBufferDesc viewFrustumCullingDataBufferDesc(sizeof(ViewFrustumCullingData));
	m_pViewFrustumMeshCullingDataBuffer = new Buffer(m_pRenderEnv, m_pRenderEnv->m_pUploadHeapProps, &viewFrustumCullingDataBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, L"m_pViewFrustumMeshCullingDataBuffer");
	
	Buffer* pMeshBoundsBuffer = m_pMeshRenderResources->GetMeshBoundsBuffer();

	ViewFrustumCullingPass::InitParams initParams;
	initParams.m_pRenderEnv = m_pRenderEnv;
	initParams.m_ObjectBoundsType = ObjectBoundsType_AABB;
	initParams.m_NumObjects = m_pMeshRenderResources->GetNumMeshes();

	m_pDetectVisibleMeshesPass = new ViewFrustumCullingPass(&initParams);

	m_pDetectVisibleMeshesResources->m_RequiredResourceStates.emplace_back(m_pNumVisibleMeshesBuffer, m_pNumVisibleMeshesBuffer->GetWriteState());
	m_pDetectVisibleMeshesResources->m_RequiredResourceStates.emplace_back(m_pVisibleMeshIndexBuffer, m_pVisibleMeshIndexBuffer->GetWriteState());
	m_pDetectVisibleMeshesResources->m_RequiredResourceStates.emplace_back(pMeshBoundsBuffer, pMeshBoundsBuffer->GetReadState());

	m_pDetectVisibleMeshesResources->m_SRVHeapStart = m_pShaderVisibleSRVHeap->Allocate();
	m_pDevice->CopyDescriptor(m_pDetectVisibleMeshesResources->m_SRVHeapStart, m_pNumVisibleMeshesBuffer->GetUAVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), m_pVisibleMeshIndexBuffer->GetUAVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), pMeshBoundsBuffer->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), m_pViewFrustumMeshCullingDataBuffer->GetCBVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	*/
}

void DXApplication::InitDetectVisiblePointLightsPass()
{
	assert(false);
	/*
	assert(m_pPointLightRenderResources != nullptr);
	
	ConstantBufferDesc viewFrustumCullingDataBufferDesc(sizeof(ViewFrustumCullingData));
	m_pViewFrustumPointLightCullingDataBuffer = new Buffer(m_pRenderEnv, m_pRenderEnv->m_pUploadHeapProps, &viewFrustumCullingDataBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, L"m_pViewFrustumPointLightCullingDataBuffer");

	FormattedBufferDesc numVisibleLightsBufferDesc(1, DXGI_FORMAT_R32_UINT, true, true);
	m_pNumVisiblePointLightsBuffer = new Buffer(m_pRenderEnv, m_pRenderEnv->m_pDefaultHeapProps, &numVisibleLightsBufferDesc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, L"m_pNumVisiblePointLightsBuffer");

	FormattedBufferDesc visibleLightIndexBufferDesc(m_pPointLightRenderResources->GetNumLights(), DXGI_FORMAT_R32_UINT, true, true);
	m_pVisiblePointLightIndexBuffer = new Buffer(m_pRenderEnv, m_pRenderEnv->m_pDefaultHeapProps, &visibleLightIndexBufferDesc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, L"m_pVisiblePointLightIndexBuffer");
	
	Buffer* pLightBoundsBuffer = m_pPointLightRenderResources->GetLightBoundsBuffer();

	ViewFrustumCullingPass::InitParams initParams;
	initParams.m_pRenderEnv = m_pRenderEnv;
	initParams.m_ObjectBoundsType = ObjectBoundsType_Sphere;
	initParams.m_NumObjects = m_pPointLightRenderResources->GetNumLights();

	m_pDetectVisiblePointLightsPass = new ViewFrustumCullingPass(&initParams);
	
	m_pDetectVisiblePointLightsResources->m_RequiredResourceStates.emplace_back(m_pNumVisiblePointLightsBuffer, m_pNumVisiblePointLightsBuffer->GetWriteState());
	m_pDetectVisiblePointLightsResources->m_RequiredResourceStates.emplace_back(m_pVisiblePointLightIndexBuffer, m_pVisiblePointLightIndexBuffer->GetWriteState());
	m_pDetectVisiblePointLightsResources->m_RequiredResourceStates.emplace_back(pLightBoundsBuffer, pLightBoundsBuffer->GetReadState());

	m_pDetectVisiblePointLightsResources->m_SRVHeapStart = m_pShaderVisibleSRVHeap->Allocate();
	m_pDevice->CopyDescriptor(m_pDetectVisiblePointLightsResources->m_SRVHeapStart, m_pNumVisiblePointLightsBuffer->GetUAVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), m_pVisiblePointLightIndexBuffer->GetUAVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), pLightBoundsBuffer->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), m_pViewFrustumPointLightCullingDataBuffer->GetCBVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	*/
}

void DXApplication::InitDetectVisibleSpotLightsPass()
{
	assert(false);
	/*
	assert(m_pSpotLightRenderResources != nullptr);

	ConstantBufferDesc viewFrustumCullingDataBufferDesc(sizeof(ViewFrustumCullingData));
	m_pViewFrustumSpotLightCullingDataBuffer = new Buffer(m_pRenderEnv, m_pRenderEnv->m_pUploadHeapProps, &viewFrustumCullingDataBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, L"m_pViewFrustumSpotLightCullingDataBuffer");

	FormattedBufferDesc numVisibleLightsBufferDesc(1, DXGI_FORMAT_R32_UINT, true, true);
	m_pNumVisibleSpotLightsBuffer = new Buffer(m_pRenderEnv, m_pRenderEnv->m_pDefaultHeapProps, &numVisibleLightsBufferDesc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, L"m_pNumVisibleSpotLightsBuffer");

	FormattedBufferDesc visibleLightIndexBufferDesc(m_pSpotLightRenderResources->GetNumLights(), DXGI_FORMAT_R32_UINT, true, true);
	m_pVisibleSpotLightIndexBuffer = new Buffer(m_pRenderEnv, m_pRenderEnv->m_pDefaultHeapProps, &visibleLightIndexBufferDesc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, L"m_pVisibleSpotLightIndexBuffer");

	Buffer* pLightBoundsBuffer = m_pSpotLightRenderResources->GetLightBoundsBuffer();

	ViewFrustumCullingPass::InitParams initParams;
	initParams.m_pRenderEnv = m_pRenderEnv;
	initParams.m_ObjectBoundsType = ObjectBoundsType_Sphere;
	initParams.m_NumObjects = m_pSpotLightRenderResources->GetNumLights();

	m_pDetectVisibleSpotLightsPass = new ViewFrustumCullingPass(&initParams);
	
	m_pDetectVisibleSpotLightsResources->m_RequiredResourceStates.emplace_back(m_pNumVisibleSpotLightsBuffer, m_pNumVisibleSpotLightsBuffer->GetWriteState());
	m_pDetectVisibleSpotLightsResources->m_RequiredResourceStates.emplace_back(m_pVisibleSpotLightIndexBuffer, m_pVisibleSpotLightIndexBuffer->GetWriteState());
	m_pDetectVisibleSpotLightsResources->m_RequiredResourceStates.emplace_back(pLightBoundsBuffer, pLightBoundsBuffer->GetReadState());

	m_pDetectVisibleSpotLightsResources->m_SRVHeapStart = m_pShaderVisibleSRVHeap->Allocate();
	m_pDevice->CopyDescriptor(m_pDetectVisibleSpotLightsResources->m_SRVHeapStart, m_pNumVisibleSpotLightsBuffer->GetUAVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), m_pVisibleSpotLightIndexBuffer->GetUAVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), pLightBoundsBuffer->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), m_pViewFrustumSpotLightCullingDataBuffer->GetCBVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	*/
}

void DXApplication::InitCreateRenderGBufferCommandsPass()
{
	assert(false);
	/*
	Buffer* pMeshDescBuffer = m_pMeshRenderResources->GetMeshDescBuffer();

	CreateRenderGBufferCommandsPass::InitParams initParams;
	initParams.m_pRenderEnv = m_pRenderEnv;
	initParams.m_NumMeshesInBatch = m_pMeshRenderResources->GetNumMeshes();

	m_pCreateRenderGBufferCommandsPass = new CreateRenderGBufferCommandsPass(&initParams);

	m_pCreateRenderGBufferCommandsResources->m_RequiredResourceStates.emplace_back(m_pNumVisibleMeshesBuffer, m_pNumVisibleMeshesBuffer->GetReadState());
	m_pCreateRenderGBufferCommandsResources->m_RequiredResourceStates.emplace_back(m_pVisibleMeshIndexBuffer, m_pVisibleMeshIndexBuffer->GetReadState());
	m_pCreateRenderGBufferCommandsResources->m_RequiredResourceStates.emplace_back(pMeshDescBuffer, pMeshDescBuffer->GetReadState());
	m_pCreateRenderGBufferCommandsResources->m_RequiredResourceStates.emplace_back(m_pDrawMeshCommandBuffer, m_pDrawMeshCommandBuffer->GetWriteState());

	m_pCreateRenderGBufferCommandsResources->m_SRVHeapStart = m_pShaderVisibleSRVHeap->Allocate();
	m_pDevice->CopyDescriptor(m_pCreateRenderGBufferCommandsResources->m_SRVHeapStart, m_pNumVisibleMeshesBuffer->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), m_pVisibleMeshIndexBuffer->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), pMeshDescBuffer->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), m_pDrawMeshCommandBuffer->GetUAVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	*/
}

void DXApplication::InitRenderGBufferPass(UINT backBufferWidth, UINT backBufferHeight)
{
	assert(false);
	/*
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

	ConstantBufferDesc visualizeTextureDataBufferDesc(sizeof(VisualizeTextureData));
	m_pVisualizeTextureDataBuffer = new Buffer(m_pRenderEnv, m_pRenderEnv->m_pUploadHeapProps, &visualizeTextureDataBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, L"m_pVisualizeTextureDataBuffer");

	Buffer* pMaterialBuffer = m_pMeshRenderResources->GetMaterialBuffer();

	RenderGBufferPass::InitParams initParams;
	initParams.m_pRenderEnv = m_pRenderEnv;
	initParams.m_NormalRTVFormat = GetRenderTargetViewFormat(m_pNormalTexture->GetFormat());
	initParams.m_DiffuseRTVFormat = GetRenderTargetViewFormat(m_pDiffuseTexture->GetFormat());
	initParams.m_SpecularRTVFormat = GetRenderTargetViewFormat(m_pSpecularTexture->GetFormat());
	initParams.m_DSVFormat = GetDepthStencilViewFormat(m_pDepthTexture->GetFormat());
	initParams.m_ShaderFlags = 0;
	initParams.m_pMeshBatch = m_pMeshRenderResources;

	m_pRenderGBufferPass = new RenderGBufferPass(&initParams);

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
	*/
}

void DXApplication::InitTiledLightCullingPass()
{
	assert(false);
	/*
	assert((m_pPointLightRenderResources != nullptr) || (m_pSpotLightRenderResources != nullptr));

	ConstantBufferDesc tiledLightCullingDataBufferDesc(sizeof(TiledLightCullingData));
	m_pTiledLightCullingDataBuffer = new Buffer(m_pRenderEnv, m_pRenderEnv->m_pUploadHeapProps, &tiledLightCullingDataBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, L"m_pTiledLightCullingDataBuffer");
	
	if (m_pPointLightRenderResources != nullptr)
	{
		FormattedBufferDesc numLightsPerTileBufferDesc(1, DXGI_FORMAT_R32_UINT, true, true);
		m_pNumPointLightsPerTileBuffer = new Buffer(m_pRenderEnv, m_pRenderEnv->m_pDefaultHeapProps, &numLightsPerTileBufferDesc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, L"m_pNumPointLightsPerTileBuffer");

		FormattedBufferDesc lightIndexPerTileBufferDesc(kNumTilesX * kNumTilesY * m_pPointLightRenderResources->GetNumLights(), DXGI_FORMAT_R32_UINT, true, true);
		m_pPointLightIndexPerTileBuffer = new Buffer(m_pRenderEnv, m_pRenderEnv->m_pDefaultHeapProps, &lightIndexPerTileBufferDesc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, L"m_pPointLightIndexPerTileBuffer");

		StructuredBufferDesc lightRangePerTileBufferDesc(kNumTilesX * kNumTilesY, sizeof(Range), true, true);
		m_pPointLightRangePerTileBuffer = new Buffer(m_pRenderEnv, m_pRenderEnv->m_pDefaultHeapProps, &lightRangePerTileBufferDesc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, L"m_pPointLightRangePerTileBuffer");
	}

	if (m_pSpotLightRenderResources != nullptr)
	{
		FormattedBufferDesc numLightsPerTileBufferDesc(1, DXGI_FORMAT_R32_UINT, true, true);
		m_pNumSpotLightsPerTileBuffer = new Buffer(m_pRenderEnv, m_pRenderEnv->m_pDefaultHeapProps, &numLightsPerTileBufferDesc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, L"m_pNumSpotLightsPerTileBuffer");

		FormattedBufferDesc lightIndexPerTileBufferDesc(kNumTilesX * kNumTilesY * m_pSpotLightRenderResources->GetNumLights(), DXGI_FORMAT_R32_UINT, true, true);
		m_pSpotLightIndexPerTileBuffer = new Buffer(m_pRenderEnv, m_pRenderEnv->m_pDefaultHeapProps, &lightIndexPerTileBufferDesc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, L"m_pSpotLightIndexPerTileBuffer");

		StructuredBufferDesc lightRangePerTileBufferDesc(kNumTilesX * kNumTilesY, sizeof(Range), true, true);
		m_pSpotLightRangePerTileBuffer = new Buffer(m_pRenderEnv, m_pRenderEnv->m_pDefaultHeapProps, &lightRangePerTileBufferDesc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, L"m_pSpotLightRangePerTileBuffer");
	}

	TiledLightCullingPass::InitParams initParams;
	initParams.m_pRenderEnv = m_pRenderEnv;
	initParams.m_TileSize = kTileSize;
	initParams.m_NumTilesX = kNumTilesX;
	initParams.m_NumTilesY = kNumTilesY;
	initParams.m_MaxNumPointLights = (m_pPointLightRenderResources != nullptr) ? m_pPointLightRenderResources->GetNumLights() : 0;
	initParams.m_MaxNumSpotLights = (m_pSpotLightRenderResources != nullptr) ? m_pSpotLightRenderResources->GetNumLights() : 0;

	m_pTiledLightCullingPass = new TiledLightCullingPass(&initParams);
	
	m_pTiledLightCullingResources->m_RequiredResourceStates.emplace_back(m_pDepthTexture, m_pDepthTexture->GetReadState());
	m_pTiledLightCullingResources->m_SRVHeapStart = m_pShaderVisibleSRVHeap->Allocate();

	m_pDevice->CopyDescriptor(m_pTiledLightCullingResources->m_SRVHeapStart, m_pTiledLightCullingDataBuffer->GetCBVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), m_pDepthTexture->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	
	if (m_pPointLightRenderResources != nullptr)
	{
		Buffer* pLightBoundsBuffer = m_pPointLightRenderResources->GetLightBoundsBuffer();

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
	
	if (m_pSpotLightRenderResources != nullptr)
	{
		Buffer* pLightBoundsBuffer = m_pSpotLightRenderResources->GetLightBoundsBuffer();

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
	*/
}

void DXApplication::InitTiledShadingPass()
{
	assert(false);
	/*
	assert((m_pPointLightRenderResources != nullptr) || (m_pSpotLightRenderResources != nullptr));

	ConstantBufferDesc tiledShadingDataBufferDesc(sizeof(TiledShadingData));
	m_pTiledShadingDataBuffer = new Buffer(m_pRenderEnv, m_pRenderEnv->m_pUploadHeapProps, &tiledShadingDataBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, L"m_pTiledShadingDataBuffer");

	TiledShadingPass::InitParams initParams;
	initParams.m_pRenderEnv = m_pRenderEnv;
	initParams.m_ShadingMode = ShadingMode_BlinnPhong;
	initParams.m_TileSize = kTileSize;
	initParams.m_NumTilesX = kNumTilesX;
	initParams.m_NumTilesY = kNumTilesY;
	initParams.m_EnablePointLights = (m_pPointLightRenderResources != nullptr);
	initParams.m_EnableSpotLights = (m_pSpotLightRenderResources != nullptr);
	initParams.m_EnableDirectionalLight = false;
	initParams.m_EnableIndirectLight = true;

	m_pTiledShadingPass = new TiledShadingPass(&initParams);

	initParams.m_EnableIndirectLight = false;
	m_pTiledDirectLightShadingPass = new TiledShadingPass(&initParams);

	initParams.m_EnablePointLights = false;
	initParams.m_EnableSpotLights = false;
	initParams.m_EnableDirectionalLight = false;
	initParams.m_EnableIndirectLight = true;
	
	m_pTiledIndirectLightShadingPass = new TiledShadingPass(&initParams);
				
	m_pTiledShadingResources->m_RequiredResourceStates.emplace_back(m_pAccumLightTexture, m_pAccumLightTexture->GetWriteState());
	m_pTiledShadingResources->m_RequiredResourceStates.emplace_back(m_pDepthTexture, m_pDepthTexture->GetReadState());
	m_pTiledShadingResources->m_RequiredResourceStates.emplace_back(m_pNormalTexture, m_pNormalTexture->GetReadState());
	m_pTiledShadingResources->m_RequiredResourceStates.emplace_back(m_pDiffuseTexture, m_pDiffuseTexture->GetReadState());
	m_pTiledShadingResources->m_RequiredResourceStates.emplace_back(m_pSpecularTexture, m_pSpecularTexture->GetReadState());
	m_pTiledShadingResources->m_RequiredResourceStates.emplace_back(m_pAccumIntensityRCoeffsTexture, m_pAccumIntensityRCoeffsTexture->GetReadState());
	m_pTiledShadingResources->m_RequiredResourceStates.emplace_back(m_pAccumIntensityGCoeffsTexture, m_pAccumIntensityGCoeffsTexture->GetReadState());
	m_pTiledShadingResources->m_RequiredResourceStates.emplace_back(m_pAccumIntensityBCoeffsTexture, m_pAccumIntensityBCoeffsTexture->GetReadState());

	m_pTiledShadingResources->m_SRVHeapStart = m_pShaderVisibleSRVHeap->Allocate();
	m_pDevice->CopyDescriptor(m_pTiledShadingResources->m_SRVHeapStart, m_pAccumLightTexture->GetUAVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), m_pTiledShadingDataBuffer->GetCBVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), m_pGridConfigDataBuffer->GetCBVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), m_pDepthTexture->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), m_pNormalTexture->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), m_pDiffuseTexture->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), m_pSpecularTexture->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), m_pAccumIntensityRCoeffsTexture->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), m_pAccumIntensityGCoeffsTexture->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), m_pAccumIntensityBCoeffsTexture->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	if (m_pPointLightRenderResources != nullptr)
	{
		Buffer* pLightBoundsBuffer = m_pPointLightRenderResources->GetLightBoundsBuffer();
		Buffer* pLightPropsBuffer = m_pPointLightRenderResources->GetLightPropsBuffer();

		m_pTiledShadingResources->m_RequiredResourceStates.emplace_back(pLightBoundsBuffer, pLightBoundsBuffer->GetReadState());
		m_pTiledShadingResources->m_RequiredResourceStates.emplace_back(pLightPropsBuffer, pLightPropsBuffer->GetReadState());
		m_pTiledShadingResources->m_RequiredResourceStates.emplace_back(m_pPointLightIndexPerTileBuffer, m_pPointLightIndexPerTileBuffer->GetReadState());
		m_pTiledShadingResources->m_RequiredResourceStates.emplace_back(m_pPointLightRangePerTileBuffer, m_pPointLightRangePerTileBuffer->GetReadState());

		m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), pLightBoundsBuffer->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), pLightPropsBuffer->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), m_pPointLightIndexPerTileBuffer->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), m_pPointLightRangePerTileBuffer->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	}
	if (m_pSpotLightRenderResources != nullptr)
	{
		Buffer* pLightBoundsBuffer = m_pSpotLightRenderResources->GetLightBoundsBuffer();
		Buffer* pLightPropsBuffer = m_pSpotLightRenderResources->GetLightPropsBuffer();

		m_pTiledShadingResources->m_RequiredResourceStates.emplace_back(pLightBoundsBuffer, pLightBoundsBuffer->GetReadState());
		m_pTiledShadingResources->m_RequiredResourceStates.emplace_back(pLightPropsBuffer, pLightPropsBuffer->GetReadState());
		m_pTiledShadingResources->m_RequiredResourceStates.emplace_back(m_pSpotLightIndexPerTileBuffer, m_pSpotLightIndexPerTileBuffer->GetReadState());
		m_pTiledShadingResources->m_RequiredResourceStates.emplace_back(m_pSpotLightRangePerTileBuffer, m_pSpotLightRangePerTileBuffer->GetReadState());

		m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), pLightBoundsBuffer->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), pLightPropsBuffer->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), m_pSpotLightIndexPerTileBuffer->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), m_pSpotLightRangePerTileBuffer->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	}
	*/
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

void DXApplication::InitClearVoxelGridPass()
{
	assert(false);
	/*
	ClearVoxelGridPass::InitParams initParams;
	initParams.m_pRenderEnv = m_pRenderEnv;
	initParams.m_NumGridCellsX = kNumGridCellsX;
	initParams.m_NumGridCellsY = kNumGridCellsY;
	initParams.m_NumGridCellsZ = kNumGridCellsZ;

	m_pClearVoxelGridPass = new ClearVoxelGridPass(&initParams);

	m_pClearVoxelGridResources->m_RequiredResourceStates.emplace_back(m_pGridBuffer, m_pGridBuffer->GetWriteState());
	m_pClearVoxelGridResources->m_SRVHeapStart = m_pShaderVisibleSRVHeap->Allocate();

	m_pDevice->CopyDescriptor(m_pClearVoxelGridResources->m_SRVHeapStart, m_pGridConfigDataBuffer->GetCBVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), m_pGridBuffer->GetUAVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	*/
}

void DXApplication::InitCreateVoxelGridPass()
{
	assert(false);
	/*
	D3D12_FEATURE_DATA_D3D12_OPTIONS supportedOptions;
	m_pDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &supportedOptions, sizeof(supportedOptions));
	assert(supportedOptions.ConservativeRasterizationTier != D3D12_CONSERVATIVE_RASTERIZATION_TIER_NOT_SUPPORTED);
	assert(supportedOptions.ROVsSupported == TRUE);

	ConstantBufferDesc gridConfigBufferDesc(sizeof(GridConfig));
	m_pGridConfigDataBuffer = new Buffer(m_pRenderEnv, m_pRenderEnv->m_pUploadHeapProps, &gridConfigBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, L"m_pGridConfigBuffer");

	ConstantBufferDesc cameraTransformBufferDesc(sizeof(CameraTransform));
	m_pCameraTransformBuffer = new Buffer(m_pRenderEnv, m_pRenderEnv->m_pUploadHeapProps, &cameraTransformBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, L"m_pCameraTransformBuffer");

	const UINT numGridElements = kNumGridCellsX * kNumGridCellsY * kNumGridCellsZ;
	StructuredBufferDesc gridBufferDesc(numGridElements, sizeof(Voxel), true, true);
	m_pGridBuffer = new Buffer(m_pRenderEnv, m_pRenderEnv->m_pDefaultHeapProps, &gridBufferDesc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, L"m_pGridBuffer");

	Buffer* pMaterialBuffer = m_pMeshRenderResources->GetMaterialBuffer();

	CreateVoxelGridPass::InitParams initParams;
	initParams.m_pRenderEnv = m_pRenderEnv;
	initParams.m_pMeshBatch = m_pMeshRenderResources;

	m_pCreateVoxelGridPass = new CreateVoxelGridPass(&initParams);

	m_pCreateVoxelGridResources->m_RequiredResourceStates.emplace_back(pMaterialBuffer, pMaterialBuffer->GetReadState());
	m_pCreateVoxelGridResources->m_RequiredResourceStates.emplace_back(m_pGridBuffer, m_pGridBuffer->GetWriteState());
	m_pCreateVoxelGridResources->m_RequiredResourceStates.emplace_back(m_pDrawMeshCommandBuffer, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
	m_pCreateVoxelGridResources->m_RequiredResourceStates.emplace_back(m_pNumVisibleMeshesBuffer, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);

	m_pCreateVoxelGridResources->m_SRVHeapStart = m_pShaderVisibleSRVHeap->Allocate();
	m_pDevice->CopyDescriptor(m_pCreateVoxelGridResources->m_SRVHeapStart, m_pObjectTransformBuffer->GetCBVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), m_pCameraTransformBuffer->GetCBVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), m_pGridConfigDataBuffer->GetCBVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), pMaterialBuffer->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), m_pGridBuffer->GetUAVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	*/
}

void DXApplication::InitInjectVirtualPointLightsPass()
{
	assert(false);
	/*
	InjectVirtualPointLightsPass::InitParams initParams;
	initParams.m_pRenderEnv = m_pRenderEnv;
	initParams.m_NumGridCellsX = kNumGridCellsX;
	initParams.m_NumGridCellsY = kNumGridCellsY;
	initParams.m_NumGridCellsZ = kNumGridCellsZ;
	initParams.m_EnablePointLights = (m_pPointLightRenderResources != nullptr);

	m_pInjectVirtualPointLightsPass = new InjectVirtualPointLightsPass(&initParams);

	const FLOAT optimizedClearColor[] = {0.0f, 0.0f, 0.0f, 0.0f};
	ColorTexture3DDesc coeffsTextureDesc(DXGI_FORMAT_R16G16B16A16_FLOAT, kNumGridCellsX, kNumGridCellsY, kNumGridCellsZ, false, true, true);

	for (u8 index = 0; index < 2; ++index)
	{
		m_IntensityRCoeffsTextures[index] = new ColorTexture(m_pRenderEnv, m_pRenderEnv->m_pDefaultHeapProps, &coeffsTextureDesc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, optimizedClearColor, L"m_IntensityRCoeffsTextures");
		m_IntensityGCoeffsTextures[index] = new ColorTexture(m_pRenderEnv, m_pRenderEnv->m_pDefaultHeapProps, &coeffsTextureDesc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, optimizedClearColor, L"m_IntensityGCoeffsTextures");
		m_IntensityBCoeffsTextures[index] = new ColorTexture(m_pRenderEnv, m_pRenderEnv->m_pDefaultHeapProps, &coeffsTextureDesc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, optimizedClearColor, L"m_IntensityBCoeffsTextures");
	}

	m_pAccumIntensityRCoeffsTexture = new ColorTexture(m_pRenderEnv, m_pRenderEnv->m_pDefaultHeapProps, &coeffsTextureDesc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, optimizedClearColor, L"m_pAccumIntensityRCoeffsTexture");
	m_pAccumIntensityGCoeffsTexture = new ColorTexture(m_pRenderEnv, m_pRenderEnv->m_pDefaultHeapProps, &coeffsTextureDesc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, optimizedClearColor, L"m_pAccumIntensityGCoeffsTexture");
	m_pAccumIntensityBCoeffsTexture = new ColorTexture(m_pRenderEnv, m_pRenderEnv->m_pDefaultHeapProps, &coeffsTextureDesc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, optimizedClearColor, L"m_pAccumIntensityBCoeffsTexture");

	ColorTexture* pIntensityRCoeffsTexture = m_IntensityRCoeffsTextures[0];
	ColorTexture* pIntensityGCoeffsTexture = m_IntensityGCoeffsTextures[0];
	ColorTexture* pIntensityBCoeffsTexture = m_IntensityBCoeffsTextures[0];
			
	m_pInjectVirtualPointLightsResources->m_RequiredResourceStates.emplace_back(m_pGridBuffer, m_pGridBuffer->GetReadState());
	m_pInjectVirtualPointLightsResources->m_SRVHeapStart = m_pShaderVisibleSRVHeap->Allocate();

	m_pDevice->CopyDescriptor(m_pInjectVirtualPointLightsResources->m_SRVHeapStart, m_pGridConfigDataBuffer->GetCBVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), m_pGridBuffer->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	if (m_pPointLightRenderResources != nullptr)
	{
		Buffer* pLightBoundsBuffer = m_pPointLightRenderResources->GetLightBoundsBuffer();
		Buffer* pLightPropsBuffer = m_pPointLightRenderResources->GetLightPropsBuffer();

		m_pInjectVirtualPointLightsResources->m_RequiredResourceStates.emplace_back(pLightBoundsBuffer, pLightBoundsBuffer->GetReadState());
		m_pInjectVirtualPointLightsResources->m_RequiredResourceStates.emplace_back(pLightPropsBuffer, pLightPropsBuffer->GetReadState());
		m_pInjectVirtualPointLightsResources->m_RequiredResourceStates.emplace_back(m_pNumVisiblePointLightsBuffer, m_pNumVisiblePointLightsBuffer->GetReadState());
		m_pInjectVirtualPointLightsResources->m_RequiredResourceStates.emplace_back(m_pVisiblePointLightIndexBuffer, m_pVisiblePointLightIndexBuffer->GetReadState());

		m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), pLightBoundsBuffer->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), pLightPropsBuffer->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), m_pNumVisiblePointLightsBuffer->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), m_pVisiblePointLightIndexBuffer->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	}

	m_pInjectVirtualPointLightsResources->m_RequiredResourceStates.emplace_back(pIntensityRCoeffsTexture, pIntensityRCoeffsTexture->GetWriteState());
	m_pInjectVirtualPointLightsResources->m_RequiredResourceStates.emplace_back(pIntensityGCoeffsTexture, pIntensityGCoeffsTexture->GetWriteState());
	m_pInjectVirtualPointLightsResources->m_RequiredResourceStates.emplace_back(pIntensityBCoeffsTexture, pIntensityBCoeffsTexture->GetWriteState());
	
	m_pInjectVirtualPointLightsResources->m_RequiredResourceStates.emplace_back(m_pAccumIntensityRCoeffsTexture, m_pAccumIntensityRCoeffsTexture->GetWriteState());
	m_pInjectVirtualPointLightsResources->m_RequiredResourceStates.emplace_back(m_pAccumIntensityGCoeffsTexture, m_pAccumIntensityGCoeffsTexture->GetWriteState());
	m_pInjectVirtualPointLightsResources->m_RequiredResourceStates.emplace_back(m_pAccumIntensityBCoeffsTexture, m_pAccumIntensityBCoeffsTexture->GetWriteState());

	m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), pIntensityRCoeffsTexture->GetUAVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), pIntensityGCoeffsTexture->GetUAVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), pIntensityBCoeffsTexture->GetUAVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), m_pAccumIntensityRCoeffsTexture->GetUAVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), m_pAccumIntensityGCoeffsTexture->GetUAVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), m_pAccumIntensityBCoeffsTexture->GetUAVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
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

void DXApplication::InitVisualizeVoxelGridDiffusePass()
{
	assert(false);
	/*
	VisualizeVoxelGridPass::InitParams initParams;
	initParams.m_pRenderEnv = m_pRenderEnv;
	initParams.m_RTVFormat = GetRenderTargetViewFormat(m_pSwapChain->GetBackBuffer(m_BackBufferIndex)->GetFormat());
	initParams.m_VoxelDataType = VisualizeVoxelGridPass::VoxelDataType_DiffuseColor;

	m_pVisualizeVoxelGridDiffusePass = new VisualizeVoxelGridPass(&initParams);

	for (u8 index = 0; index < kNumBackBuffers; ++index)
	{
		ColorTexture* pRenderTarget = m_pSwapChain->GetBackBuffer(index);

		m_VisualizeVoxelGridDiffuseResources[index]->m_RequiredResourceStates.emplace_back(pRenderTarget, pRenderTarget->GetWriteState());
		m_VisualizeVoxelGridDiffuseResources[index]->m_RequiredResourceStates.emplace_back(m_pDepthTexture, m_pDepthTexture->GetReadState());

		m_VisualizeVoxelGridDiffuseResources[index]->m_RTVHeapStart = pRenderTarget->GetRTVHandle();
		m_VisualizeVoxelGridDiffuseResources[index]->m_SRVHeapStart = m_pShaderVisibleSRVHeap->Allocate();

		m_pDevice->CopyDescriptor(m_VisualizeVoxelGridDiffuseResources[index]->m_SRVHeapStart, m_pGridConfigDataBuffer->GetCBVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), m_pCameraTransformBuffer->GetCBVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), m_pDepthTexture->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), m_pGridBuffer->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	}
	*/
}

void DXApplication::InitVisualizeVoxelGridNormalPass()
{
	assert(false);
	/*
	VisualizeVoxelGridPass::InitParams initParams;
	initParams.m_pRenderEnv = m_pRenderEnv;
	initParams.m_RTVFormat = GetRenderTargetViewFormat(m_pSwapChain->GetBackBuffer(m_BackBufferIndex)->GetFormat());
	initParams.m_VoxelDataType = VisualizeVoxelGridPass::VoxelDataType_Normal;

	m_pVisualizeVoxelGridNormalPass = new VisualizeVoxelGridPass(&initParams);

	for (u8 index = 0; index < kNumBackBuffers; ++index)
	{
		ColorTexture* pRenderTarget = m_pSwapChain->GetBackBuffer(index);

		m_VisualizeVoxelGridNormalResources[index]->m_RequiredResourceStates.emplace_back(pRenderTarget, pRenderTarget->GetWriteState());
		m_VisualizeVoxelGridNormalResources[index]->m_RequiredResourceStates.emplace_back(m_pDepthTexture, m_pDepthTexture->GetReadState());

		m_VisualizeVoxelGridNormalResources[index]->m_RTVHeapStart = pRenderTarget->GetRTVHandle();
		m_VisualizeVoxelGridNormalResources[index]->m_SRVHeapStart = m_pShaderVisibleSRVHeap->Allocate();

		m_pDevice->CopyDescriptor(m_VisualizeVoxelGridNormalResources[index]->m_SRVHeapStart, m_pGridConfigDataBuffer->GetCBVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), m_pCameraTransformBuffer->GetCBVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), m_pDepthTexture->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), m_pGridBuffer->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	}
	*/
}

void DXApplication::InitVisualizeAccumLightPass()
{
	assert(false);
	/*
	VisualizeTexturePass::InitParams initParams;
	initParams.m_pRenderEnv = m_pRenderEnv;
	initParams.m_RTVFormat = GetRenderTargetViewFormat(m_pSwapChain->GetBackBuffer(m_BackBufferIndex)->GetFormat());
	initParams.m_TextureType = VisualizeTexturePass::TextureType_Other;
	
	m_pVisualizeAccumLightPass = new VisualizeTexturePass(&initParams);

	for (u8 index = 0; index < kNumBackBuffers; ++index)
	{
		ColorTexture* pRenderTarget = m_pSwapChain->GetBackBuffer(index);
		
		m_VisualizeAccumLightResources[index]->m_RequiredResourceStates.emplace_back(pRenderTarget, pRenderTarget->GetWriteState());
		m_VisualizeAccumLightResources[index]->m_RequiredResourceStates.emplace_back(m_pAccumLightTexture, m_pAccumLightTexture->GetReadState());

		m_VisualizeAccumLightResources[index]->m_RTVHeapStart = pRenderTarget->GetRTVHandle();
		m_VisualizeAccumLightResources[index]->m_SRVHeapStart = m_pShaderVisibleSRVHeap->Allocate();
		
		m_pDevice->CopyDescriptor(m_VisualizeAccumLightResources[index]->m_SRVHeapStart, m_pVisualizeTextureDataBuffer->GetCBVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), m_pAccumLightTexture->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	}
	*/
}

void DXApplication::InitVisualizeDiffuseBufferPass()
{
	assert(false);
	/*
	VisualizeTexturePass::InitParams initParams;
	initParams.m_pRenderEnv = m_pRenderEnv;
	initParams.m_RTVFormat = GetRenderTargetViewFormat(m_pSwapChain->GetBackBuffer(m_BackBufferIndex)->GetFormat());
	initParams.m_TextureType = VisualizeTexturePass::TextureType_GBufferDiffuse;

	m_pVisualizeDiffuseBufferPass = new VisualizeTexturePass(&initParams);

	for (u8 index = 0; index < kNumBackBuffers; ++index)
	{
		ColorTexture* pRenderTarget = m_pSwapChain->GetBackBuffer(index);

		m_VisualizeDiffuseBufferResources[index]->m_RequiredResourceStates.emplace_back(pRenderTarget, pRenderTarget->GetWriteState());
		m_VisualizeDiffuseBufferResources[index]->m_RequiredResourceStates.emplace_back(m_pDiffuseTexture, m_pDiffuseTexture->GetReadState());

		m_VisualizeDiffuseBufferResources[index]->m_RTVHeapStart = pRenderTarget->GetRTVHandle();
		m_VisualizeDiffuseBufferResources[index]->m_SRVHeapStart = m_pShaderVisibleSRVHeap->Allocate();

		m_pDevice->CopyDescriptor(m_VisualizeDiffuseBufferResources[index]->m_SRVHeapStart, m_pVisualizeTextureDataBuffer->GetCBVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), m_pDiffuseTexture->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	}
	*/
}

void DXApplication::InitVisualizeSpecularBufferPass()
{
	assert(false);
	/*
	VisualizeTexturePass::InitParams initParams;
	initParams.m_pRenderEnv = m_pRenderEnv;
	initParams.m_RTVFormat = GetRenderTargetViewFormat(m_pSwapChain->GetBackBuffer(m_BackBufferIndex)->GetFormat());
	initParams.m_TextureType = VisualizeTexturePass::TextureType_GBufferSpecular;

	m_pVisualizeSpecularBufferPass = new VisualizeTexturePass(&initParams);

	for (u8 index = 0; index < kNumBackBuffers; ++index)
	{
		ColorTexture* pRenderTarget = m_pSwapChain->GetBackBuffer(index);

		m_VisualizeSpecularBufferResources[index]->m_RequiredResourceStates.emplace_back(pRenderTarget, pRenderTarget->GetWriteState());
		m_VisualizeSpecularBufferResources[index]->m_RequiredResourceStates.emplace_back(m_pSpecularTexture, m_pSpecularTexture->GetReadState());

		m_VisualizeSpecularBufferResources[index]->m_RTVHeapStart = pRenderTarget->GetRTVHandle();
		m_VisualizeSpecularBufferResources[index]->m_SRVHeapStart = m_pShaderVisibleSRVHeap->Allocate();

		m_pDevice->CopyDescriptor(m_VisualizeSpecularBufferResources[index]->m_SRVHeapStart, m_pVisualizeTextureDataBuffer->GetCBVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), m_pSpecularTexture->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	}
	*/
}

void DXApplication::InitVisualizeNormalBufferPass()
{
	assert(false);
	/*
	VisualizeTexturePass::InitParams initParams;
	initParams.m_pRenderEnv = m_pRenderEnv;
	initParams.m_RTVFormat = GetRenderTargetViewFormat(m_pSwapChain->GetBackBuffer(m_BackBufferIndex)->GetFormat());
	initParams.m_TextureType = VisualizeTexturePass::TextureType_GBufferNormal;

	m_pVisualizeNormalBufferPass = new VisualizeTexturePass(&initParams);

	for (u8 index = 0; index < kNumBackBuffers; ++index)
	{
		ColorTexture* pRenderTarget = m_pSwapChain->GetBackBuffer(index);

		m_VisualizeNormalBufferResources[index]->m_RequiredResourceStates.emplace_back(pRenderTarget, pRenderTarget->GetWriteState());
		m_VisualizeNormalBufferResources[index]->m_RequiredResourceStates.emplace_back(m_pNormalTexture, m_pNormalTexture->GetReadState());

		m_VisualizeNormalBufferResources[index]->m_RTVHeapStart = pRenderTarget->GetRTVHandle();
		m_VisualizeNormalBufferResources[index]->m_SRVHeapStart = m_pShaderVisibleSRVHeap->Allocate();

		m_pDevice->CopyDescriptor(m_VisualizeNormalBufferResources[index]->m_SRVHeapStart, m_pVisualizeTextureDataBuffer->GetCBVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), m_pNormalTexture->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	}
	*/
}

void DXApplication::InitVisualizeDepthBufferPass()
{
	assert(false);
	/*
	VisualizeTexturePass::InitParams initParams;
	initParams.m_pRenderEnv = m_pRenderEnv;
	initParams.m_RTVFormat = GetRenderTargetViewFormat(m_pSwapChain->GetBackBuffer(m_BackBufferIndex)->GetFormat());
	initParams.m_TextureType = VisualizeTexturePass::TextureType_Depth;

	m_pVisualizeDepthBufferPass = new VisualizeTexturePass(&initParams);

	for (u8 index = 0; index < kNumBackBuffers; ++index)
	{
		ColorTexture* pRenderTarget = m_pSwapChain->GetBackBuffer(index);

		m_VisualizeDepthBufferResources[index]->m_RequiredResourceStates.emplace_back(pRenderTarget, pRenderTarget->GetWriteState());
		m_VisualizeDepthBufferResources[index]->m_RequiredResourceStates.emplace_back(m_pDepthTexture, m_pDepthTexture->GetReadState());

		m_VisualizeDepthBufferResources[index]->m_RTVHeapStart = pRenderTarget->GetRTVHandle();
		m_VisualizeDepthBufferResources[index]->m_SRVHeapStart = m_pShaderVisibleSRVHeap->Allocate();

		m_pDevice->CopyDescriptor(m_VisualizeDepthBufferResources[index]->m_SRVHeapStart, m_pVisualizeTextureDataBuffer->GetCBVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), m_pDepthTexture->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
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

void DXApplication::InitVisualizeIntensityPass()
{
	assert(false);
	/*
	VisualizeIntensityPass::InitParams initParams;
	initParams.m_pRenderEnv = m_pRenderEnv;
	initParams.m_NumGridCellsX = kNumGridCellsX;
	initParams.m_NumGridCellsY = kNumGridCellsY;
	initParams.m_NumGridCellsZ = kNumGridCellsZ;
	initParams.m_ViewDir = VisualizeIntensityPass::ViewDirection_WorldSpaceZ;
	initParams.m_SliceToVisualize = kNumGridCellsZ / 2;
	initParams.m_NumIntensitySamples = 32;
	initParams.m_RTVFormat = GetRenderTargetViewFormat(m_pSwapChain->GetBackBuffer(m_BackBufferIndex)->GetFormat());

	m_pVisualizeIntensityPass = new VisualizeIntensityPass(&initParams);

	DescriptorHandle srvHeapStart = m_pShaderVisibleSRVHeap->Allocate();
	m_pDevice->CopyDescriptor(srvHeapStart, m_IntensityRCoeffsTextures[0]->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), m_IntensityGCoeffsTextures[0]->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), m_IntensityBCoeffsTextures[0]->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), m_IntensityRCoeffsTextures[1]->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), m_IntensityGCoeffsTextures[1]->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), m_IntensityBCoeffsTextures[1]->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), m_pAccumIntensityRCoeffsTexture->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), m_pAccumIntensityGCoeffsTexture->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), m_pAccumIntensityBCoeffsTexture->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	for (u8 index = 0; index < kNumBackBuffers; ++index)
	{
		ColorTexture* pRenderTarget = m_pSwapChain->GetBackBuffer(index);

		m_VisualizeIntensityResources[index]->m_RequiredResourceStates.emplace_back(pRenderTarget, pRenderTarget->GetWriteState());
		m_VisualizeIntensityResources[index]->m_RequiredResourceStates.emplace_back(m_IntensityRCoeffsTextures[0], m_IntensityRCoeffsTextures[0]->GetReadState());
		m_VisualizeIntensityResources[index]->m_RequiredResourceStates.emplace_back(m_IntensityGCoeffsTextures[0], m_IntensityGCoeffsTextures[0]->GetReadState());
		m_VisualizeIntensityResources[index]->m_RequiredResourceStates.emplace_back(m_IntensityBCoeffsTextures[0], m_IntensityBCoeffsTextures[0]->GetReadState());
		m_VisualizeIntensityResources[index]->m_RequiredResourceStates.emplace_back(m_IntensityRCoeffsTextures[1], m_IntensityRCoeffsTextures[1]->GetReadState());
		m_VisualizeIntensityResources[index]->m_RequiredResourceStates.emplace_back(m_IntensityGCoeffsTextures[1], m_IntensityGCoeffsTextures[1]->GetReadState());
		m_VisualizeIntensityResources[index]->m_RequiredResourceStates.emplace_back(m_IntensityBCoeffsTextures[1], m_IntensityBCoeffsTextures[1]->GetReadState());
		m_VisualizeIntensityResources[index]->m_RequiredResourceStates.emplace_back(m_pAccumIntensityRCoeffsTexture, m_pAccumIntensityRCoeffsTexture->GetReadState());
		m_VisualizeIntensityResources[index]->m_RequiredResourceStates.emplace_back(m_pAccumIntensityGCoeffsTexture, m_pAccumIntensityGCoeffsTexture->GetReadState());
		m_VisualizeIntensityResources[index]->m_RequiredResourceStates.emplace_back(m_pAccumIntensityBCoeffsTexture, m_pAccumIntensityBCoeffsTexture->GetReadState());

		m_VisualizeIntensityResources[index]->m_RTVHeapStart = pRenderTarget->GetRTVHandle();
		m_VisualizeIntensityResources[index]->m_SRVHeapStart = srvHeapStart;
	}
	*/
}

CommandList* DXApplication::RecordDetectVisibleMeshesPass()
{
	assert(false);
	/*
	ViewFrustumCullingPass::RenderParams renderParams;
	renderParams.m_pRenderEnv = m_pRenderEnv;
	renderParams.m_pCommandList = m_pCommandListPool->Create(L"pDetectVisibleMeshesCommandList");
	renderParams.m_pResources = m_pDetectVisibleMeshesResources;
	renderParams.m_pNumVisibleObjectsBuffer = m_pNumVisibleMeshesBuffer;

	m_pDetectVisibleMeshesPass->Record(&renderParams);
	return renderParams.m_pCommandList;
	*/
	return nullptr;
}

CommandList* DXApplication::RecordDetectVisiblePointLightsPass()
{
	assert(false);
	/*
	ViewFrustumCullingPass::RenderParams renderParams;
	renderParams.m_pRenderEnv = m_pRenderEnv;
	renderParams.m_pCommandList = m_pCommandListPool->Create(L"pDetectVisiblePointLightsCommandList");
	renderParams.m_pResources = m_pDetectVisiblePointLightsResources;
	renderParams.m_pNumVisibleObjectsBuffer = m_pNumVisiblePointLightsBuffer;

	m_pDetectVisiblePointLightsPass->Record(&renderParams);
	return renderParams.m_pCommandList;
	*/
	return nullptr;
}

CommandList* DXApplication::RecordDetectVisibleSpotLightsPass()
{
	assert(false);
	/*
	ViewFrustumCullingPass::RenderParams renderParams;
	renderParams.m_pRenderEnv = m_pRenderEnv;
	renderParams.m_pCommandList = m_pCommandListPool->Create(L"pDetectVisibleSpotLightsCommandList");
	renderParams.m_pResources = m_pDetectVisibleSpotLightsResources;
	renderParams.m_pNumVisibleObjectsBuffer = m_pNumVisibleSpotLightsBuffer;

	m_pDetectVisibleSpotLightsPass->Record(&renderParams);
	return renderParams.m_pCommandList;
	*/
	return nullptr;
}

CommandList* DXApplication::RecordCreateRenderGBufferCommandsPass()
{
	assert(false);
	/*
	CreateRenderGBufferCommandsPass::RenderParams renderParams;
	renderParams.m_pRenderEnv = m_pRenderEnv;
	renderParams.m_pCommandList = m_pCommandListPool->Create(L"pCreateRenderGBufferCommandsCommandList");
	renderParams.m_pResources = m_pCreateRenderGBufferCommandsResources;

	m_pCreateRenderGBufferCommandsPass->Record(&renderParams);
	return renderParams.m_pCommandList;
	*/
	return nullptr;
}

CommandList* DXApplication::RecordRenderGBufferPass()
{	
	assert(false);
	/*
	RenderGBufferPass::RenderParams renderParams;
	renderParams.m_pRenderEnv = m_pRenderEnv;
	renderParams.m_pCommandList = m_pCommandListPool->Create(L"pRenderGBufferCommandList");
	renderParams.m_pResources = m_pRenderGBufferResources;
	renderParams.m_pViewport = m_pBackBufferViewport;
	renderParams.m_pMeshBatch = m_pMeshRenderResources;
	renderParams.m_pDrawMeshCommandBuffer = m_pDrawMeshCommandBuffer;
	renderParams.m_pNumDrawMeshesBuffer = m_pNumVisibleMeshesBuffer;

	m_pRenderGBufferPass->Record(&renderParams);
	return renderParams.m_pCommandList;
	*/
	return nullptr;
}

CommandList* DXApplication::RecordTiledLightCullingPass()
{
	assert(false);
	/*
	TiledLightCullingPass::RenderParams renderParams;
	renderParams.m_pRenderEnv = m_pRenderEnv;
	renderParams.m_pCommandList = m_pCommandListPool->Create(L"pTiledLightCullingCommandList");
	renderParams.m_pResources = m_pTiledLightCullingResources;
	renderParams.m_pNumPointLightsPerTileBuffer = m_pNumPointLightsPerTileBuffer;
	renderParams.m_pNumSpotLightsPerTileBuffer = m_pNumSpotLightsPerTileBuffer;

	m_pTiledLightCullingPass->Record(&renderParams);
	return renderParams.m_pCommandList;
	*/
	return nullptr;
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

CommandList* DXApplication::RecordTiledShadingPass()
{
	assert(false);
	/*
	TiledShadingPass::RenderParams renderParams;
	renderParams.m_pRenderEnv = m_pRenderEnv;
	renderParams.m_pCommandList = m_pCommandListPool->Create(L"pTiledShadingCommandList");
	renderParams.m_pResources = m_pTiledShadingResources;
	renderParams.m_pAccumLightTexture = m_pAccumLightTexture;

	TiledShadingPass* pTiledShadingPass = m_pTiledShadingPass;
	if (m_ShadingMode == TileShadingMode::DirectLight)
		pTiledShadingPass = m_pTiledDirectLightShadingPass;
	else if (m_ShadingMode == TileShadingMode::IndirectLight)
		pTiledShadingPass = m_pTiledIndirectLightShadingPass;
	else
		assert(m_ShadingMode == TileShadingMode::DirectAndIndirectLight);

	pTiledShadingPass->Record(&renderParams);
	return renderParams.m_pCommandList;
	*/
	return nullptr;
}

CommandList* DXApplication::RecordClearVoxelGridPass()
{
	assert(false);
	/*
	ClearVoxelGridPass::RenderParams renderParams;
	renderParams.m_pRenderEnv = m_pRenderEnv;
	renderParams.m_pCommandList = m_pCommandListPool->Create(L"pClearGridCommandList");
	renderParams.m_pResources = m_pClearVoxelGridResources;

	m_pClearVoxelGridPass->Record(&renderParams);
	return renderParams.m_pCommandList;
	*/
	return nullptr;
}

CommandList* DXApplication::RecordCreateVoxelGridPass()
{
	assert(false);
	/*
	CreateVoxelGridPass::RenderParams renderParams;
	renderParams.m_pRenderEnv = m_pRenderEnv;
	renderParams.m_pCommandList = m_pCommandListPool->Create(L"pCreateGridCommandList");
	renderParams.m_pResources = m_pCreateVoxelGridResources;
	renderParams.m_pViewport = m_pBackBufferViewport;
	renderParams.m_pMeshBatch = m_pMeshRenderResources;
	renderParams.m_pDrawMeshCommandBuffer = m_pDrawMeshCommandBuffer;
	renderParams.m_pNumDrawMeshesBuffer = m_pNumVisibleMeshesBuffer;

	m_pCreateVoxelGridPass->Record(&renderParams);
	return renderParams.m_pCommandList;
	*/
	return nullptr;
}

CommandList* DXApplication::RecordInjectVirtualPointLightsPass()
{
	assert(false);
	/*
	InjectVirtualPointLightsPass::RenderParams renderParams;
	renderParams.m_pRenderEnv = m_pRenderEnv;
	renderParams.m_pCommandList = m_pCommandListPool->Create(L"pInjectVirtualPointLightsCommandList");
	renderParams.m_pResources = m_pInjectVirtualPointLightsResources;

	m_pInjectVirtualPointLightsPass->Record(&renderParams);
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

CommandList* DXApplication::RecordVisualizeVoxelGridDiffusePass()
{
	assert(false);
	/*
	VisualizeVoxelGridPass::RenderParams renderParams;
	renderParams.m_pRenderEnv = m_pRenderEnv;
	renderParams.m_pCommandList = m_pCommandListPool->Create(L"pVisualizeVoxelGridDiffuseCommandList");
	renderParams.m_pResources = m_VisualizeVoxelGridDiffuseResources[m_BackBufferIndex];
	renderParams.m_pViewport = m_pBackBufferViewport;

	m_pVisualizeVoxelGridDiffusePass->Record(&renderParams);
	return renderParams.m_pCommandList;
	*/
	return nullptr;
}

CommandList* DXApplication::RecordVisualizeVoxelGridNormalPass()
{
	assert(false);
	/*
	VisualizeVoxelGridPass::RenderParams renderParams;
	renderParams.m_pRenderEnv = m_pRenderEnv;
	renderParams.m_pCommandList = m_pCommandListPool->Create(L"pVisualizeVoxelGridNormalCommandList");
	renderParams.m_pResources = m_VisualizeVoxelGridNormalResources[m_BackBufferIndex];
	renderParams.m_pViewport = m_pBackBufferViewport;

	m_pVisualizeVoxelGridNormalPass->Record(&renderParams);
	return renderParams.m_pCommandList;
	*/
	return nullptr;
}

CommandList* DXApplication::RecordVisualizeAccumLightPass()
{
	assert(false);
	/*
	VisualizeTexturePass::RenderParams renderParams;
	renderParams.m_pRenderEnv = m_pRenderEnv;
	renderParams.m_pCommandList = m_pCommandListPool->Create(L"pVisualizeAccumLightCommandList");
	renderParams.m_pResources = m_VisualizeAccumLightResources[m_BackBufferIndex];
	renderParams.m_pViewport = m_pBackBufferViewport;

	m_pVisualizeAccumLightPass->Record(&renderParams);
	return renderParams.m_pCommandList;
	*/
	return nullptr;
}

CommandList* DXApplication::RecordVisualizeDiffuseBufferPass()
{
	assert(false);
	/*
	VisualizeTexturePass::RenderParams renderParams;
	renderParams.m_pRenderEnv = m_pRenderEnv;
	renderParams.m_pCommandList = m_pCommandListPool->Create(L"pVisualizeDiffuseBufferCommandList");
	renderParams.m_pResources = m_VisualizeDiffuseBufferResources[m_BackBufferIndex];
	renderParams.m_pViewport = m_pBackBufferViewport;

	m_pVisualizeDiffuseBufferPass->Record(&renderParams);
	return renderParams.m_pCommandList;
	*/
	return nullptr;
}

CommandList* DXApplication::RecordVisualizeSpecularBufferPass()
{
	assert(false);
	/*
	VisualizeTexturePass::RenderParams renderParams;
	renderParams.m_pRenderEnv = m_pRenderEnv;
	renderParams.m_pCommandList = m_pCommandListPool->Create(L"pVisualizeSpecularBufferCommandList");
	renderParams.m_pResources = m_VisualizeSpecularBufferResources[m_BackBufferIndex];
	renderParams.m_pViewport = m_pBackBufferViewport;

	m_pVisualizeSpecularBufferPass->Record(&renderParams);
	return renderParams.m_pCommandList;
	*/
	return nullptr;
}

CommandList* DXApplication::RecordVisualizeNormalBufferPass()
{
	assert(false);
	/*
	VisualizeTexturePass::RenderParams renderParams;
	renderParams.m_pRenderEnv = m_pRenderEnv;
	renderParams.m_pCommandList = m_pCommandListPool->Create(L"pVisualizeNormalBufferCommandList");
	renderParams.m_pResources = m_VisualizeNormalBufferResources[m_BackBufferIndex];
	renderParams.m_pViewport = m_pBackBufferViewport;

	m_pVisualizeNormalBufferPass->Record(&renderParams);
	return renderParams.m_pCommandList;
	*/
	return nullptr;
}

CommandList* DXApplication::RecordVisualizeDepthBufferPass()
{
	assert(false);
	/*
	VisualizeTexturePass::RenderParams renderParams;
	renderParams.m_pRenderEnv = m_pRenderEnv;
	renderParams.m_pCommandList = m_pCommandListPool->Create(L"pVisualizeDepthBufferCommandList");
	renderParams.m_pResources = m_VisualizeDepthBufferResources[m_BackBufferIndex];
	renderParams.m_pViewport = m_pBackBufferViewport;

	m_pVisualizeDepthBufferPass->Record(&renderParams);
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

CommandList* DXApplication::RecordVisualizeIntensityPass()
{
	assert(false);
	/*
	VisualizeIntensityPass::RenderParams renderParams;
	renderParams.m_pRenderEnv = m_pRenderEnv;
	renderParams.m_pCommandList = m_pCommandListPool->Create(L"pVisualizeVisualizeIntensityCommandList");;
	renderParams.m_pResources = m_VisualizeIntensityResources[m_BackBufferIndex];
	renderParams.m_TextureIndexToVisualize = m_IndirectLightIntensity * 3 + m_IndirectLightComponent;
	renderParams.m_pViewport = m_pBackBufferViewport;

	m_pVisualizeIntensityPass->Record(&renderParams);
	return renderParams.m_pCommandList;
	*/
	return nullptr;
}

CommandList* DXApplication::RecordDisplayResultPass()
{
	if (m_DisplayResult == DisplayResult::ShadingResult)
		return RecordVisualizeAccumLightPass();

	if (m_DisplayResult == DisplayResult::DiffuseBuffer)
		return RecordVisualizeDiffuseBufferPass();

	if (m_DisplayResult == DisplayResult::SpecularBuffer)
		return RecordVisualizeSpecularBufferPass();
		
	if (m_DisplayResult == DisplayResult::NormalBuffer)
		return RecordVisualizeNormalBufferPass();

	if (m_DisplayResult == DisplayResult::DepthBuffer)
		return RecordVisualizeDepthBufferPass();

	if (m_DisplayResult == DisplayResult::SpotLightTiledShadowMap)
		return RecordVisualizeSpotLightTiledShadowMapPass();

	if (m_DisplayResult == DisplayResult::PointLightTiledShadowMap)
		return RecordVisualizePointLightTiledShadowMapPass();
	
	if (m_DisplayResult == DisplayResult::VoxelGridDiffuse)
		return RecordVisualizeVoxelGridDiffusePass();

	if (m_DisplayResult == DisplayResult::VoxelGridNormal)
		return RecordVisualizeVoxelGridNormalPass();

	if (m_DisplayResult == DisplayResult::IndirectLightIntensityResult)
		return RecordVisualizeIntensityPass();
	
	assert(false);
	return nullptr;
}

CommandList* DXApplication::RecordPresentResourceBarrierPass()
{
	ColorTexture* pRenderTarget = m_pSwapChain->GetBackBuffer(m_BackBufferIndex);
	ResourceBarrier resourceBarrier(pRenderTarget, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
		
	CommandList* pCommandList = m_pCommandListPool->Create(L"pPresentResourceBarrierCommandList");
	pCommandList->Begin();
	pCommandList->ResourceBarrier(1, &resourceBarrier);
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
			std::wstringstream stream;
			stream << L"Direct + indirect lighting ";
			stream << "(" << m_NumPropagationIterations << " iterations)";

			m_pWindow->SetWindowText(stream.str().c_str());
		}
		else if (m_ShadingMode == TileShadingMode::DirectLight)
		{
			m_pWindow->SetWindowText(L"Direct lighting");
		}
		else if (m_ShadingMode == TileShadingMode::IndirectLight)
		{
			std::wstringstream stream;
			stream << L"Indirect lighting ";
			stream << "(" << m_NumPropagationIterations << " iterations)";

			m_pWindow->SetWindowText(stream.str().c_str());
		}
		else
		{
			assert(false);
		}
	}
	else if (m_DisplayResult == DisplayResult::IndirectLightIntensityResult)
	{
		const wchar_t* pIntensityBufferStr = nullptr;
		if (m_IndirectLightIntensity == IndirectLightIntensity_Previous)
			pIntensityBufferStr = L"previous intensity";
		else if (m_IndirectLightIntensity == IndirectLightIntensity_Current)
			pIntensityBufferStr = L"current intensity";
		else if (m_IndirectLightIntensity == IndirectLightIntensity_Accumulated)
			pIntensityBufferStr = L"accumulated intensity";
		else
			assert(false);

		const wchar_t* pIntensityComponentStr = nullptr;
		if (m_IndirectLightComponent == IndirectLightComponent_Red)
			pIntensityComponentStr = L"R";
		else if (m_IndirectLightComponent == IndirectLightComponent_Green)
			pIntensityComponentStr = L"G";
		else if (m_IndirectLightComponent == IndirectLightComponent_Blue)
			pIntensityComponentStr = L"B";
		else
			assert(false);

		std::wstringstream stream;
		stream << pIntensityComponentStr << " " << pIntensityBufferStr << L" ";
		stream << "(" << m_NumPropagationIterations << " iterations)";

		m_pWindow->SetWindowText(stream.str().c_str());
	}
	else if (m_DisplayResult == DisplayResult::DiffuseBuffer)
	{
		m_pWindow->SetWindowText(L"Diffuse buffer");
	}
	else if (m_DisplayResult == DisplayResult::SpecularBuffer)
	{
		m_pWindow->SetWindowText(L"Specular buffer");
	}
	else if (m_DisplayResult == DisplayResult::NormalBuffer)
	{
		m_pWindow->SetWindowText(L"Normal buffer");
	}
	else if (m_DisplayResult == DisplayResult::DepthBuffer)
	{
		m_pWindow->SetWindowText(L"Depth buffer");
	}
	else if (m_DisplayResult == DisplayResult::SpotLightTiledShadowMap)
	{
		m_pWindow->SetWindowText(L"Spot light tiled shadow map");
	}
	else if (m_DisplayResult == DisplayResult::PointLightTiledShadowMap)
	{
		m_pWindow->SetWindowText(L"Point light tiled shadow map");
	}
	else if (m_DisplayResult == DisplayResult::VoxelGridDiffuse)
	{
		m_pWindow->SetWindowText(L"Voxel grid diffuse");
	}
	else if (m_DisplayResult == DisplayResult::VoxelGridNormal)
	{
		m_pWindow->SetWindowText(L"Voxel grid normal");
	}
	else
	{
		assert(false);
	}
}

#ifdef DEBUG_RENDER_PASS
void DXApplication::OuputDebugRenderPassResult()
{
	OutputDebugStringA("1.Debug =========================\n");
	
	auto elementFormatter = [](const void* pElementData)
	{
		const u32* pElement = (u32*)pElementData;

		std::stringstream stringStream;
		stringStream << "visible: " << *pElement;
		return stringStream.str();
	};
	OutputBufferContent(m_pRenderEnv,
		m_pFillVisibilityBufferPass->GetVisibilityBuffer(),
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
		sizeof(u32),
		elementFormatter);

	OutputDebugStringA("2.Debug =========================\n");
}
#endif // DEBUG_RENDER_PASS