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
#include "DX/DXEvent.h"
#include "CommandRecorders/ClearVoxelGridRecorder.h"
#include "CommandRecorders/VisualizeMeshRecorder.h"
#include "Common/MeshData.h"
#include "Common/MeshDataUtilities.h"
#include "Common/Mesh.h"
#include "Common/Color.h"
#include "Common/Camera.h"
#include "Math/Vector3f.h"
#include "Math/Vector4f.h"
#include "Math/Matrix4f.h"
#include "Math/Transform.h"

enum
{
	kNumGridCellsX = 64,
	kNumGridCellsY = 64,
	kNumGridCellsZ = 64,
};

enum DSVHeapHandles
{
	kDSVHandle = 0,
	kNumDSVHandles
};

enum CBVSRVUAVHeapHandles
{
	kTransformCBVHandle = 0,
	kGridConfigCBVHandle,
	kGridBufferUAVHandle,
	kNumCBVSRVUAVHandles
};

struct ObjectTransform
{
	Matrix4f m_WorldNormalMatrix;
	Matrix4f m_WorldViewProjMatrix;
	Matrix4f m_NotUsed1;
	Matrix4f m_NotUsed2;
};

struct GridConfig
{
	Vector4f m_WorldSpaceOrigin;
	Vector4f m_RcpCellSize;
	//int4   numCells;
};

DXApplication::DXApplication(HINSTANCE hApp)
	: Application(hApp, L"Scene Voxelization", 0, 0, 924, 668)
	, m_pDevice(nullptr)
	, m_pSwapChain(nullptr)
	, m_pCommandQueue(nullptr)
	, m_pCommandList(nullptr)
	, m_pRTVHeap(nullptr)
	, m_pDSVHeap(nullptr)
	, m_pCBVSRVUAVHeap(nullptr)
	, m_pDSVTexture(nullptr)
	, m_pTransformBuffer(nullptr)
	, m_pGridBuffer(nullptr)
	, m_pGridConfigBuffer(nullptr)
	, m_pFence(nullptr)
	, m_pFenceEvent(nullptr)
	, m_BackBufferIndex(0)
	, m_pClearVoxelGridRecorder(nullptr)
	, m_pVisualizeMeshRecorder(nullptr)
	, m_pMesh(nullptr)
	, m_pCamera(nullptr)
{
	for (UINT index = 0; index < kBackBufferCount; ++index)
		m_CommandAllocators[index] = nullptr;

	for (UINT index = 0; index < kBackBufferCount; ++index)
		m_FenceValues[index] = 0;
}

DXApplication::~DXApplication()
{
	SafeDelete(m_pCamera);
	SafeDelete(m_pMesh);
	SafeDelete(m_pClearVoxelGridRecorder);
	SafeDelete(m_pVisualizeMeshRecorder);
	SafeDelete(m_pFenceEvent);
	SafeDelete(m_pFence);
	SafeDelete(m_pCBVSRVUAVHeap);
	SafeDelete(m_pGridConfigBuffer);
	SafeDelete(m_pGridBuffer);
	SafeDelete(m_pTransformBuffer);
	SafeDelete(m_pDSVHeap);
	SafeDelete(m_pDSVTexture);
	SafeDelete(m_pRTVHeap);
	SafeDelete(m_pCommandQueue);
	SafeDelete(m_pCommandList);
	SafeDelete(m_pSwapChain);
	SafeDelete(m_pDevice);
}

void DXApplication::OnInit()
{
	DXFactory factory;

	m_pDevice = new DXDevice(&factory, D3D_FEATURE_LEVEL_11_0);

	DXCommandQueueDesc commandQueueDesc(D3D12_COMMAND_LIST_TYPE_DIRECT);
	m_pCommandQueue = new DXCommandQueue(m_pDevice, &commandQueueDesc, L"m_pCommandQueue");

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
	m_pSwapChain = new DXSwapChain(&factory, &swapChainDesc, m_pCommandQueue);
	m_BackBufferIndex = m_pSwapChain->GetCurrentBackBufferIndex();

	DXDescriptorHeapDesc rtvHeapDesc(D3D12_DESCRIPTOR_HEAP_TYPE_RTV, kBackBufferCount, false);
	m_pRTVHeap = new DXDescriptorHeap(m_pDevice, &rtvHeapDesc, L"m_pRTVHeap");

	DXTex2DRenderTargetViewDesc rtvDesc;
	for (UINT index = 0; index < kBackBufferCount; ++index)
	{
		DXResource* pRenderTarget = m_pSwapChain->GetBackBuffer(index);
		D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_pRTVHeap->GetCPUDescriptor(index);

		m_pDevice->CreateRenderTargetView(pRenderTarget, &rtvDesc, rtvHandle);
	}

	DXHeapProperties uploadHeapProperties(D3D12_HEAP_TYPE_UPLOAD);
	DXHeapProperties defaultHeapProperties(D3D12_HEAP_TYPE_DEFAULT);

	DXDescriptorHeapDesc dsvHeapDesc(D3D12_DESCRIPTOR_HEAP_TYPE_DSV, kNumDSVHandles, false);
	m_pDSVHeap = new DXDescriptorHeap(m_pDevice, &dsvHeapDesc, L"m_pDSVHeap");

	DXTex2DResourceDesc dsvTexDesc(DXGI_FORMAT_D32_FLOAT, bufferWidth, bufferHeight, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);
	m_pDSVTexture = new DXResource(m_pDevice, &defaultHeapProperties, D3D12_HEAP_FLAG_NONE, &dsvTexDesc, D3D12_RESOURCE_STATE_DEPTH_WRITE, L"m_pDSVTexture");

	DXTex2DDepthStencilViewDesc dsvDesc;
	m_pDevice->CreateDepthStencilView(m_pDSVTexture, &dsvDesc, m_pDSVHeap->GetCPUDescriptor(kDSVHandle));

	DXDescriptorHeapDesc cbvHeapDesc(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, kNumCBVSRVUAVHandles, true);
	m_pCBVSRVUAVHeap = new DXDescriptorHeap(m_pDevice, &cbvHeapDesc, L"m_pCBVSRVUAVHeap");

	DXBufferResourceDesc transformBufferDesc(sizeof(ObjectTransform));
	m_pTransformBuffer = new DXResource(m_pDevice, &uploadHeapProperties, D3D12_HEAP_FLAG_NONE, &transformBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, L"m_pCBVBuffer");

	DXConstantBufferViewDesc transformCBVDesc(m_pTransformBuffer, sizeof(ObjectTransform));
	m_pDevice->CreateConstantBufferView(&transformCBVDesc, m_pCBVSRVUAVHeap->GetCPUDescriptor(kTransformCBVHandle));

	for (UINT index = 0; index < kBackBufferCount; ++index)
		m_CommandAllocators[index] = new DXCommandAllocator(m_pDevice, D3D12_COMMAND_LIST_TYPE_DIRECT, L"m_CommandAllocators");

	m_pFenceEvent = new DXEvent();
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
		Color::WHITE, Color::WHITE, Color::WHITE, Color::WHITE,

		// Ceiling
		Color::WHITE, Color::WHITE, Color::WHITE, Color::WHITE,

		// Back wall
		Color::WHITE, Color::WHITE, Color::WHITE, Color::WHITE,
				
		// Right wall
		Color::GREEN, Color::GREEN, Color::GREEN, Color::GREEN,	

		// Left wall
		Color::RED, Color::RED, Color::RED, Color::RED,

		// Short block
		Color::WHITE, Color::WHITE, Color::WHITE, Color::WHITE,
		Color::WHITE, Color::WHITE, Color::WHITE, Color::WHITE,
		Color::WHITE, Color::WHITE, Color::WHITE, Color::WHITE,
		Color::WHITE, Color::WHITE, Color::WHITE, Color::WHITE, 
		Color::WHITE, Color::WHITE, Color::WHITE, Color::WHITE,

		// Tall block
		Color::WHITE, Color::WHITE, Color::WHITE, Color::WHITE,
		Color::WHITE, Color::WHITE, Color::WHITE, Color::WHITE,
		Color::WHITE, Color::WHITE, Color::WHITE, Color::WHITE,
		Color::WHITE, Color::WHITE, Color::WHITE, Color::WHITE,
		Color::WHITE, Color::WHITE, Color::WHITE, Color::WHITE
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
	
	m_pMesh = new Mesh(m_pDevice, &meshData);
	
	m_pMesh->RecordDataForUpload(m_pCommandList);
	m_pCommandList->Close();

	ID3D12CommandList* pDXCommandList = m_pCommandList->GetDXObject();
	m_pCommandQueue->ExecuteCommandLists(1, &pDXCommandList);
	
	WaitForGPU();
	m_pMesh->RemoveDataForUpload();

	ClearVoxelGridInitParams clearGridInitParams;
	clearGridInitParams.m_pDevice = m_pDevice;
	clearGridInitParams.m_NumGridCellsX = kNumGridCellsX;
	clearGridInitParams.m_NumGridCellsY = kNumGridCellsY;
	clearGridInitParams.m_NumGridCellsZ = kNumGridCellsZ;

	m_pClearVoxelGridRecorder = new ClearVoxelGridRecorder(&clearGridInitParams);
	
	VisualizeMeshInitParams visualizeMeshInitParams;
	visualizeMeshInitParams.m_pDevice = m_pDevice;
	visualizeMeshInitParams.m_MeshDataElement = MeshDataElement_Normal;
	visualizeMeshInitParams.m_VertexElementFlags = m_pMesh->GetVertexElementFlags();
	visualizeMeshInitParams.m_RTVFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	visualizeMeshInitParams.m_DSVFormat = DXGI_FORMAT_D32_FLOAT;
		
	m_pVisualizeMeshRecorder = new VisualizeMeshRecorder(&visualizeMeshInitParams);
}

void DXApplication::OnUpdate()
{
	const Matrix4f& viewMatrix = m_pCamera->GetViewMatrix();
	const Matrix4f& projMatrix = m_pCamera->GetProjMatrix();

	ObjectTransform transform;
	transform.m_WorldNormalMatrix = Matrix4f::IDENTITY;
	transform.m_WorldViewProjMatrix = Matrix4f::IDENTITY * viewMatrix * projMatrix;

	m_pTransformBuffer->Write(&transform, sizeof(transform));
}

void DXApplication::OnRender()
{
	ID3D12CommandList* pDXCommandList = m_pCommandList->GetDXObject();

	DXCommandAllocator* pCommandAllocator = m_CommandAllocators[m_BackBufferIndex];
	pCommandAllocator->Reset();

#if 0
	DXResource* pRTVTexture = m_pSwapChain->GetBackBuffer(m_BackBufferIndex);
	
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_pRTVHeap->GetCPUDescriptor(m_BackBufferIndex);
	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = m_pDSVHeap->GetCPUDescriptor(kDSVHandle);
	
	const u8 clearFlags = m_pCamera->GetClearFlags();
	if (clearFlags != 0)
	{
		m_pCommandList->Reset(pCommandAllocator);

		if (clearFlags & Camera::ClearFlag_Color)
		{
			if (pRTVTexture->GetState() != D3D12_RESOURCE_STATE_RENDER_TARGET)
				m_pCommandList->TransitionBarrier(pRTVTexture, D3D12_RESOURCE_STATE_RENDER_TARGET);

			const Vector4f& clearColor = m_pCamera->GetBackgroundColor();
			m_pCommandList->ClearRenderTargetView(rtvHandle, &clearColor.m_X);
		}
		if (clearFlags & Camera::ClearFlag_Depth)
		{
			if (m_pDSVTexture->GetState() != D3D12_RESOURCE_STATE_DEPTH_WRITE)
				m_pCommandList->TransitionBarrier(m_pDSVTexture, D3D12_RESOURCE_STATE_DEPTH_WRITE);

			m_pCommandList->ClearDepthView(dsvHandle);
		}

		m_pCommandList->Close();
		m_pCommandQueue->ExecuteCommandLists(1, &pDXCommandList);

		WaitForGPU();
	}

	D3D12_RESOURCE_STATES rtvEndState = D3D12_RESOURCE_STATE_PRESENT;
	
	VisualizeMeshRecordParams params;
	params.m_pMesh = m_pMesh;
	params.m_pCommandList = m_pCommandList;
	params.m_pCommandAllocator = pCommandAllocator;
	params.m_pRTVTexture = pRTVTexture;
	params.m_RTVHandle = rtvHandle;
	params.m_pRTVEndState = &rtvEndState;
	params.m_pDSVTexture = m_pDSVTexture;
	params.m_DSVHandle = dsvHandle;
	params.m_NumDXDescriptorHeaps = 1;
	params.m_pDXFirstDescriptorHeap = m_pCBVSRVUAVHeap->GetDXObject();
	params.m_CBVHandle = m_pCBVSRVUAVHeap->GetGPUDescriptor(kTransformCBVHandle);
	
	m_pVisualizeMeshRecorder->Record(&params);
#else
	ClearVoxelGridRecordParams recordParams;
	recordParams.m_pCommandAllocator = pCommandAllocator;
	recordParams.m_pCommandList = m_pCommandList;
	recordParams.m_NumDXDescriptorHeaps = 1;
	recordParams.m_pDXFirstDescriptorHeap = m_pCBVSRVUAVHeap->GetDXObject();
	//recordParams.m_pGridBuffer = 
	//recordParams.m_GridBufferUAVHandle =
	//recordParams.m_GridConfigCBVHandle =

#endif

	m_pCommandQueue->ExecuteCommandLists(1, &pDXCommandList);

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

	m_pFence->SetEventOnCompletion(m_FenceValues[m_BackBufferIndex], m_pFenceEvent);
	m_pFenceEvent->Wait();

	++m_FenceValues[m_BackBufferIndex];
}

void DXApplication::MoveToNextFrame()
{
	const UINT64 currentFenceValue = m_FenceValues[m_BackBufferIndex];
	m_pCommandQueue->Signal(m_pFence, currentFenceValue);

	m_BackBufferIndex = m_pSwapChain->GetCurrentBackBufferIndex();
	if (m_pFence->GetCompletedValue() < m_FenceValues[m_BackBufferIndex])
	{
		m_pFence->SetEventOnCompletion(m_FenceValues[m_BackBufferIndex], m_pFenceEvent);
		m_pFenceEvent->Wait();
	}

	m_FenceValues[m_BackBufferIndex] = currentFenceValue + 1;
}
