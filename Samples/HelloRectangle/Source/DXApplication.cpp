#include "DXApplication.h"
#include "DX/DXFactory.h"
#include "DX/DXDevice.h"
#include "DX/DXSwapChain.h"
#include "DX/DXCommandQueue.h"
#include "DX/DXCommandAllocator.h"
#include "DX/DXCommandList.h"
#include "DX/DXDescriptorHeap.h"
#include "DX/DXRootSignature.h"
#include "DX/DXPipelineState.h"
#include "DX/DXResource.h"
#include "DX/DXRenderEnvironment.h"
#include "DX/DXFence.h"
#include "DX/DXEvent.h"
#include "DX/DXUtils.h"
#include "Math/Vector4.h"

DXApplication::DXApplication(HINSTANCE hApp)
	: Application(hApp, L"Hello Rectangle", 0, 0, 1024, 512)
	, m_pDevice(nullptr)
	, m_pSwapChain(nullptr)
	, m_pCommandQueue(nullptr)
	, m_pRTVDescriptorHeap(nullptr)
	, m_pRootSignature(nullptr)
	, m_pPipelineState(nullptr)
	, m_pCommandList(nullptr)
	, m_pVertexBuffer(nullptr)
	, m_pVertexBufferView(nullptr)
	, m_pIndexBuffer(nullptr)
	, m_pIndexBufferView(nullptr)
	, m_pDefaultHeapProps(new DXHeapProperties(D3D12_HEAP_TYPE_DEFAULT))
	, m_pUploadHeapProps(new DXHeapProperties(D3D12_HEAP_TYPE_UPLOAD))
	, m_pEnv(new DXRenderEnvironment())
	, m_pFence(nullptr)
	, m_pFenceEvent(nullptr)
	, m_pViewport(nullptr)
	, m_pScissorRect(nullptr)
	, m_BackBufferIndex(0)
{
	std::memset(m_CommandAllocators, 0, sizeof(m_CommandAllocators));
	std::memset(m_FenceValues, 0, sizeof(m_FenceValues));
}

DXApplication::~DXApplication()
{
	for (UINT index = 0; index < kBackBufferCount; ++index)
		SafeDelete(m_CommandAllocators[index]);

	SafeDelete(m_pEnv);
	SafeDelete(m_pDefaultHeapProps);
	SafeDelete(m_pUploadHeapProps);
	SafeDelete(m_pViewport);
	SafeDelete(m_pScissorRect);
	SafeDelete(m_pVertexBufferView);
	SafeDelete(m_pVertexBuffer);
	SafeDelete(m_pIndexBufferView);
	SafeDelete(m_pIndexBuffer);
	SafeDelete(m_pCommandList);
	SafeDelete(m_pFenceEvent);
	SafeDelete(m_pFence);
	SafeDelete(m_pRTVDescriptorHeap);
	SafeDelete(m_pRootSignature);
	SafeDelete(m_pPipelineState);
	SafeDelete(m_pDevice);
	SafeDelete(m_pSwapChain);
	SafeDelete(m_pCommandQueue);
}

void DXApplication::OnInit()
{
	DXFactory factory;
	m_pDevice = new DXDevice(&factory, D3D_FEATURE_LEVEL_11_0);
	
	DXDescriptorHeapDesc descriptorHeapDesc(D3D12_DESCRIPTOR_HEAP_TYPE_RTV, kBackBufferCount, false);
	m_pRTVDescriptorHeap = new DXDescriptorHeap(m_pDevice, &descriptorHeapDesc, L"m_pRTVDescriptorHeap");

	m_pEnv->m_pDevice = m_pDevice;
	m_pEnv->m_pDefaultHeapProps = m_pDefaultHeapProps;
	m_pEnv->m_pUploadHeapProps = m_pUploadHeapProps;
	m_pEnv->m_pRTVDescriptorHeap = m_pRTVDescriptorHeap;

	DXCommandQueueDesc commandQueueDesc(D3D12_COMMAND_LIST_TYPE_DIRECT);
	m_pCommandQueue = new DXCommandQueue(m_pDevice, &commandQueueDesc, L"m_pCommandQueue");
		
	const RECT bufferRect = m_pWindow->GetClientRect();
	const UINT bufferWidth = bufferRect.right - bufferRect.left;
	const UINT bufferHeight = bufferRect.bottom - bufferRect.top;

	m_pViewport = new DXViewport(0.0f, 0.0f, FLOAT(bufferWidth), FLOAT(bufferHeight));
	m_pScissorRect = new DXRect(0, 0, bufferWidth, bufferHeight);

	DXSwapChainDesc swapChainDesc(kBackBufferCount, m_pWindow->GetHWND(), bufferWidth, bufferHeight);
	m_pSwapChain = new DXSwapChain(&factory, m_pEnv, &swapChainDesc, m_pCommandQueue);
	m_BackBufferIndex = m_pSwapChain->GetCurrentBackBufferIndex();
	
	for (UINT index = 0; index < kBackBufferCount; ++index)
		m_CommandAllocators[index] = new DXCommandAllocator(m_pDevice, D3D12_COMMAND_LIST_TYPE_DIRECT, L"m_CommandAllocators");

	DXRootSignatureDesc rootSignatureDesc(D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
	m_pRootSignature = new DXRootSignature(m_pDevice, &rootSignatureDesc, L"DXRootSignature");

	const DXInputElementDesc inputElementDescs[] =
	{
		DXInputElementDesc("POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0),
		DXInputElementDesc("COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 16)
	};
	
	DXShader vertexShader(L"Shaders//PassThroughVS.hlsl", "Main", "vs_4_0");
	DXShader pixelShader(L"Shaders//PassThroughPS.hlsl", "Main", "ps_4_0");

	DXGraphicsPipelineStateDesc pipelineStateDesc;
	pipelineStateDesc.SetRootSignature(m_pRootSignature);
	pipelineStateDesc.SetVertexShader(&vertexShader);
	pipelineStateDesc.SetPixelShader(&pixelShader);
	pipelineStateDesc.SetInputLayout(ARRAYSIZE(inputElementDescs), inputElementDescs);
	pipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	pipelineStateDesc.SetRenderTargetFormat(DXGI_FORMAT_R8G8B8A8_UNORM);
		
	m_pPipelineState = new DXPipelineState(m_pDevice, &pipelineStateDesc, L"DXPipelineState");
	
	const FLOAT scale = 0.5f;
	struct Vertex
	{
		Vector4f clipSpacePos;
		Vector4f color;
	};	
	const Vertex vertices[] = 
	{
		{Vector4f(-1.0f * scale,  1.0f * scale, 0.0f, 1.0f), Vector4f(0.5f, 0.0f, 0.5f, 1.0f)},
		{Vector4f( 1.0f * scale,  1.0f * scale, 0.0f, 1.0f), Vector4f(0.5f, 0.0f, 0.5f, 1.0f)},
		{Vector4f( 1.0f * scale, -1.0f * scale, 0.0f, 1.0f), Vector4f(0.5f, 0.0f, 0.5f, 1.0f)},
		{Vector4f(-1.0f * scale, -1.0f * scale, 0.0f, 1.0f), Vector4f(0.5f, 0.0f, 0.5f, 1.0f)}
	};
	const WORD indices[] = {0, 1, 3, 1, 2, 3};

	DXVertexBufferDesc vertexBufferDesc(sizeof(vertices));
	m_pVertexBuffer = new DXBuffer(m_pEnv, m_pEnv->m_pDefaultHeapProps, &vertexBufferDesc, D3D12_RESOURCE_STATE_COPY_DEST, L"m_pVertexBuffer");
	m_pVertexBufferView = new DXVertexBufferView(m_pVertexBuffer, sizeof(vertices), sizeof(vertices[0]));
	
	DXBuffer uploadVertexBuffer(m_pEnv, m_pEnv->m_pUploadHeapProps, &vertexBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, L"uploadVertexBuffer");
	uploadVertexBuffer.Write(vertices, sizeof(vertices));

	DXIndexBufferDesc indexBufferDesc(sizeof(indices));
	m_pIndexBuffer = new DXBuffer(m_pEnv, m_pEnv->m_pDefaultHeapProps, &indexBufferDesc, D3D12_RESOURCE_STATE_COPY_DEST, L"m_pIndexBuffer");
	m_pIndexBufferView = new DXIndexBufferView(m_pIndexBuffer, sizeof(indices), sizeof(indices[0]));

	DXBuffer uploadIndexBuffer(m_pEnv, m_pEnv->m_pUploadHeapProps, &indexBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, L"uploadIndexBuffer");
	uploadIndexBuffer.Write(indices, sizeof(indices));
			
	m_pFenceEvent = new DXEvent();
	m_pFence = new DXFence(m_pDevice, m_FenceValues[m_BackBufferIndex]);
	++m_FenceValues[m_BackBufferIndex];

	m_pCommandList = new DXCommandList(m_pDevice, m_CommandAllocators[m_BackBufferIndex], nullptr, L"m_pCommandList");

	m_pCommandList->CopyResource(m_pVertexBuffer, &uploadVertexBuffer);
	m_pCommandList->TransitionBarrier(m_pVertexBuffer, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);

	m_pCommandList->CopyResource(m_pIndexBuffer, &uploadIndexBuffer);
	m_pCommandList->TransitionBarrier(m_pIndexBuffer, D3D12_RESOURCE_STATE_INDEX_BUFFER);

	m_pCommandList->Close();

	ID3D12CommandList* pDXCommandList = m_pCommandList->GetDXObject();
	m_pCommandQueue->ExecuteCommandLists(1, &pDXCommandList);
	WaitForGPU();
}

void DXApplication::OnUpdate()
{
}

void DXApplication::OnRender()
{
	DXCommandAllocator* pCommandAllocator = m_CommandAllocators[m_BackBufferIndex];
	pCommandAllocator->Reset();
	
	m_pCommandList->Reset(pCommandAllocator, m_pPipelineState);
	m_pCommandList->SetGraphicsRootSignature(m_pRootSignature);
		
	DXRenderTarget* pRenderTarget = m_pSwapChain->GetBackBuffer(m_BackBufferIndex);
	m_pCommandList->TransitionBarrier(pRenderTarget, D3D12_RESOURCE_STATE_RENDER_TARGET);

	const FLOAT clearColor[4] = {0.1f, 0.7f, 0.4f, 1.0f};
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = pRenderTarget->GetRTVHandle();
	m_pCommandList->ClearRenderTargetView(rtvHandle, clearColor);

	m_pCommandList->OMSetRenderTargets(1, &rtvHandle);
	m_pCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	m_pCommandList->IASetVertexBuffers(0, 1, m_pVertexBufferView);
	m_pCommandList->IASetIndexBuffer(m_pIndexBufferView);
	m_pCommandList->RSSetViewports(1, m_pViewport);
	m_pCommandList->RSSetScissorRects(1, m_pScissorRect);
	m_pCommandList->DrawIndexedInstanced(6, 1, 0, 0, 0);

	m_pCommandList->TransitionBarrier(pRenderTarget, D3D12_RESOURCE_STATE_PRESENT);
	m_pCommandList->Close();

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
