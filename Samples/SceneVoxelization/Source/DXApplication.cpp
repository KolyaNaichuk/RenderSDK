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
#include "DX/DXResourceList.h"
#include "CommandRecorders/FillGBufferRecorder.h"
#include "CommandRecorders/TiledShadingRecorder.h"
#include "CommandRecorders/ClearVoxelGridRecorder.h"
#include "CommandRecorders/CreateVoxelGridRecorder.h"
#include "CommandRecorders/InjectVPLsIntoVoxelGridRecorder.h"
#include "CommandRecorders/VisualizeVoxelGridRecorder.h"
#include "CommandRecorders/VisualizeMeshRecorder.h"
#include "CommandRecorders/ViewFrustumCullingRecorder.h"
#include "Common/MeshData.h"
#include "Common/MeshDataUtilities.h"
#include "Common/Mesh.h"
#include "Common/Color.h"
#include "Common/Camera.h"
#include "Math/Vector3.h"
#include "Math/Vector4.h"
#include "Math/Matrix4.h"
#include "Math/Transform.h"
#include "Math/BasisAxes.h"
#include "Math/Radian.h"

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

enum RTVHeapHandles
{
	kDiffuseRTVHandle = kBackBufferCount,
	kNormalRTVHandle,
	kSpecularRTVHandle,
	kNumRTVHandles
};

enum DSVHeapHandles
{
	kDSVHandle = 0,
	kNumDSVHandles
};

enum SRVHeapHandles
{
	kNullHandle = kBackBufferCount,
	kObjectTransformCBVHandle,
	kCameraTransformCBVHandle,
	kGridConfigCBVHandle,
	kGridBufferUAVHandle,
	kGridBufferSRVHandle,
	kDepthSRVHandle,
	kDiffuseSRVHandle,
	kNormalSRVHandle,
	kSpecularSRVHandle,
	kAccumLightSRVHandle,
	kAccumLightUAVHandle,
	kNumSRVHandles
};

enum SamplerHandles
{
	kAnisoSamplerHandle = 0,
	kNumSamplerHandles
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
	, m_pDefaultHeapProps(new DXHeapProperties(D3D12_HEAP_TYPE_DEFAULT))
	, m_pUploadHeapProps(new DXHeapProperties(D3D12_HEAP_TYPE_UPLOAD))
	, m_pShaderInvisibleRTVHeap(nullptr)
	, m_pDSVDescriptorHeap(nullptr)
	, m_pShaderInvisibleSRVHeap(nullptr)
	, m_pShaderInvisibleSamplerHeap(nullptr)
	, m_pShaderVisibleSRVHeap(nullptr)
	, m_pShaderVisibleSamplerHeap(nullptr)
	, m_pDepthTexture(nullptr)
	, m_pDiffuseTexture(nullptr)
	, m_pNormalTexture(nullptr)
	, m_pSpecularTexture(nullptr)
	, m_pAccumLightTexture(nullptr)
	, m_pObjectTransformBuffer(nullptr)
	, m_pCameraTransformBuffer(nullptr)
	, m_pGridBuffer(nullptr)
	, m_pGridConfigBuffer(nullptr)
	, m_pAnisoSampler(nullptr)
	, m_pEnv(new DXRenderEnvironment())
	, m_pFence(nullptr)
	, m_BackBufferIndex(0)
	, m_pFillGBufferRecorder(nullptr)
	, m_pTiledShadingRecorder(nullptr)
	, m_pClearVoxelGridRecorder(nullptr)
	, m_pClearVoxelGridResources(nullptr)
	, m_pCreateVoxelGridRecorder(nullptr)
	, m_pInjectVPLsIntoVoxelGridRecorder(nullptr)
	, m_pVisualizeVoxelGridRecorder(nullptr)
	, m_pVisualizeMeshRecorder(nullptr)
	, m_pViewFrustumCullingRecorder(nullptr)
	, m_pViewFrustumCullingResources(nullptr)
	, m_pMesh(nullptr)
	, m_pCamera(nullptr)
{
	std::memset(m_CommandAllocators, 0, sizeof(m_CommandAllocators));
	std::memset(m_FenceValues, 0, sizeof(m_FenceValues));
}

DXApplication::~DXApplication()
{
	for (UINT index = 0; index < kBackBufferCount; ++index)
		SafeDelete(m_CommandAllocators[index]);

	SafeDelete(m_pCamera);
	SafeDelete(m_pMesh);
	SafeDelete(m_pClearVoxelGridRecorder);
	SafeDelete(m_pClearVoxelGridResources);
	SafeDelete(m_pCreateVoxelGridRecorder);
	SafeDelete(m_pInjectVPLsIntoVoxelGridRecorder);
	SafeDelete(m_pVisualizeVoxelGridRecorder);
	SafeDelete(m_pVisualizeMeshRecorder);
	SafeDelete(m_pTiledShadingRecorder);
	SafeDelete(m_pFillGBufferRecorder);
	SafeDelete(m_pViewFrustumCullingRecorder);
	SafeDelete(m_pViewFrustumCullingResources);
	SafeDelete(m_pFence);
	SafeDelete(m_pDefaultHeapProps);
	SafeDelete(m_pUploadHeapProps);
	SafeDelete(m_pDSVDescriptorHeap);
	SafeDelete(m_pShaderInvisibleSRVHeap);
	SafeDelete(m_pShaderInvisibleRTVHeap);
	SafeDelete(m_pShaderInvisibleSamplerHeap);
	SafeDelete(m_pShaderVisibleSRVHeap);
	SafeDelete(m_pShaderVisibleSamplerHeap);
	SafeDelete(m_pEnv);
	SafeDelete(m_pAnisoSampler);
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
	SafeDelete(m_pSwapChain);
	SafeDelete(m_pDevice);
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

	DXDescriptorHeapDesc rtvHeapDesc(D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 16, false);
	m_pShaderInvisibleRTVHeap = new DXDescriptorHeap(m_pDevice, &rtvHeapDesc, L"m_pShaderInvisibleRTVHeap");

	DXDescriptorHeapDesc shaderInvisibleSamplerHeapDesc(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, 4, false);
	m_pShaderInvisibleSamplerHeap = new DXDescriptorHeap(m_pDevice, &shaderInvisibleSamplerHeapDesc, L"m_pShaderInvisibleSamplerHeap");

	DXDescriptorHeapDesc shaderVisibleSamplerHeapDesc(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, 16, true);
	m_pShaderVisibleSamplerHeap = new DXDescriptorHeap(m_pDevice, &shaderVisibleSamplerHeapDesc, L"m_pShaderVisibleSamplerHeap");

	DXDescriptorHeapDesc shaderVisibleSRVHeapDesc(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 16, true);
	m_pShaderVisibleSRVHeap = new DXDescriptorHeap(m_pDevice, &shaderVisibleSRVHeapDesc, L"m_pShaderVisibleSRVHeap");

	DXDescriptorHeapDesc shaderInvisibleSRVHeapDesc(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 16, false);
	m_pShaderInvisibleSRVHeap = new DXDescriptorHeap(m_pDevice, &shaderInvisibleSRVHeapDesc, L"m_pShaderInvisibleSRVHeap");

	DXDescriptorHeapDesc dsvHeapDesc(D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 4, false);
	m_pDSVDescriptorHeap = new DXDescriptorHeap(m_pDevice, &dsvHeapDesc, L"m_pDSVDescriptorHeap");

	m_pEnv->m_pDevice = m_pDevice;
	m_pEnv->m_pDefaultHeapProps = m_pDefaultHeapProps;
	m_pEnv->m_pUploadHeapProps = m_pUploadHeapProps;
	m_pEnv->m_pShaderInvisibleRTVHeap = m_pShaderInvisibleRTVHeap;
	m_pEnv->m_pShaderInvisibleSRVHeap = m_pShaderInvisibleSRVHeap;
	m_pEnv->m_pShaderInvisibleDSVHeap = m_pDSVDescriptorHeap;
	m_pEnv->m_pShaderInvisibleSamplerHeap = m_pShaderInvisibleSamplerHeap;

	const RECT bufferRect = m_pWindow->GetClientRect();
	const UINT bufferWidth = bufferRect.right - bufferRect.left;
	const UINT bufferHeight = bufferRect.bottom - bufferRect.top;
	
	m_pCamera = new Camera(Camera::ProjType_Perspective, 0.1f, 1300.0f, FLOAT(bufferWidth) / FLOAT(bufferHeight));
	m_pCamera->SetClearFlags(Camera::ClearFlag_Color | Camera::ClearFlag_Depth);
	m_pCamera->SetBackgroundColor(Color::GRAY);

	Transform& transform = m_pCamera->GetTransform();
	transform.SetPosition(Vector3f(278.0f, 274.0f, 700.0f));
	transform.SetRotation(CreateRotationYQuaternion(Radian(PI)));

	DXSwapChainDesc swapChainDesc(kBackBufferCount, m_pWindow->GetHWND(), bufferWidth, bufferHeight);
	m_pSwapChain = new DXSwapChain(&factory, m_pEnv, &swapChainDesc, m_pCommandQueue);
	m_BackBufferIndex = m_pSwapChain->GetCurrentBackBufferIndex();
		
	DXSamplerDesc anisoSamplerDesc(DXSamplerDesc::Anisotropic);
	m_pAnisoSampler = new DXSampler(m_pEnv, &anisoSamplerDesc);
	
	DXDepthTexture2DDesc depthTexDesc(DXGI_FORMAT_R32_TYPELESS, bufferWidth, bufferHeight, true, true);
	DXDepthStencilClearValue depthClearValue(GetDepthStencilViewFormat(depthTexDesc.Format));
	m_pDepthTexture = new DXDepthTexture(m_pEnv, m_pEnv->m_pDefaultHeapProps, &depthTexDesc, D3D12_RESOURCE_STATE_DEPTH_WRITE, &depthClearValue, L"m_pDepthTexture");
	
	DXColorTexture2DDesc diffuseTexDesc(DXGI_FORMAT_R10G10B10A2_UNORM, bufferWidth, bufferHeight, true, true, false);
	m_pDiffuseTexture = new DXColorTexture(m_pEnv, m_pEnv->m_pDefaultHeapProps, &diffuseTexDesc, D3D12_RESOURCE_STATE_RENDER_TARGET, L"m_pDiffuseTexture");
	
	DXColorTexture2DDesc normalTexDesc(DXGI_FORMAT_R8G8B8A8_SNORM, bufferWidth, bufferHeight, true, true, false);
	m_pNormalTexture = new DXColorTexture(m_pEnv, m_pEnv->m_pDefaultHeapProps, &normalTexDesc, D3D12_RESOURCE_STATE_RENDER_TARGET, L"m_pNormalTexture");
		
	DXColorTexture2DDesc specularTexDesc(DXGI_FORMAT_R8G8B8A8_UNORM, bufferWidth, bufferHeight, true, true, false);
	m_pSpecularTexture = new DXColorTexture(m_pEnv, m_pEnv->m_pDefaultHeapProps, &specularTexDesc, D3D12_RESOURCE_STATE_RENDER_TARGET, L"m_pSpecularTexture");

	DXColorTexture2DDesc accumLightTexDesc(DXGI_FORMAT_R10G10B10A2_UNORM, bufferWidth, bufferHeight, true, true, true);
	m_pAccumLightTexture = new DXColorTexture(m_pEnv, m_pEnv->m_pDefaultHeapProps, &accumLightTexDesc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, L"m_pAccumLightTexture");
	
	DXConstantBufferDesc objectTransformBufferDesc(sizeof(ObjectTransform));
	m_pObjectTransformBuffer = new DXBuffer(m_pEnv, m_pEnv->m_pUploadHeapProps, &objectTransformBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, L"m_pObjectTransformBuffer");

	DXConstantBufferDesc cameraTransformBufferDesc(sizeof(CameraTransform));
	m_pCameraTransformBuffer = new DXBuffer(m_pEnv, m_pEnv->m_pUploadHeapProps, &cameraTransformBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, L"m_pCameraTransformBuffer");
	
	DXConstantBufferDesc gridConfigBufferDesc(sizeof(GridConfig));
	m_pGridConfigBuffer = new DXBuffer(m_pEnv, m_pEnv->m_pUploadHeapProps, &gridConfigBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, L"m_pGridConfigBuffer");
	
	const UINT numGridElements = kNumGridCellsX * kNumGridCellsY * kNumGridCellsZ;
	DXStructuredBufferDesc gridBufferDesc(numGridElements, sizeof(Voxel), true, true);
	m_pGridBuffer = new DXBuffer(m_pEnv, m_pEnv->m_pDefaultHeapProps, &gridBufferDesc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, L"m_pGridBuffer");
	
	for (UINT index = 0; index < kBackBufferCount; ++index)
		m_CommandAllocators[index] = new DXCommandAllocator(m_pDevice, D3D12_COMMAND_LIST_TYPE_DIRECT, L"m_CommandAllocators");

	m_pFence = new DXFence(m_pDevice, m_FenceValues[m_BackBufferIndex]);
	++m_FenceValues[m_BackBufferIndex];

	m_pCommandList = new DXCommandList(m_pDevice, m_CommandAllocators[m_BackBufferIndex], nullptr, L"m_pCommandList");

	const Vector3f positions[] =
	{
		// Floor
		Vector3f(552.8f,   0.0f,   0.0f),
		Vector3f(  0.0f,   0.0f,   0.0f),
		Vector3f(  0.0f,   0.0f, 559.2f),
		Vector3f(549.6f,   0.0f, 559.2f),

		// Ceiling
		Vector3f(556.0f, 548.8f,   0.0f),
		Vector3f(556.0f, 548.8f, 559.2f),
		Vector3f(  0.0f, 548.8f, 559.2f),
		Vector3f(  0.0f, 548.8f,   0.0f),

		// Back wall
		Vector3f(549.6f,   0.0f, 559.2f),
		Vector3f(  0.0f,   0.0f, 559.2f),
		Vector3f(  0.0f, 548.8f, 559.2f),
		Vector3f(556.0f, 548.8f, 559.2f),

		// Right wall
		Vector3f(  0.0f,   0.0f, 559.2f),
		Vector3f(  0.0f,   0.0f,   0.0f),
		Vector3f(  0.0f, 548.8f,   0.0f),
		Vector3f(  0.0f, 548.8f, 559.2f),

		// Left wall
		Vector3f(552.8f,   0.0f,   0.0f),
		Vector3f(549.6f,   0.0f, 559.2f),
		Vector3f(556.0f, 548.8f, 559.2f),
		Vector3f(556.0f, 548.8f,   0.0f),

		// Short block
		Vector3f(130.0f, 165.0f,  65.0f),
		Vector3f( 82.0f, 165.0f, 225.0f),
		Vector3f(240.0f, 165.0f, 272.0f),
		Vector3f(290.0f, 165.0f, 114.0f),

		Vector3f(290.0f,   0.0f, 114.0f),
		Vector3f(290.0f, 165.0f, 114.0f),
		Vector3f(240.0f, 165.0f, 272.0f),
		Vector3f(240.0f,   0.0f, 272.0f),

		Vector3f(130.0f,   0.0f,  65.0f),
		Vector3f(130.0f, 165.0f,  65.0f),
		Vector3f(290.0f, 165.0f, 114.0f),
		Vector3f(290.0f,   0.0f, 114.0f),

		Vector3f( 82.0f,   0.0f, 225.0f),
		Vector3f( 82.0f, 165.0f, 225.0f),
		Vector3f(130.0f, 165.0f,  65.0f),
		Vector3f(130.0f,   0.0f,  65.0f),

		Vector3f(240.0f,   0.0f, 272.0f),
		Vector3f(240.0f, 165.0f, 272.0f),
		Vector3f( 82.0f, 165.0f, 225.0f),
		Vector3f( 82.0f,   0.0f, 225.0f),

		// Tall block
		Vector3f(423.0f, 330.0f, 247.0f),
		Vector3f(265.0f, 330.0f, 296.0f),
		Vector3f(314.0f, 330.0f, 456.0f),
		Vector3f(472.0f, 330.0f, 406.0f),

		Vector3f(423.0f,   0.0f, 247.0f),
		Vector3f(423.0f, 330.0f, 247.0f),
		Vector3f(472.0f, 330.0f, 406.0f),
		Vector3f(472.0f,   0.0f, 406.0f),

		Vector3f(472.0f,   0.0f, 406.0f),
		Vector3f(472.0f, 330.0f, 406.0f),
		Vector3f(314.0f, 330.0f, 456.0f),
		Vector3f(314.0f,   0.0f, 456.0f),

		Vector3f(314.0f,   0.0f, 456.0f),
		Vector3f(314.0f, 330.0f, 456.0f),
		Vector3f(265.0f, 330.0f, 296.0f),
		Vector3f(265.0f,   0.0f, 296.0f),

		Vector3f(265.0f,   0.0f, 296.0f),
		Vector3f(265.0f, 330.0f, 296.0f),
		Vector3f(423.0f, 330.0f, 247.0f),
		Vector3f(423.0f,   0.0f, 247.0f)
	};
		
	const Vector4f colors[] =
	{
		// Floor
		Color::BISQUE, Color::BISQUE, Color::BISQUE, Color::BISQUE,
		//Color::WHITE, Color::WHITE, Color::WHITE, Color::WHITE,

		// Ceiling
		Color::BLANCHED_ALMOND, Color::BLANCHED_ALMOND, Color::BLANCHED_ALMOND, Color::BLANCHED_ALMOND,
		//Color::WHITE, Color::WHITE, Color::WHITE, Color::WHITE,

		// Back wall
		Color::BLUE_VIOLET, Color::BLUE_VIOLET, Color::BLUE_VIOLET, Color::BLUE_VIOLET,
		//Color::WHITE, Color::WHITE, Color::WHITE, Color::WHITE,
				
		// Right wall
		Color::GREEN, Color::GREEN, Color::GREEN, Color::GREEN,	

		// Left wall
		Color::RED, Color::RED, Color::RED, Color::RED,

		// Short block
		Color::BLUE, Color::BLUE, Color::BLUE, Color::BLUE,
		Color::BLUE, Color::BLUE, Color::BLUE, Color::BLUE,
		Color::BLUE, Color::BLUE, Color::BLUE, Color::BLUE,
		Color::BLUE, Color::BLUE, Color::BLUE, Color::BLUE,
		Color::BLUE, Color::BLUE, Color::BLUE, Color::BLUE,
		/*
		Color::WHITE, Color::WHITE, Color::WHITE, Color::WHITE,
		Color::WHITE, Color::WHITE, Color::WHITE, Color::WHITE,
		Color::WHITE, Color::WHITE, Color::WHITE, Color::WHITE,
		Color::WHITE, Color::WHITE, Color::WHITE, Color::WHITE, 
		Color::WHITE, Color::WHITE, Color::WHITE, Color::WHITE,
		*/

		// Tall block
		Color::GOLD, Color::GOLD, Color::GOLD, Color::GOLD,
		Color::GOLD, Color::GOLD, Color::GOLD, Color::GOLD,
		Color::GOLD, Color::GOLD, Color::GOLD, Color::GOLD,
		Color::GOLD, Color::GOLD, Color::GOLD, Color::GOLD,
		Color::GOLD, Color::GOLD, Color::GOLD, Color::GOLD,
		/*
		Color::WHITE, Color::WHITE, Color::WHITE, Color::WHITE,
		Color::WHITE, Color::WHITE, Color::WHITE, Color::WHITE,
		Color::WHITE, Color::WHITE, Color::WHITE, Color::WHITE,
		Color::WHITE, Color::WHITE, Color::WHITE, Color::WHITE,
		Color::WHITE, Color::WHITE, Color::WHITE, Color::WHITE
		*/
	};

	const u16 indices[] =
	{
		// Floor
		0, 1, 2, 2, 3, 0,

		// Ceiling
		4, 5, 6, 6, 7, 4,

		// Back wall
		8, 9, 10, 10, 11, 8,

		// Right wall
		12, 13, 14, 14, 15, 12,

		// Left wall
		16, 17, 18, 18, 19, 16,

		// Short block
		20, 21, 22, 22, 23, 20,
		24, 25, 26, 26, 27, 24,
		28, 29, 30, 30, 31, 28,
		32, 33, 34, 34, 35, 32,
		36, 37, 38, 38, 39, 36,

		// Tall block
		40, 41, 42, 42, 43, 40,
		44, 45, 46, 46, 47, 44,
		48, 49, 50, 50, 51, 48,
		52, 53, 54, 54, 55, 52,
		56, 57, 58, 58, 59, 56
	};

	MeshData meshData;
	meshData.SetVertexData(ARRAYSIZE(positions), positions, colors);
	meshData.SetIndexData(ARRAYSIZE(indices), indices);
	meshData.ComputeNormals();
	
	SubMeshData subMeshData(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE, 0, meshData.GetNumVertices(), 0, meshData.GetNumIndices());
	meshData.SetSubMeshData(1, &subMeshData);

	ConvertMeshData(&meshData, ConvertionFlag_LeftHandedCoordSystem);
	m_pMesh = new Mesh(m_pEnv, &meshData);
	
	m_pMesh->RecordDataForUpload(m_pCommandList);
	m_pCommandList->Close();

	m_pCommandQueue->ExecuteCommandLists(m_pEnv, 1, &m_pCommandList, nullptr);
	
	WaitForGPU();
	m_pMesh->RemoveDataForUpload();

	ViewFrustumCullingRecorder::InitParams viewFrustumCullingParams;
	viewFrustumCullingParams.m_pEnv = m_pEnv;
	viewFrustumCullingParams.m_NumMeshes = meshData.GetNumSubMeshes();

	m_pViewFrustumCullingRecorder = new ViewFrustumCullingRecorder(&viewFrustumCullingParams);
		
	FillGBufferRecorder::InitParams fillGBufferParams;
	fillGBufferParams.m_pEnv = m_pEnv;
	fillGBufferParams.m_DiffuseRTVFormat = GetRenderTargetViewFormat(m_pDiffuseTexture->GetFormat());
	fillGBufferParams.m_NormalRTVFormat = GetRenderTargetViewFormat(m_pNormalTexture->GetFormat());
	fillGBufferParams.m_SpecularRTVFormat = GetRenderTargetViewFormat(m_pSpecularTexture->GetFormat());
	fillGBufferParams.m_DSVFormat = GetDepthStencilViewFormat(m_pDepthTexture->GetFormat());
	fillGBufferParams.m_VertexElementFlags = m_pMesh->GetVertexElementFlags();
	fillGBufferParams.m_MaterialElementFlags = 0;

	//m_pFillGBufferRecorder = new FillGBufferRecorder(&fillGBufferParams);

	TiledShadingRecorder::InitParams tiledShadingParams;
	tiledShadingParams.m_pEnv = m_pEnv;
	tiledShadingParams.m_ShadingMode = ShadingMode_Phong;
	tiledShadingParams.m_NumTilesX = kNumTilesX;
	tiledShadingParams.m_NumTilesY = kNumTilesY;
	tiledShadingParams.m_NumPointLights = 1;
	tiledShadingParams.m_NumSpotLights = 0;
	tiledShadingParams.m_UseDirectLight = false;

	//m_pTiledShadingRecorder = new TiledShadingRecorder(&tiledShadingParams);
	
	ClearVoxelGridInitParams clearGridParams;
	clearGridParams.m_pEnv = m_pEnv;
	clearGridParams.m_NumGridCellsX = kNumGridCellsX;
	clearGridParams.m_NumGridCellsY = kNumGridCellsY;
	clearGridParams.m_NumGridCellsZ = kNumGridCellsZ;

	m_pClearVoxelGridRecorder = new ClearVoxelGridRecorder(&clearGridParams);

	m_pClearVoxelGridResources = new DXBindingResourceList;
	m_pClearVoxelGridResources->m_ResourceTransitions.emplace_back(m_pGridBuffer, m_pGridBuffer->GetWriteState());
	m_pClearVoxelGridResources->m_SRVHeapStart = m_pShaderVisibleSRVHeap->Allocate();
	m_pDevice->CopyDescriptor(m_pClearVoxelGridResources->m_SRVHeapStart, m_pGridConfigBuffer->GetCBVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_pDevice->CopyDescriptor(m_pShaderVisibleSRVHeap->Allocate(), m_pGridBuffer->GetUAVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	CreateVoxelGridInitParams createGridParams;
	createGridParams.m_pEnv = m_pEnv;
	createGridParams.m_VertexElementFlags = m_pMesh->GetVertexElementFlags();

	//m_pCreateVoxelGridRecorder = new CreateVoxelGridRecorder(&createGridParams);

	InjectVPLsIntoVoxelGridRecorder::InitPrams injectVPLsParams;
	injectVPLsParams.m_pEnv = m_pEnv;
	injectVPLsParams.m_NumGridCellsX = kNumGridCellsX;
	injectVPLsParams.m_NumGridCellsY = kNumGridCellsY;
	injectVPLsParams.m_NumGridCellsZ = kNumGridCellsZ;

	//m_pInjectVPLsIntoVoxelGridRecorder = new InjectVPLsIntoVoxelGridRecorder(&injectVPLsParams);

	VisualizeVoxelGridInitParams visualizeGridParams;
	visualizeGridParams.m_pEnv = m_pEnv;
	visualizeGridParams.m_RTVFormat = DXGI_FORMAT_R8G8B8A8_UNORM;

	//m_pVisualizeVoxelGridRecorder = new VisualizeVoxelGridRecorder(&visualizeGridParams);

	VisualizeMeshInitParams visualizeMeshParams;
	visualizeMeshParams.m_pEnv = m_pEnv;
	visualizeMeshParams.m_MeshDataElement = MeshDataElement_Normal;
	visualizeMeshParams.m_VertexElementFlags = m_pMesh->GetVertexElementFlags();
	visualizeMeshParams.m_RTVFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	visualizeMeshParams.m_DSVFormat = DXGI_FORMAT_D32_FLOAT;
		
	//m_pVisualizeMeshRecorder = new VisualizeMeshRecorder(&visualizeMeshParams);

	// Kolya: Should be moved to OnUpdate
	// Temporarily moved constant buffer update here to overcome frame capture crash on AMD R9 290
	const Vector3f& mainCameraPos = m_pCamera->GetTransform().GetPosition();
	const Quaternion& mainCameraRotation = m_pCamera->GetTransform().GetRotation();
	const Matrix4f mainViewProjMatrix = m_pCamera->GetViewMatrix() * m_pCamera->GetProjMatrix();

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

	m_pGridConfigBuffer->Write(&gridConfig, sizeof(GridConfig));

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

		if (clearFlags & Camera::ClearFlag_Color)
		{
			if (pRenderTarget->GetState() != pRenderTarget->GetWriteState())
				m_pCommandList->TransitionBarrier(pRenderTarget, pRenderTarget->GetState(), pRenderTarget->GetWriteState());

			const Vector4f& clearColor = m_pCamera->GetBackgroundColor();
			m_pCommandList->ClearRenderTargetView(rtvHandle, &clearColor.m_X);
		}
		if (clearFlags & Camera::ClearFlag_Depth)
		{
			if (m_pDepthTexture->GetState() != m_pDepthTexture->GetWriteState())
				m_pCommandList->TransitionBarrier(m_pDepthTexture, m_pDepthTexture->GetState(), m_pDepthTexture->GetWriteState());

			m_pCommandList->ClearDepthView(dsvHandle);
		}

		m_pCommandList->Close();
		m_pCommandQueue->ExecuteCommandLists(m_pEnv, 1, &m_pCommandList, pCommandAllocator);
		WaitForGPU();
	}

	GBuffer gBuffer;
	gBuffer.m_pDiffuseTexture = m_pDiffuseTexture;
	gBuffer.m_pNormalTexture = m_pNormalTexture;
	gBuffer.m_pSpecularTexture = m_pSpecularTexture;
	gBuffer.m_pAccumLightTexture = m_pAccumLightTexture;
	gBuffer.m_pDepthTexture = m_pDepthTexture;
	
	D3D12_RESOURCE_STATES renderTargetEndState = D3D12_RESOURCE_STATE_PRESENT;

	FillGBufferRecorder::RenderPassParams fillGBufferParams;
	fillGBufferParams.m_pEnv = m_pEnv;
	fillGBufferParams.m_pCommandList = m_pCommandList;
	fillGBufferParams.m_pCommandAllocator = pCommandAllocator;
	fillGBufferParams.m_pGBuffer = &gBuffer;
	fillGBufferParams.m_pMesh = m_pMesh;
	//fillGBufferParams.m_pMaterial;
	fillGBufferParams.m_pAnisoSampler = m_pAnisoSampler;
	
//	m_pFillGBufferRecorder->Record(&fillGBufferParams);
//	m_pCommandQueue->ExecuteCommandLists(m_pEnv, 1, &m_pCommandList);
//	WaitForGPU();

	/*
	VisualizeMeshRecordParams visualizeMeshParams;
	visualizeMeshParams.m_pEnv = m_pEnv;
	visualizeMeshParams.m_pMesh = m_pMesh;
	visualizeMeshParams.m_pCommandList = m_pCommandList;
	visualizeMeshParams.m_pCommandAllocator = pCommandAllocator;
	visualizeMeshParams.m_pRenderTarget = pRenderTarget;
	visualizeMeshParams.m_pDepthTexture = m_pDepthTexture;
	
	m_pVisualizeMeshRecorder->Record(&visualizeMeshParams);
	m_pCommandQueue->ExecuteCommandLists(m_pEnv, 1, &m_pCommandList, pCommandAllocator);
	WaitForGPU();
	
	ClearVoxelGridRecordParams clearGridParams;
	clearGridParams.m_pEnv = m_pEnv;
	clearGridParams.m_pCommandAllocator = pCommandAllocator;
	clearGridParams.m_pCommandList = m_pCommandList;
	clearGridParams.m_pResources = m_pClearVoxelGridResources;
	
	m_pClearVoxelGridRecorder->Record(&clearGridParams);
//	m_pCommandQueue->ExecuteCommandLists(m_pEnv, 1, &m_pCommandList);
	WaitForGPU();

	CreateVoxelGridRecordParams createGridParams;
	createGridParams.m_pEnv = m_pEnv;
	createGridParams.m_pCommandList = m_pCommandList;
	createGridParams.m_pCommandAllocator = pCommandAllocator;
	createGridParams.m_pObjectTransformBuffer = m_pObjectTransformBuffer;
	createGridParams.m_pCameraTransformBuffer = m_pCameraTransformBuffer;
	createGridParams.m_pGridConfigBuffer = m_pGridConfigBuffer;
	createGridParams.m_pGridBuffer = m_pGridBuffer;
	createGridParams.m_pMesh = m_pMesh;

#ifdef HAS_TEXCOORD
	createGridParams.m_pColorTexture = 
	createGridParams.m_ColorSRVHandle = 
	createGridParams.m_LinearSamplerHandle = 
#endif // HAS_TEXCOORD

	m_pCreateVoxelGridRecorder->Record(&createGridParams);
//	m_pCommandQueue->ExecuteCommandLists(m_pEnv, 1, &m_pCommandList);
	WaitForGPU();

	VisualizeVoxelGridRecordParams visualizeGridParams;
	visualizeGridParams.m_pCommandList = m_pCommandList;
	visualizeGridParams.m_pCommandAllocator = pCommandAllocator;
	visualizeGridParams.m_pRenderTarget = pRenderTarget;
	visualizeGridParams.m_pDepthTexture = m_pDepthTexture;
	visualizeGridParams.m_pGridBuffer = m_pGridBuffer;
	visualizeGridParams.m_pGridConfigBuffer = m_pGridConfigBuffer;
	visualizeGridParams.m_pCameraTransformBuffer = m_pCameraTransformBuffer;

	m_pVisualizeVoxelGridRecorder->Record(&visualizeGridParams);
//	m_pCommandQueue->ExecuteCommandLists(m_pEnv, 1, &m_pCommandList);
	*/	
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
