#include "D3DWrapper/D3DCommandList.h"
#include "D3DWrapper/D3DCommandAllocator.h"
#include "D3DWrapper/D3DCommandSignature.h"
#include "D3DWrapper/D3DPipelineState.h"
#include "D3DWrapper/D3DResource.h"
#include "D3DWrapper/D3DRootSignature.h"
#include "D3DWrapper/D3DDevice.h"
#include "D3DWrapper/D3DDescriptorHeap.h"
#include "D3DWrapper/D3DUtils.h"

D3DCommandList::D3DCommandList(D3DDevice* pDevice, D3DCommandAllocator* pCommandAllocator, D3DPipelineState* pInitialState, LPCWSTR pName)
	: m_pResourceTransitions(nullptr)
{
	UINT nodeMask = 0;
	DXVerify(pDevice->GetDXObject()->CreateCommandList(nodeMask,
		pCommandAllocator->GetType(),
		pCommandAllocator->GetDXObject(),
	   (pInitialState != nullptr) ? pInitialState->GetDXObject() : nullptr,
		IID_PPV_ARGS(GetDXObjectAddress())));

#ifdef _DEBUG
	SetName(pName);
#endif
}

void D3DCommandList::Reset(D3DCommandAllocator* pAllocator, D3DPipelineState* pInitialState)
{
	DXVerify(GetDXObject()->Reset(pAllocator->GetDXObject(),
		(pInitialState != nullptr) ? pInitialState->GetDXObject() : nullptr));

	m_pResourceTransitions = nullptr;
}

void D3DCommandList::Close()
{
	DXVerify(GetDXObject()->Close());
}

void D3DCommandList::IASetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY primitiveTopology)
{
	GetDXObject()->IASetPrimitiveTopology(primitiveTopology);
}

void D3DCommandList::IASetVertexBuffers(UINT startSlot, UINT numViews, const D3DVertexBufferView* pViews)
{
	GetDXObject()->IASetVertexBuffers(startSlot, numViews, pViews);
}

void D3DCommandList::IASetIndexBuffer(const D3DIndexBufferView* pView)
{
	GetDXObject()->IASetIndexBuffer(pView);
}

void D3DCommandList::CopyResource(D3DResource* pDestResource, D3DResource* pSourceResource)
{
	GetDXObject()->CopyResource(pDestResource->GetDXObject(), pSourceResource->GetDXObject());
}

void D3DCommandList::CopyBufferRegion(D3DBuffer* pDestBuffer, UINT64 destOffset, D3DBuffer* pSourceBuffer, UINT64 sourceOffset, UINT64 numBytes)
{
	GetDXObject()->CopyBufferRegion(pDestBuffer->GetDXObject(), destOffset, pSourceBuffer->GetDXObject(), sourceOffset, numBytes);
}

void D3DCommandList::ResourceBarrier(UINT numBarriers, const D3D12_RESOURCE_BARRIER* pBarriers)
{
	GetDXObject()->ResourceBarrier(numBarriers, pBarriers);
}

D3DRequiredResourceStateList* D3DCommandList::GetResourceTransitions()
{
	return m_pResourceTransitions;
}

void D3DCommandList::SetResourceTransitions(D3DRequiredResourceStateList* pResourceTransitions)
{
	m_pResourceTransitions = pResourceTransitions;
}

void D3DCommandList::SetGraphicsRootSignature(D3DRootSignature* pRootSignature)
{
	GetDXObject()->SetGraphicsRootSignature(pRootSignature->GetDXObject());
}

void D3DCommandList::SetDescriptorHeaps(D3DDescriptorHeap* pCBVSRVUAVDescriptorHeap, D3DDescriptorHeap* pSamplerDescriptorHeap)
{
	static const UINT maxNumDescriptorHeaps = 2;
	ID3D12DescriptorHeap* dxDescriptorHeaps[maxNumDescriptorHeaps];
	
	assert(pCBVSRVUAVDescriptorHeap != nullptr);
	dxDescriptorHeaps[0] = pCBVSRVUAVDescriptorHeap->GetDXObject();
	UINT numDescriptorHeaps = 1;

	if (pSamplerDescriptorHeap != nullptr)
	{
		dxDescriptorHeaps[1] = pSamplerDescriptorHeap->GetDXObject();
		++numDescriptorHeaps;
	}

	GetDXObject()->SetDescriptorHeaps(numDescriptorHeaps, dxDescriptorHeaps);
}

void D3DCommandList::SetGraphicsRootDescriptorTable(UINT rootParameterIndex, D3D12_GPU_DESCRIPTOR_HANDLE baseHandle)
{
	GetDXObject()->SetGraphicsRootDescriptorTable(rootParameterIndex, baseHandle);
}

void D3DCommandList::SetComputeRootSignature(D3DRootSignature* pRootSignature)
{
	GetDXObject()->SetComputeRootSignature(pRootSignature->GetDXObject());
}

void D3DCommandList::SetComputeRootDescriptorTable(UINT rootParameterIndex, D3D12_GPU_DESCRIPTOR_HANDLE baseHandle)
{
	GetDXObject()->SetComputeRootDescriptorTable(rootParameterIndex, baseHandle);
}

void D3DCommandList::RSSetViewports(UINT numViewports, const D3DViewport* pViewports)
{
	GetDXObject()->RSSetViewports(numViewports, pViewports);
}

void D3DCommandList::RSSetScissorRects(UINT numRects, const D3DRect* pRects)
{
	GetDXObject()->RSSetScissorRects(numRects, pRects);
}

void D3DCommandList::OMSetRenderTargets(UINT numRenderTargets, const D3D12_CPU_DESCRIPTOR_HANDLE* renderTargetViewHandles,
	BOOL singleRenderTargetViewsRange, const D3D12_CPU_DESCRIPTOR_HANDLE* pDepthStencilHandle)
{
	GetDXObject()->OMSetRenderTargets(numRenderTargets, renderTargetViewHandles, singleRenderTargetViewsRange, pDepthStencilHandle);
}

void D3DCommandList::DrawInstanced(UINT vertexCountPerInstance, UINT instanceCount, UINT startVertexLocation, UINT startInstanceLocation)
{
	GetDXObject()->DrawInstanced(vertexCountPerInstance, instanceCount, startVertexLocation, startInstanceLocation);
}

void D3DCommandList::DrawIndexedInstanced(UINT indexCountPerInstance, UINT instanceCount, UINT startIndexLocation, INT baseVertexLocation, UINT startInstanceLocation)
{
	GetDXObject()->DrawIndexedInstanced(indexCountPerInstance, instanceCount, startIndexLocation, baseVertexLocation, startInstanceLocation);
}

void D3DCommandList::Dispatch(UINT numThreadGroupsX, UINT numThreadGroupsY, UINT numThreadGroupsZ)
{
	GetDXObject()->Dispatch(numThreadGroupsX, numThreadGroupsY, numThreadGroupsZ);
}

void D3DCommandList::ExecuteIndirect(D3DCommandSignature* pCommandSignature, UINT maxCommandCount,
	D3DBuffer* pArgumentBuffer, UINT64 argumentBufferOffset,
	D3DBuffer* pCountBuffer, UINT64 countBufferOffset)
{
	GetDXObject()->ExecuteIndirect(pCommandSignature->GetDXObject(), maxCommandCount,
		pArgumentBuffer->GetDXObject(), argumentBufferOffset,
		pCountBuffer->GetDXObject(), countBufferOffset);
}

void D3DCommandList::ClearRenderTargetView(D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle, const FLOAT clearColor[4])
{
	GetDXObject()->ClearRenderTargetView(cpuHandle, clearColor, 0, nullptr);
}

void D3DCommandList::ClearDepthStencilView(D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle, FLOAT depth, UINT8 stencil)
{
	GetDXObject()->ClearDepthStencilView(cpuHandle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, depth, stencil, 0, nullptr);
}

void D3DCommandList::ClearDepthView(D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle, FLOAT depth)
{
	GetDXObject()->ClearDepthStencilView(cpuHandle, D3D12_CLEAR_FLAG_DEPTH, depth, 0, 0, nullptr);
}

void D3DCommandList::ClearStencilView(D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle, UINT8 stencil)
{
	GetDXObject()->ClearDepthStencilView(cpuHandle, D3D12_CLEAR_FLAG_STENCIL, 1.0f, stencil, 0, nullptr);
}

void D3DCommandList::ClearUnorderedAccessView(D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle, D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle, D3DResource* pResource, const UINT clearValue[4])
{
	GetDXObject()->ClearUnorderedAccessViewUint(gpuHandle, cpuHandle, pResource->GetDXObject(), clearValue, 0, nullptr);
}

void D3DCommandList::ClearUnorderedAccessView(D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle, D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle, D3DResource* pResource, const FLOAT clearValue[4])
{
	GetDXObject()->ClearUnorderedAccessViewFloat(gpuHandle, cpuHandle, pResource->GetDXObject(), clearValue, 0, nullptr);
}

D3DCommandListPool::D3DCommandListPool(D3DDevice* pDevice, D3D12_COMMAND_LIST_TYPE type)
	: m_pDevice(pDevice)
	, m_Type(type)
{
}

D3DCommandListPool::~D3DCommandListPool()
{
	while (!m_CommandListQueue.empty())
	{
		D3DCommandList* pCommandList = m_CommandListQueue.front();
		m_CommandListQueue.pop();

		SafeDelete(pCommandList);
	}
}

D3DCommandList* D3DCommandListPool::Create(D3DCommandAllocator* pCommandAllocator, D3DPipelineState* pInitialState, LPCWSTR pName)
{
	assert(m_Type == pCommandAllocator->GetType());
	if (!m_CommandListQueue.empty())
	{
		D3DCommandList* pCommandList = m_CommandListQueue.front();
		m_CommandListQueue.pop();

		pCommandList->Reset(pCommandAllocator, pInitialState);
		pCommandList->SetName(pName);

		return pCommandList;
	}
	return new D3DCommandList(m_pDevice, pCommandAllocator, pInitialState, pName);
}

void D3DCommandListPool::Release(D3DCommandList* pCommandList)
{
	m_CommandListQueue.push(pCommandList);
}
