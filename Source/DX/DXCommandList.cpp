#include "DX/DXCommandList.h"
#include "DX/DXCommandAllocator.h"
#include "DX/DXCommandSignature.h"
#include "DX/DXPipelineState.h"
#include "DX/DXResource.h"
#include "DX/DXRootSignature.h"
#include "DX/DXDevice.h"
#include "DX/DXDescriptorHeap.h"
#include "DX/DXUtils.h"

DXCommandList::DXCommandList(DXDevice* pDevice, DXCommandAllocator* pCommandAllocator, DXPipelineState* pInitialState, LPCWSTR pName)
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

void DXCommandList::Reset(DXCommandAllocator* pAllocator, DXPipelineState* pInitialState)
{
	DXVerify(GetDXObject()->Reset(pAllocator->GetDXObject(),
		(pInitialState != nullptr) ? pInitialState->GetDXObject() : nullptr));

	m_pResourceTransitions = nullptr;
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

void DXCommandList::ResourceBarrier(UINT numBarriers, const D3D12_RESOURCE_BARRIER* pBarriers)
{
	GetDXObject()->ResourceBarrier(numBarriers, pBarriers);
}

DXResourceTransitionList* DXCommandList::GetResourceTransitions()
{
	return m_pResourceTransitions;
}

void DXCommandList::SetResourceTransitions(DXResourceTransitionList* pResourceTransitions)
{
	m_pResourceTransitions = pResourceTransitions;
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

void DXCommandList::ExecuteIndirect(DXCommandSignature* pCommandSignature, UINT maxCommandCount,
	DXBuffer* pArgumentBuffer, UINT64 argumentBufferOffset,
	DXBuffer* pCountBuffer, UINT64 countBufferOffset)
{
	GetDXObject()->ExecuteIndirect(pCommandSignature->GetDXObject(), maxCommandCount,
		pArgumentBuffer->GetDXObject(), argumentBufferOffset,
		pCountBuffer->GetDXObject(), countBufferOffset);
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

void DXCommandList::ClearUnorderedAccessView(D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle, D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle, DXResource* pResource, const UINT clearValue[4])
{
	GetDXObject()->ClearUnorderedAccessViewUint(gpuHandle, cpuHandle, pResource->GetDXObject(), clearValue, 0, nullptr);
}

void DXCommandList::ClearUnorderedAccessView(D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle, D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle, DXResource* pResource, const FLOAT clearValue[4])
{
	GetDXObject()->ClearUnorderedAccessViewFloat(gpuHandle, cpuHandle, pResource->GetDXObject(), clearValue, 0, nullptr);
}

DXCommandListPool::DXCommandListPool(DXDevice* pDevice, D3D12_COMMAND_LIST_TYPE type)
	: m_pDevice(pDevice)
	, m_Type(type)
{
}

DXCommandListPool::~DXCommandListPool()
{
	while (!m_CommandListQueue.empty())
	{
		DXCommandList* pCommandList = m_CommandListQueue.front();
		m_CommandListQueue.pop();

		SafeDelete(pCommandList);
	}
}

DXCommandList* DXCommandListPool::Create(DXCommandAllocator* pCommandAllocator, DXPipelineState* pInitialState, LPCWSTR pName)
{
	assert(m_Type == pCommandAllocator->GetType());
	if (!m_CommandListQueue.empty())
	{
		DXCommandList* pCommandList = m_CommandListQueue.front();
		m_CommandListQueue.pop();

		pCommandList->Reset(pCommandAllocator, pInitialState);
		pCommandList->SetName(pName);

		return pCommandList;
	}
	return new DXCommandList(m_pDevice, pCommandAllocator, pInitialState, pName);
}

void DXCommandListPool::Release(DXCommandList* pCommandList)
{
	m_CommandListQueue.push(pCommandList);
}
