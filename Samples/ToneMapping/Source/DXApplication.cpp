#include "DXApplication.h"
#include "DX/DXFactory.h"
#include "DX/DXDevice.h"
#include "DX/DXSwapChain.h"
#include "DX/DXCommandQueue.h"
#include "DX/DXCommandAllocator.h"
#include "DX/DXCommandList.h"
#include "DX/DXDescriptorHeap.h"
#include "DX/DXPipelineState.h"
#include "DX/DXResource.h"
#include "DX/DXFence.h"
#include "DX/DXEvent.h"
#include "DX/DXUtils.h"
#include "DDSTextureLoader/DDSTextureLoader.h"
#include "CommandRecorders/CopyTextureRecorder.h"
#include "CommandRecorders/CalcTextureLuminanceRecorder.h"

enum SRVDescriptors
{
	kSRVDescriptor_HDRTexture = 0,
	kSRVDescriptor_Count
};

enum SamplerDescriptors
{
	kSamplerDescriptor_Point = 0,
	kSamplerDescriptor_Count
};

DXApplication::DXApplication(HINSTANCE hApp)
	: Application(hApp, L"Tone Mapping - F1 (HDR), F2 (Lum), F3 (Log Lum)", 0, 0, 700, 700)
	, m_pDevice(nullptr)
	, m_pSwapChain(nullptr)
	, m_pCommandQueue(nullptr)
	, m_pRTVDescriptorHeap(nullptr)
	, m_pSRVHeap(nullptr)
	, m_pSamplerHeap(nullptr)
	, m_pCommandList(nullptr)
	, m_pFence(nullptr)
	, m_pFenceEvent(nullptr)
	, m_BackBufferIndex(0)
	, m_pHDRTexture(nullptr)
	, m_pCopyTextureRecorder(nullptr)
	, m_CalcTextureLuminanceRecorder(nullptr)
	, m_CalcTextureLogLuminanceRecorder(nullptr)
	, m_DisplayResult(DisplayResult_HDRImage)
{
	for (UINT index = 0; index < kBackBufferCount; ++index)
		m_CommandAllocators[index] = nullptr;

	for (UINT index = 0; index < kBackBufferCount; ++index)
		m_FenceValues[index] = 0;
}

DXApplication::~DXApplication()
{
	for (UINT index = 0; index < kBackBufferCount; ++index)
		delete m_CommandAllocators[index];

	delete m_pCopyTextureRecorder;
	delete m_CalcTextureLuminanceRecorder;
	delete m_CalcTextureLogLuminanceRecorder;
	delete m_pSRVHeap;
	delete m_pSamplerHeap;
	delete m_pHDRTexture;
	delete m_pCommandList;
	delete m_pFenceEvent;
	delete m_pFence;
	delete m_pRTVDescriptorHeap;
	delete m_pDevice;
	delete m_pSwapChain;
	delete m_pCommandQueue;
}

void DXApplication::OnInit()
{
	DXFactory factory;

	m_pDevice = new DXDevice(&factory, D3D_FEATURE_LEVEL_11_0, true);
	
	DXCommandQueueDesc commandQueueDesc(D3D12_COMMAND_LIST_TYPE_DIRECT);
	m_pCommandQueue = new DXCommandQueue(m_pDevice, &commandQueueDesc, L"m_pCommandQueue");
		
	const RECT bufferRect = m_pWindow->GetClientRect();
	const UINT bufferWidth = bufferRect.right - bufferRect.left;
	const UINT bufferHeight = bufferRect.bottom - bufferRect.top;

	DXSwapChainDesc swapChainDesc(kBackBufferCount, m_pWindow->GetHWND(), bufferWidth, bufferHeight);
	m_pSwapChain = new DXSwapChain(&factory, &swapChainDesc, m_pCommandQueue);
	m_BackBufferIndex = m_pSwapChain->GetCurrentBackBufferIndex();

	DXDescriptorHeapDesc rtvDescriptorHeapDesc(D3D12_DESCRIPTOR_HEAP_TYPE_RTV, kBackBufferCount, false);
	m_pRTVDescriptorHeap = new DXDescriptorHeap(m_pDevice, &rtvDescriptorHeapDesc, L"m_pRTVDescriptorHeap");

	DXDescriptorHeapDesc srvDescriptorHeapDesc(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, kSRVDescriptor_Count, true);
	m_pSRVHeap = new DXDescriptorHeap(m_pDevice, &srvDescriptorHeapDesc, L"m_pSRVHeap");

	DXDescriptorHeapDesc samplerDescriptorHeapDesc(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, kSamplerDescriptor_Count, true);
	m_pSamplerHeap = new DXDescriptorHeap(m_pDevice, &samplerDescriptorHeapDesc, L"m_pSamplerHeap");

	DXSamplerDesc pointSampler(DXSamplerDesc::Point);
	m_pDevice->CreateSampler(&pointSampler, m_pSamplerHeap->GetCPUDescriptor(kSamplerDescriptor_Point));
	
	DXTex2DRenderTargetViewDesc rtvDesc;
	for (UINT index = 0; index < kBackBufferCount; ++index)
	{
		DXResource* pRenderTarget = m_pSwapChain->GetBackBuffer(index);
		D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_pRTVDescriptorHeap->GetCPUDescriptor(index);

		m_pDevice->CreateRenderTargetView(pRenderTarget, &rtvDesc, rtvHandle);
	}

	DXResource* pFirstRenderTarget = m_pSwapChain->GetBackBuffer(0);
	const DXGI_FORMAT rtvFormat = GetRenderTargetViewFormat(pFirstRenderTarget->GetFormat());

	m_pCopyTextureRecorder = new CopyTextureRecorder(m_pDevice, rtvFormat);
	m_CalcTextureLuminanceRecorder = new CalcTextureLuminanceRecorder(m_pDevice, rtvFormat);
	m_CalcTextureLogLuminanceRecorder = new CalcTextureLuminanceRecorder(m_pDevice, rtvFormat, true);

	for (UINT index = 0; index < kBackBufferCount; ++index)
		m_CommandAllocators[index] = new DXCommandAllocator(m_pDevice, D3D12_COMMAND_LIST_TYPE_DIRECT, L"m_CommandAllocators");

	DXHeapProperties defaultHeapProperties(D3D12_HEAP_TYPE_DEFAULT);
				
	m_pFenceEvent = new DXEvent();
	m_pFence = new DXFence(m_pDevice, m_FenceValues[m_BackBufferIndex]);
	++m_FenceValues[m_BackBufferIndex];

	m_pCommandList = new DXCommandList(m_pDevice, m_CommandAllocators[m_BackBufferIndex], nullptr, L"m_pCommandList");
		
	ID3D12Resource* pDXUploadTexture = nullptr;
	D3D12_SHADER_RESOURCE_VIEW_DESC srvView;
	DXVerify(DirectX::CreateDDSTextureFromFileEx(m_pDevice->GetDXObject(), L"cornellbox.dds", 0, true, &pDXUploadTexture, &srvView));
	DXResource uploadTexture(pDXUploadTexture, D3D12_RESOURCE_STATE_COMMON, L"pDXUploadTexture");

	D3D12_RESOURCE_DESC texDesc = pDXUploadTexture->GetDesc();
	m_pHDRTexture = new DXResource(m_pDevice, &defaultHeapProperties, D3D12_HEAP_FLAG_NONE, &texDesc, D3D12_RESOURCE_STATE_COMMON, L"m_pHDRTexture");
	
	m_pCommandList->TransitionBarrier(m_pHDRTexture, D3D12_RESOURCE_STATE_COPY_DEST);
	m_pCommandList->CopyResource(m_pHDRTexture, &uploadTexture);
	m_pCommandList->TransitionBarrier(m_pHDRTexture, D3D12_RESOURCE_STATE_GENERIC_READ);

	m_pCommandList->Close();

	ID3D12CommandList* pDXCommandList = m_pCommandList->GetDXObject();
	m_pCommandQueue->ExecuteCommandLists(1, &pDXCommandList);
	WaitForGPU();

	m_pDevice->CreateShaderResourceView(m_pHDRTexture, &srvView, m_pSRVHeap->GetCPUDescriptor(kSRVDescriptor_HDRTexture));
}

void DXApplication::OnUpdate()
{
}

void DXApplication::OnRender()
{
	DXCommandAllocator* pCommandAllocator = m_CommandAllocators[m_BackBufferIndex];
	pCommandAllocator->Reset();
	
	DXResource* pRenderTarget = m_pSwapChain->GetBackBuffer(m_BackBufferIndex);
	D3D12_RESOURCE_STATES rtvEndState = D3D12_RESOURCE_STATE_PRESENT;

	if (m_DisplayResult == DisplayResult_HDRImage)
	{
		m_pCopyTextureRecorder->Record(m_pCommandList, pCommandAllocator,
			pRenderTarget, m_pRTVDescriptorHeap->GetCPUDescriptor(m_BackBufferIndex),
			m_pSRVHeap, m_pHDRTexture, m_pSRVHeap->GetGPUDescriptor(kSRVDescriptor_HDRTexture),
			m_pSamplerHeap, m_pSamplerHeap->GetGPUDescriptor(kSamplerDescriptor_Point),
			&rtvEndState);
	}
	else if (m_DisplayResult == DisplayResult_ImageLuminance)
	{
		m_CalcTextureLuminanceRecorder->Record(m_pCommandList, pCommandAllocator,
			pRenderTarget, m_pRTVDescriptorHeap->GetCPUDescriptor(m_BackBufferIndex),
			m_pSRVHeap, m_pHDRTexture, m_pSRVHeap->GetGPUDescriptor(kSRVDescriptor_HDRTexture),
			m_pSamplerHeap, m_pSamplerHeap->GetGPUDescriptor(kSamplerDescriptor_Point),
			&rtvEndState);
	}
	else if (m_DisplayResult == DisplayResult_ImageLogLuminance)
	{
		m_CalcTextureLogLuminanceRecorder->Record(m_pCommandList, pCommandAllocator,
			pRenderTarget, m_pRTVDescriptorHeap->GetCPUDescriptor(m_BackBufferIndex),
			m_pSRVHeap, m_pHDRTexture, m_pSRVHeap->GetGPUDescriptor(kSRVDescriptor_HDRTexture),
			m_pSamplerHeap, m_pSamplerHeap->GetGPUDescriptor(kSamplerDescriptor_Point),
			&rtvEndState);
	}
	
	ID3D12CommandList* pDXCommandList = m_pCommandList->GetDXObject();
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
	if (key == VK_F1)
		m_DisplayResult = DisplayResult_HDRImage;
	else if (key == VK_F2)
		m_DisplayResult = DisplayResult_ImageLuminance;
	else if (key == VK_F3)
		m_DisplayResult = DisplayResult_ImageLogLuminance;
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
