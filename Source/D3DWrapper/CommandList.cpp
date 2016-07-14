#include "D3DWrapper/CommandList.h"
#include "D3DWrapper/CommandAllocator.h"
#include "D3DWrapper/CommandSignature.h"
#include "D3DWrapper/PipelineState.h"
#include "D3DWrapper/GraphicsResource.h"
#include "D3DWrapper/RootSignature.h"
#include "D3DWrapper/GraphicsDevice.h"
#include "D3DWrapper/DescriptorHeap.h"
#include "D3DWrapper/GraphicsUtils.h"

CommandList::CommandList(GraphicsDevice* pDevice, CommandAllocator* pCommandAllocator, PipelineState* pInitialState, LPCWSTR pName)
	: m_pRequiredResourceStates(nullptr)
{
	UINT nodeMask = 0;
	VerifyD3DResult(pDevice->GetD3DObject()->CreateCommandList(nodeMask,
		pCommandAllocator->GetType(),
		pCommandAllocator->GetD3DObject(),
	   (pInitialState != nullptr) ? pInitialState->GetD3DObject() : nullptr,
		IID_PPV_ARGS(&m_D3DCommandList)));

#ifdef _DEBUG
	SetName(pName);
#endif
}

void CommandList::SetName(LPCWSTR pName) {
	VerifyD3DResult(m_D3DCommandList->SetName(pName));
}

void CommandList::Reset(CommandAllocator* pAllocator, PipelineState* pInitialState)
{
	VerifyD3DResult(m_D3DCommandList->Reset(pAllocator->GetD3DObject(),
		(pInitialState != nullptr) ? pInitialState->GetD3DObject() : nullptr));

	m_pRequiredResourceStates = nullptr;
}

void CommandList::Close()
{
	VerifyD3DResult(m_D3DCommandList->Close());
}

void CommandList::IASetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY primitiveTopology)
{
	m_D3DCommandList->IASetPrimitiveTopology(primitiveTopology);
}

void CommandList::IASetVertexBuffers(UINT startSlot, UINT numViews, const VertexBufferView* pViews)
{
	m_D3DCommandList->IASetVertexBuffers(startSlot, numViews, pViews);
}

void CommandList::IASetIndexBuffer(const IndexBufferView* pView)
{
	m_D3DCommandList->IASetIndexBuffer(pView);
}

void CommandList::CopyResource(GraphicsResource* pDestResource, GraphicsResource* pSourceResource)
{
	m_D3DCommandList->CopyResource(pDestResource->GetD3DObject(), pSourceResource->GetD3DObject());
}

void CommandList::CopyBufferRegion(Buffer* pDestBuffer, UINT64 destOffset, Buffer* pSourceBuffer, UINT64 sourceOffset, UINT64 numBytes)
{
	m_D3DCommandList->CopyBufferRegion(pDestBuffer->GetD3DObject(), destOffset, pSourceBuffer->GetD3DObject(), sourceOffset, numBytes);
}

void CommandList::ResourceBarrier(UINT numBarriers, const D3D12_RESOURCE_BARRIER* pBarriers)
{
	m_D3DCommandList->ResourceBarrier(numBarriers, pBarriers);
}

RequiredResourceStateList* CommandList::GetRequiredResourceStates()
{
	return m_pRequiredResourceStates;
}

void CommandList::SetRequiredResourceStates(RequiredResourceStateList* pRequiredResourceStates)
{
	m_pRequiredResourceStates = pRequiredResourceStates;
}

void CommandList::SetGraphicsRootSignature(RootSignature* pRootSignature)
{
	m_D3DCommandList->SetGraphicsRootSignature(pRootSignature->GetD3DObject());
}

void CommandList::SetDescriptorHeaps(DescriptorHeap* pCBVSRVUAVDescriptorHeap, DescriptorHeap* pSamplerDescriptorHeap)
{
	static const UINT maxNumDescriptorHeaps = 2;
	ID3D12DescriptorHeap* dxDescriptorHeaps[maxNumDescriptorHeaps];
	
	assert(pCBVSRVUAVDescriptorHeap != nullptr);
	dxDescriptorHeaps[0] = pCBVSRVUAVDescriptorHeap->GetD3DObject();
	UINT numDescriptorHeaps = 1;

	if (pSamplerDescriptorHeap != nullptr)
	{
		dxDescriptorHeaps[1] = pSamplerDescriptorHeap->GetD3DObject();
		++numDescriptorHeaps;
	}

	m_D3DCommandList->SetDescriptorHeaps(numDescriptorHeaps, dxDescriptorHeaps);
}

void CommandList::SetGraphicsRootDescriptorTable(UINT rootParameterIndex, D3D12_GPU_DESCRIPTOR_HANDLE baseHandle)
{
	m_D3DCommandList->SetGraphicsRootDescriptorTable(rootParameterIndex, baseHandle);
}

void CommandList::SetComputeRootSignature(RootSignature* pRootSignature)
{
	m_D3DCommandList->SetComputeRootSignature(pRootSignature->GetD3DObject());
}

void CommandList::SetComputeRootDescriptorTable(UINT rootParameterIndex, D3D12_GPU_DESCRIPTOR_HANDLE baseHandle)
{
	m_D3DCommandList->SetComputeRootDescriptorTable(rootParameterIndex, baseHandle);
}

void CommandList::RSSetViewports(UINT numViewports, const Viewport* pViewports)
{
	m_D3DCommandList->RSSetViewports(numViewports, pViewports);
}

void CommandList::RSSetScissorRects(UINT numRects, const Rect* pRects)
{
	m_D3DCommandList->RSSetScissorRects(numRects, pRects);
}

void CommandList::OMSetRenderTargets(UINT numRenderTargets, const D3D12_CPU_DESCRIPTOR_HANDLE* renderTargetViewHandles,
	BOOL singleRenderTargetViewsRange, const D3D12_CPU_DESCRIPTOR_HANDLE* pDepthStencilHandle)
{
	m_D3DCommandList->OMSetRenderTargets(numRenderTargets, renderTargetViewHandles, singleRenderTargetViewsRange, pDepthStencilHandle);
}

void CommandList::DrawInstanced(UINT vertexCountPerInstance, UINT instanceCount, UINT startVertexLocation, UINT startInstanceLocation)
{
	m_D3DCommandList->DrawInstanced(vertexCountPerInstance, instanceCount, startVertexLocation, startInstanceLocation);
}

void CommandList::DrawIndexedInstanced(UINT indexCountPerInstance, UINT instanceCount, UINT startIndexLocation, INT baseVertexLocation, UINT startInstanceLocation)
{
	m_D3DCommandList->DrawIndexedInstanced(indexCountPerInstance, instanceCount, startIndexLocation, baseVertexLocation, startInstanceLocation);
}

void CommandList::Dispatch(UINT numThreadGroupsX, UINT numThreadGroupsY, UINT numThreadGroupsZ)
{
	m_D3DCommandList->Dispatch(numThreadGroupsX, numThreadGroupsY, numThreadGroupsZ);
}

void CommandList::ExecuteIndirect(CommandSignature* pCommandSignature, UINT maxCommandCount,
	Buffer* pArgumentBuffer, UINT64 argumentBufferOffset,
	Buffer* pCountBuffer, UINT64 countBufferOffset)
{
	m_D3DCommandList->ExecuteIndirect(pCommandSignature->GetD3DObject(), maxCommandCount,
		pArgumentBuffer->GetD3DObject(), argumentBufferOffset,
		pCountBuffer->GetD3DObject(), countBufferOffset);
}

void CommandList::ClearRenderTargetView(D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle, const FLOAT clearColor[4])
{
	m_D3DCommandList->ClearRenderTargetView(cpuHandle, clearColor, 0, nullptr);
}

void CommandList::ClearDepthStencilView(D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle, FLOAT depth, UINT8 stencil)
{
	m_D3DCommandList->ClearDepthStencilView(cpuHandle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, depth, stencil, 0, nullptr);
}

void CommandList::ClearDepthView(D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle, FLOAT depth)
{
	m_D3DCommandList->ClearDepthStencilView(cpuHandle, D3D12_CLEAR_FLAG_DEPTH, depth, 0, 0, nullptr);
}

void CommandList::ClearStencilView(D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle, UINT8 stencil)
{
	m_D3DCommandList->ClearDepthStencilView(cpuHandle, D3D12_CLEAR_FLAG_STENCIL, 1.0f, stencil, 0, nullptr);
}

void CommandList::ClearUnorderedAccessView(D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle, D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle, GraphicsResource* pResource, const UINT clearValue[4])
{
	m_D3DCommandList->ClearUnorderedAccessViewUint(gpuHandle, cpuHandle, pResource->GetD3DObject(), clearValue, 0, nullptr);
}

void CommandList::ClearUnorderedAccessView(D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle, D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle, GraphicsResource* pResource, const FLOAT clearValue[4])
{
	m_D3DCommandList->ClearUnorderedAccessViewFloat(gpuHandle, cpuHandle, pResource->GetD3DObject(), clearValue, 0, nullptr);
}

CommandListPool::CommandListPool(GraphicsDevice* pDevice, D3D12_COMMAND_LIST_TYPE type)
	: m_pDevice(pDevice)
	, m_Type(type)
{
}

CommandListPool::~CommandListPool()
{
	while (!m_CommandListQueue.empty())
	{
		CommandList* pCommandList = m_CommandListQueue.front();
		m_CommandListQueue.pop();

		SafeDelete(pCommandList);
	}
}

CommandList* CommandListPool::Create(CommandAllocator* pCommandAllocator, PipelineState* pInitialState, LPCWSTR pName)
{
	assert(m_Type == pCommandAllocator->GetType());
	if (!m_CommandListQueue.empty())
	{
		CommandList* pCommandList = m_CommandListQueue.front();
		m_CommandListQueue.pop();

		pCommandList->Reset(pCommandAllocator, pInitialState);
#ifdef _DEBUG
		pCommandList->SetName(pName);
#endif

		return pCommandList;
	}
	return new CommandList(m_pDevice, pCommandAllocator, pInitialState, pName);
}

void CommandListPool::Release(CommandList* pCommandList)
{
	m_CommandListQueue.push(pCommandList);
}
