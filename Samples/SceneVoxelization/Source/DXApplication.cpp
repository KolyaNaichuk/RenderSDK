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
#include "Common/MeshData.h"
#include "Common/MeshDataUtilities.h"
#include "Common/Color.h"
#include "Math/Vector3f.h"
#include "Math/Vector4f.h"

enum
{
	kNumGridCellsX = 64,
	kNumGridCellsY = 64,
	kNumGridCellsZ = 64
};

DXApplication::DXApplication(HINSTANCE hApp)
	: Application(hApp, L"Scene Voxelization", 0, 0, 1024, 512)
	, m_pDevice(nullptr)
	, m_pSwapChain(nullptr)
	, m_pCommandQueue(nullptr)
	, m_pRTVHeap(nullptr)
	, m_pFence(nullptr)
	, m_pFenceEvent(nullptr)
	, m_BackBufferIndex(0)
	, m_pClearVoxelGridRecorder(nullptr)
{
	for (UINT index = 0; index < kBackBufferCount; ++index)
		m_CommandAllocators[index] = nullptr;

	for (UINT index = 0; index < kBackBufferCount; ++index)
		m_FenceValues[index] = 0;
}

DXApplication::~DXApplication()
{
	delete m_pClearVoxelGridRecorder;
	delete m_pFenceEvent;
	delete m_pFence;
	delete m_pRTVHeap;
	delete m_pCommandQueue;
	delete m_pSwapChain;
	delete m_pDevice;
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
	
	DXSwapChainDesc swapChainDesc(kBackBufferCount, m_pWindow->GetHWND(), bufferWidth, bufferHeight);
	m_pSwapChain = new DXSwapChain(&factory, &swapChainDesc, m_pCommandQueue);
	m_BackBufferIndex = m_pSwapChain->GetCurrentBackBufferIndex();

	DXDescriptorHeapDesc descriptorHeapDesc(D3D12_DESCRIPTOR_HEAP_TYPE_RTV, kBackBufferCount, false);
	m_pRTVHeap = new DXDescriptorHeap(m_pDevice, &descriptorHeapDesc, L"m_pRTVHeap");

	DXTex2DRenderTargetViewDesc rtvDesc;
	for (UINT index = 0; index < kBackBufferCount; ++index)
	{
		DXResource* pRenderTarget = m_pSwapChain->GetBackBuffer(index);
		D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_pRTVHeap->GetCPUDescriptor(index);

		m_pDevice->CreateRenderTargetView(pRenderTarget, &rtvDesc, rtvHandle);
	}

	for (UINT index = 0; index < kBackBufferCount; ++index)
		m_CommandAllocators[index] = new DXCommandAllocator(m_pDevice, D3D12_COMMAND_LIST_TYPE_DIRECT, L"m_CommandAllocators");

	m_pClearVoxelGridRecorder = new ClearVoxelGridRecorder(m_pDevice, kNumGridCellsX, kNumGridCellsY, kNumGridCellsZ);
	
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

	ConvertMeshData(&meshData, ConvertionFlags_LeftHandedCoordSystem);
}

void DXApplication::OnUpdate()
{
}

void DXApplication::OnRender()
{
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
