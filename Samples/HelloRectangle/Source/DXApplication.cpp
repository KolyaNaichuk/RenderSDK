#include "DXApplication.h"
#include "D3DWrapper/D3DFactory.h"
#include "D3DWrapper/D3DDevice.h"
#include "D3DWrapper/D3DSwapChain.h"
#include "D3DWrapper/D3DCommandQueue.h"
#include "D3DWrapper/D3DCommandAllocator.h"
#include "D3DWrapper/D3DCommandList.h"
#include "D3DWrapper/D3DDescriptorHeap.h"
#include "D3DWrapper/D3DRootSignature.h"
#include "D3DWrapper/D3DPipelineState.h"
#include "D3DWrapper/D3DResource.h"
#include "D3DWrapper/D3DRenderEnv.h"
#include "D3DWrapper/D3DFence.h"
#include "D3DWrapper/D3DUtils.h"
#include "Math/Vector4.h"

DXApplication::DXApplication(HINSTANCE hApp)
	: Application(hApp, L"Hello Rectangle", 0, 0, 1024, 512)
	, m_pDevice(nullptr)
	, m_pSwapChain(nullptr)
	, m_pCommandQueue(nullptr)
	, m_pShaderInvisibleRTVHeap(nullptr)
	, m_pShaderInvisibleSRVHeap(nullptr)
	, m_pRootSignature(nullptr)
	, m_pPipelineState(nullptr)
	, m_pCommandList(nullptr)
	, m_pVertexBuffer(nullptr)
	, m_pIndexBuffer(nullptr)
	, m_pDefaultHeapProps(new D3DHeapProperties(D3D12_HEAP_TYPE_DEFAULT))
	, m_pUploadHeapProps(new D3DHeapProperties(D3D12_HEAP_TYPE_UPLOAD))
	, m_pRenderEnv(new D3DRenderEnv())
	, m_pFence(nullptr)
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

	SafeDelete(m_pRenderEnv);
	SafeDelete(m_pDefaultHeapProps);
	SafeDelete(m_pUploadHeapProps);
	SafeDelete(m_pViewport);
	SafeDelete(m_pScissorRect);
	SafeDelete(m_pVertexBuffer);
	SafeDelete(m_pIndexBuffer);
	SafeDelete(m_pCommandList);
	SafeDelete(m_pFence);
	SafeDelete(m_pShaderInvisibleSRVHeap);
	SafeDelete(m_pShaderInvisibleRTVHeap);
	SafeDelete(m_pRootSignature);
	SafeDelete(m_pPipelineState);
	SafeDelete(m_pDevice);
	SafeDelete(m_pSwapChain);
	SafeDelete(m_pCommandQueue);
}

void DXApplication::OnInit()
{
	D3DFactory factory;
	m_pDevice = new D3DDevice(&factory, D3D_FEATURE_LEVEL_11_0);
	
	D3DDescriptorHeapDesc rtvHeapDesc(D3D12_DESCRIPTOR_HEAP_TYPE_RTV, kBackBufferCount, false);
	m_pShaderInvisibleRTVHeap = new D3DDescriptorHeap(m_pDevice, &rtvHeapDesc, L"m_pShaderInvisibleRTVHeap");

	D3DDescriptorHeapDesc srvHeapDesc(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, kBackBufferCount, false);
	m_pShaderInvisibleSRVHeap = new D3DDescriptorHeap(m_pDevice, &srvHeapDesc, L"m_pShaderInvisibleSRVHeap");

	m_pRenderEnv->m_pDevice = m_pDevice;
	m_pRenderEnv->m_pDefaultHeapProps = m_pDefaultHeapProps;
	m_pRenderEnv->m_pUploadHeapProps = m_pUploadHeapProps;
	m_pRenderEnv->m_pShaderInvisibleRTVHeap = m_pShaderInvisibleRTVHeap;
	m_pRenderEnv->m_pShaderInvisibleSRVHeap = m_pShaderInvisibleSRVHeap;

	D3DCommandQueueDesc commandQueueDesc(D3D12_COMMAND_LIST_TYPE_DIRECT);
	m_pCommandQueue = new D3DCommandQueue(m_pDevice, &commandQueueDesc, L"m_pCommandQueue");
		
	const RECT bufferRect = m_pWindow->GetClientRect();
	const UINT bufferWidth = bufferRect.right - bufferRect.left;
	const UINT bufferHeight = bufferRect.bottom - bufferRect.top;

	m_pViewport = new D3DViewport(0.0f, 0.0f, FLOAT(bufferWidth), FLOAT(bufferHeight));
	m_pScissorRect = new D3DRect(0, 0, bufferWidth, bufferHeight);

	D3DSwapChainDesc swapChainDesc(kBackBufferCount, m_pWindow->GetHWND(), bufferWidth, bufferHeight);
	m_pSwapChain = new D3DSwapChain(&factory, m_pRenderEnv, &swapChainDesc, m_pCommandQueue);
	m_BackBufferIndex = m_pSwapChain->GetCurrentBackBufferIndex();
	
	for (UINT index = 0; index < kBackBufferCount; ++index)
		m_CommandAllocators[index] = new D3DCommandAllocator(m_pDevice, D3D12_COMMAND_LIST_TYPE_DIRECT, L"m_CommandAllocators");

	D3DRootSignatureDesc rootSignatureDesc(D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
	m_pRootSignature = new D3DRootSignature(m_pDevice, &rootSignatureDesc, L"D3DRootSignature");

	const D3DInputElementDesc inputElementDescs[] =
	{
		D3DInputElementDesc("POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0),
		D3DInputElementDesc("COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 16)
	};
	
	D3DShader vertexShader(L"Shaders//PassThroughVS.hlsl", "Main", "vs_4_0");
	D3DShader pixelShader(L"Shaders//PassThroughPS.hlsl", "Main", "ps_4_0");

	D3DGraphicsPipelineStateDesc pipelineStateDesc;
	pipelineStateDesc.SetRootSignature(m_pRootSignature);
	pipelineStateDesc.SetVertexShader(&vertexShader);
	pipelineStateDesc.SetPixelShader(&pixelShader);
	pipelineStateDesc.InputLayout = D3DInputLayoutDesc(ARRAYSIZE(inputElementDescs), inputElementDescs);
	pipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	pipelineStateDesc.SetRenderTargetFormat(DXGI_FORMAT_R8G8B8A8_UNORM);
		
	m_pPipelineState = new D3DPipelineState(m_pDevice, &pipelineStateDesc, L"D3DPipelineState");
	
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

	D3DVertexBufferDesc vertexBufferDesc(ARRAYSIZE(vertices), sizeof(vertices[0]));
	m_pVertexBuffer = new D3DBuffer(m_pRenderEnv, m_pRenderEnv->m_pDefaultHeapProps, &vertexBufferDesc, D3D12_RESOURCE_STATE_COPY_DEST, L"m_pVertexBuffer");
	
	D3DBuffer uploadVertexBuffer(m_pRenderEnv, m_pRenderEnv->m_pUploadHeapProps, &vertexBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, L"uploadVertexBuffer");
	uploadVertexBuffer.Write(vertices, sizeof(vertices));

	D3DIndexBufferDesc indexBufferDesc(ARRAYSIZE(indices), sizeof(indices[0]));
	m_pIndexBuffer = new D3DBuffer(m_pRenderEnv, m_pRenderEnv->m_pDefaultHeapProps, &indexBufferDesc, D3D12_RESOURCE_STATE_COPY_DEST, L"m_pIndexBuffer");
	
	D3DBuffer uploadIndexBuffer(m_pRenderEnv, m_pRenderEnv->m_pUploadHeapProps, &indexBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, L"uploadIndexBuffer");
	uploadIndexBuffer.Write(indices, sizeof(indices));
	
	m_pFence = new D3DFence(m_pDevice, m_FenceValues[m_BackBufferIndex]);
	++m_FenceValues[m_BackBufferIndex];

	m_pCommandList = new D3DCommandList(m_pDevice, m_CommandAllocators[m_BackBufferIndex], nullptr, L"m_pCommandList");

	m_pCommandList->CopyResource(m_pVertexBuffer, &uploadVertexBuffer);
	m_pCommandList->CopyResource(m_pIndexBuffer, &uploadIndexBuffer);
	
	const D3D12_RESOURCE_BARRIER resourceTransitions[] =
	{
		D3DResourceTransitionBarrier(m_pVertexBuffer, m_pVertexBuffer->GetState(), m_pVertexBuffer->GetReadState()),
		D3DResourceTransitionBarrier(m_pIndexBuffer, m_pIndexBuffer->GetState(), m_pIndexBuffer->GetReadState())
	};
	m_pCommandList->ResourceBarrier(ARRAYSIZE(resourceTransitions), &resourceTransitions[0]);

	m_pVertexBuffer->SetState(m_pVertexBuffer->GetReadState());
	m_pIndexBuffer->SetState(m_pIndexBuffer->GetReadState());

	m_pCommandList->Close();
	m_pCommandQueue->ExecuteCommandLists(m_pRenderEnv, 1, &m_pCommandList, nullptr);

	WaitForGPU();
}

void DXApplication::OnUpdate()
{
}

void DXApplication::OnRender()
{
	D3DCommandAllocator* pCommandAllocator = m_CommandAllocators[m_BackBufferIndex];
	pCommandAllocator->Reset();
	
	m_pCommandList->Reset(pCommandAllocator, m_pPipelineState);
	m_pCommandList->SetGraphicsRootSignature(m_pRootSignature);
		
	D3DColorTexture* pRenderTarget = m_pSwapChain->GetBackBuffer(m_BackBufferIndex);
		
	D3DResourceTransitionBarrier writeStateTransition(pRenderTarget, pRenderTarget->GetState(), pRenderTarget->GetWriteState());
	m_pCommandList->ResourceBarrier(1, &writeStateTransition);
	pRenderTarget->SetState(pRenderTarget->GetWriteState());
		
	const FLOAT clearColor[4] = {0.1f, 0.7f, 0.4f, 1.0f};
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = pRenderTarget->GetRTVHandle();
	m_pCommandList->ClearRenderTargetView(rtvHandle, clearColor);

	m_pCommandList->OMSetRenderTargets(1, &rtvHandle);
	m_pCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	m_pCommandList->IASetVertexBuffers(0, 1, m_pVertexBuffer->GetVBView());
	m_pCommandList->IASetIndexBuffer(m_pIndexBuffer->GetIBView());
	m_pCommandList->RSSetViewports(1, m_pViewport);
	m_pCommandList->RSSetScissorRects(1, m_pScissorRect);
	m_pCommandList->DrawIndexedInstanced(6, 1, 0, 0, 0);

	D3DResourceTransitionBarrier presentStateTransition(pRenderTarget, pRenderTarget->GetState(), D3D12_RESOURCE_STATE_PRESENT);
	m_pCommandList->ResourceBarrier(1, &presentStateTransition);
	pRenderTarget->SetState(D3D12_RESOURCE_STATE_PRESENT);

	m_pCommandList->Close();

	m_pCommandQueue->ExecuteCommandLists(m_pRenderEnv, 1, &m_pCommandList, nullptr);

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
