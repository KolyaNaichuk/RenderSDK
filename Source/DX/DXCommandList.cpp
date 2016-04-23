#include "DX/DXCommandList.h"
#include "DX/DXCommandAllocator.h"
#include "DX/DXPipelineState.h"
#include "DX/DXResource.h"
#include "DX/DXRootSignature.h"
#include "DX/DXDevice.h"
#include "DX/DXDescriptorHeap.h"
#include "DX/DXUtils.h"

DXCommandList::DXCommandList(DXDevice* pDevice, DXCommandAllocator* pAllocator, DXPipelineState* pInitialState, LPCWSTR pName)
{
	UINT nodeMask = 0;
	DXVerify(pDevice->GetDXObject()->CreateCommandList(nodeMask,
		pAllocator->GetType(),
		pAllocator->GetDXObject(),
	   (pInitialState != nullptr) ? pInitialState->GetDXObject() : nullptr,
		IID_PPV_ARGS(GetDXObjectAddress())));

#ifdef _DEBUG
	SetName(pName);
#endif
}

void DXCommandList::Reset(DXCommandAllocator* pAllocator, DXPipelineState* pInitialState)
{
	DXVerify(GetDXObject()->Reset(pAllocator->GetDXObject(),
		(pInitialState != nullptr) ? pInitialState->GetDXObject() : nullptr));

	m_PendingResourceStates.clear();
}

void DXCommandList::Close()
{
	DXVerify(GetDXObject()->Close());
}

void DXCommandList::IASetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY primitiveTopology)
{
	GetDXObject()->IASetPrimitiveTopology(primitiveTopology);
}

void DXCommandList::IASetVertexBuffers(UINT startSlot, UINT numViews, const DXVertexBufferView* pViews)
{
	GetDXObject()->IASetVertexBuffers(startSlot, numViews, pViews);
}

void DXCommandList::IASetIndexBuffer(const DXIndexBufferView* pView)
{
	GetDXObject()->IASetIndexBuffer(pView);
}

void DXCommandList::CopyResource(DXResource* pDest, DXResource* pSource)
{
	GetDXObject()->CopyResource(pDest->GetDXObject(), pSource->GetDXObject());
}

void DXCommandList::TransitionBarrier(DXResource* pResource, D3D12_RESOURCE_STATES newState)
{
	D3D12_RESOURCE_BARRIER barrier;
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = pResource->GetDXObject();
	barrier.Transition.StateBefore = pResource->GetState();
	barrier.Transition.StateAfter = newState;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

	GetDXObject()->ResourceBarrier(1, &barrier);
	pResource->SetState(newState);
}

void DXCommandList::SetGraphicsRootSignature(DXRootSignature* pRootSignature)
{
	GetDXObject()->SetGraphicsRootSignature(pRootSignature->GetDXObject());
}

void DXCommandList::SetDescriptorHeaps(DXDescriptorHeap* pCBVSRVUAVDescriptorHeap, DXDescriptorHeap* pSamplerDescriptorHeap)
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

void DXCommandList::SetGraphicsRootDescriptorTable(UINT rootParameterIndex, D3D12_GPU_DESCRIPTOR_HANDLE baseHandle)
{
	GetDXObject()->SetGraphicsRootDescriptorTable(rootParameterIndex, baseHandle);
}

void DXCommandList::SetComputeRootSignature(DXRootSignature* pRootSignature)
{
	GetDXObject()->SetComputeRootSignature(pRootSignature->GetDXObject());
}

void DXCommandList::SetComputeRootDescriptorTable(UINT rootParameterIndex, D3D12_GPU_DESCRIPTOR_HANDLE baseHandle)
{
	GetDXObject()->SetComputeRootDescriptorTable(rootParameterIndex, baseHandle);
}

void DXCommandList::RSSetViewports(UINT numViewports, const DXViewport* pViewports)
{
	GetDXObject()->RSSetViewports(numViewports, pViewports);
}

void DXCommandList::RSSetScissorRects(UINT numRects, const DXRect* pRects)
{
	GetDXObject()->RSSetScissorRects(numRects, pRects);
}

void DXCommandList::OMSetRenderTargets(UINT numRenderTargets, const D3D12_CPU_DESCRIPTOR_HANDLE* renderTargetViewHandles,
	BOOL singleRenderTargetViewsRange, const D3D12_CPU_DESCRIPTOR_HANDLE* pDepthStencilHandle)
{
	GetDXObject()->OMSetRenderTargets(numRenderTargets, renderTargetViewHandles, singleRenderTargetViewsRange, pDepthStencilHandle);
}

void DXCommandList::DrawInstanced(UINT vertexCountPerInstance, UINT instanceCount, UINT startVertexLocation, UINT startInstanceLocation)
{
	GetDXObject()->DrawInstanced(vertexCountPerInstance, instanceCount, startVertexLocation, startInstanceLocation);
}

void DXCommandList::DrawIndexedInstanced(UINT indexCountPerInstance, UINT instanceCount, UINT startIndexLocation, INT baseVertexLocation, UINT startInstanceLocation)
{
	GetDXObject()->DrawIndexedInstanced(indexCountPerInstance, instanceCount, startIndexLocation, baseVertexLocation, startInstanceLocation);
}

void DXCommandList::Dispatch(UINT numThreadGroupsX, UINT numThreadGroupsY, UINT numThreadGroupsZ)
{
	GetDXObject()->Dispatch(numThreadGroupsX, numThreadGroupsY, numThreadGroupsZ);
}

void DXCommandList::ClearRenderTargetView(D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle, const FLOAT clearColor[4])
{
	GetDXObject()->ClearRenderTargetView(cpuHandle, clearColor, 0, nullptr);
}

void DXCommandList::ClearDepthStencilView(D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle, FLOAT depth, UINT8 stencil)
{
	GetDXObject()->ClearDepthStencilView(cpuHandle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, depth, stencil, 0, nullptr);
}

void DXCommandList::ClearDepthView(D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle, FLOAT depth)
{
	GetDXObject()->ClearDepthStencilView(cpuHandle, D3D12_CLEAR_FLAG_DEPTH, depth, 0, 0, nullptr);
}

void DXCommandList::ClearStencilView(D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle, UINT8 stencil)
{
	GetDXObject()->ClearDepthStencilView(cpuHandle, D3D12_CLEAR_FLAG_STENCIL, 1.0f, stencil, 0, nullptr);
}

void DXCommandList::ClearUnorderedAccessView(D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle, D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle, DXResource* pResource, const FLOAT clearValue[4])
{
	GetDXObject()->ClearUnorderedAccessViewFloat(gpuHandle, cpuHandle, pResource->GetDXObject(), clearValue, 0, nullptr);
}

DXPendingResourceState::DXPendingResourceState(DXResource* pResource, D3D12_RESOURCE_STATES nextState)
	: m_pResource(pResource)
	, m_NextState(nextState)
{
}

std::size_t DXCommandList::GetNumPendingResourceStates() const
{
	return m_PendingResourceStates.size();
}

DXPendingResourceState* DXCommandList::GetFirstPendingResourceState()
{
	return &m_PendingResourceStates[0];
}

void DXCommandList::PushPendingResourceState(DXResource* pResource, D3D12_RESOURCE_STATES nextState)
{
	m_PendingResourceStates.emplace_back(pResource, nextState);
}
