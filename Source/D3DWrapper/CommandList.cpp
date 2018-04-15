#include "D3DWrapper/CommandList.h"
#include "D3DWrapper/CommandSignature.h"
#include "D3DWrapper/PipelineState.h"
#include "D3DWrapper/GraphicsResource.h"
#include "D3DWrapper/RootSignature.h"
#include "D3DWrapper/GraphicsDevice.h"
#include "D3DWrapper/DescriptorHeap.h"
#include "D3DWrapper/QueryHeap.h"
#include "D3DWrapper/Fence.h"
#include "D3DWrapper/GraphicsUtils.h"

CommandList::CommandList(GraphicsDevice* pDevice, D3D12_COMMAND_LIST_TYPE type, LPCWSTR pName)
{
	ID3D12Device* pD3DDevice = pDevice->GetD3DObject();
	VerifyD3DResult(pD3DDevice->CreateCommandAllocator(type, IID_PPV_ARGS(&m_D3DCommandAllocator)));

	UINT nodeMask = 0;
	ID3D12PipelineState* pD3DPipelineState = nullptr;
	VerifyD3DResult(pD3DDevice->CreateCommandList(nodeMask, type, m_D3DCommandAllocator.Get(), pD3DPipelineState, IID_PPV_ARGS(&m_D3DCommandList)));
	VerifyD3DResult(m_D3DCommandList->Close());
	
#ifdef ENABLE_GRAPHICS_DEBUGGING
	VerifyD3DResult(m_D3DCommandList->SetName(pName));
#endif // ENABLE_GRAPHICS_DEBUGGING
}

void CommandList::SetName(LPCWSTR pName)
{
	VerifyD3DResult(m_D3DCommandList->SetName(pName));
}

void CommandList::Begin(PipelineState* pPipelineState)
{
	VerifyD3DResult(m_D3DCommandAllocator->Reset()); 
	VerifyD3DResult(m_D3DCommandList->Reset(m_D3DCommandAllocator.Get(), (pPipelineState != nullptr) ? pPipelineState->GetD3DObject() : nullptr));
	SetCompletionFence(nullptr, 0);
}

void CommandList::End()
{
	VerifyD3DResult(m_D3DCommandList->Close());
}

void CommandList::SetPipelineState(PipelineState* pPipelineState)
{
	m_D3DCommandList->SetPipelineState(pPipelineState->GetD3DObject());
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

void CommandList::CopyTextureRegion(const D3D12_TEXTURE_COPY_LOCATION* pDestLocation, UINT destX, UINT destY, UINT destZ,
	const D3D12_TEXTURE_COPY_LOCATION* pSourceLocation, const D3D12_BOX* pSourceBox)
{
	m_D3DCommandList->CopyTextureRegion(pDestLocation, destX, destY, destZ, pSourceLocation, pSourceBox);
}

void CommandList::CopyBufferRegion(Buffer* pDestBuffer, UINT64 destOffsetInBytes, Buffer* pSourceBuffer, UINT64 sourceOffsetInBytes, UINT64 numBytesToCopy)
{
	m_D3DCommandList->CopyBufferRegion(pDestBuffer->GetD3DObject(), destOffsetInBytes, pSourceBuffer->GetD3DObject(), sourceOffsetInBytes, numBytesToCopy);
}

void CommandList::ResourceBarrier(UINT numBarriers, const D3D12_RESOURCE_BARRIER* pBarriers)
{
	m_D3DCommandList->ResourceBarrier(numBarriers, pBarriers);
}

void CommandList::SetGraphicsRootSignature(RootSignature* pRootSignature)
{
	m_D3DCommandList->SetGraphicsRootSignature(pRootSignature->GetD3DObject());
}

void CommandList::SetDescriptorHeaps(DescriptorHeap* pSRVDescriptorHeap, DescriptorHeap* pSamplerDescriptorHeap)
{
	static const UINT maxNumDescriptorHeaps = 2;
	ID3D12DescriptorHeap* d3dDescriptorHeaps[maxNumDescriptorHeaps];
	
	assert(pSRVDescriptorHeap != nullptr);
	d3dDescriptorHeaps[0] = pSRVDescriptorHeap->GetD3DObject();
	UINT numDescriptorHeaps = 1;

	if (pSamplerDescriptorHeap != nullptr)
	{
		d3dDescriptorHeaps[1] = pSamplerDescriptorHeap->GetD3DObject();
		++numDescriptorHeaps;
	}

	m_D3DCommandList->SetDescriptorHeaps(numDescriptorHeaps, d3dDescriptorHeaps);
}

void CommandList::SetGraphicsRootDescriptorTable(UINT rootParamIndex, D3D12_GPU_DESCRIPTOR_HANDLE baseHandle)
{
	m_D3DCommandList->SetGraphicsRootDescriptorTable(rootParamIndex, baseHandle);
}

void CommandList::SetGraphicsRoot32BitConstant(UINT rootParamIndex, UINT srcData, UINT destOffsetIn32BitValues)
{
	m_D3DCommandList->SetGraphicsRoot32BitConstant(rootParamIndex, srcData, destOffsetIn32BitValues);
}

void CommandList::SetGraphicsRoot32BitConstants(UINT rootParamIndex, UINT num32BitValues, const void* pSrcData, UINT destOffsetIn32BitValues)
{
	m_D3DCommandList->SetGraphicsRoot32BitConstants(rootParamIndex, num32BitValues, pSrcData, destOffsetIn32BitValues);
}

void CommandList::SetGraphicsRootConstantBufferView(UINT rootParamIndex, Buffer* pBuffer)
{
	ID3D12Resource* pD3DResource = pBuffer->GetD3DObject();
	m_D3DCommandList->SetGraphicsRootConstantBufferView(rootParamIndex, pD3DResource->GetGPUVirtualAddress());
}

void CommandList::SetGraphicsRootShaderResourceView(UINT rootParamIndex, Buffer* pBuffer)
{
	ID3D12Resource* pD3DResource = pBuffer->GetD3DObject();
	m_D3DCommandList->SetGraphicsRootShaderResourceView(rootParamIndex, pD3DResource->GetGPUVirtualAddress());
}

void CommandList::SetComputeRootSignature(RootSignature* pRootSignature)
{
	m_D3DCommandList->SetComputeRootSignature(pRootSignature->GetD3DObject());
}

void CommandList::SetComputeRootDescriptorTable(UINT rootParamIndex, D3D12_GPU_DESCRIPTOR_HANDLE baseHandle)
{
	m_D3DCommandList->SetComputeRootDescriptorTable(rootParamIndex, baseHandle);
}

void CommandList::SetComputeRoot32BitConstant(UINT rootParamIndex, UINT srcData, UINT destOffsetIn32BitValues)
{
	m_D3DCommandList->SetComputeRoot32BitConstant(rootParamIndex, srcData, destOffsetIn32BitValues);
}

void CommandList::SetComputeRoot32BitConstants(UINT rootParamIndex, UINT num32BitValues, const void* pSrcData, UINT destOffsetIn32BitValues)
{
	m_D3DCommandList->SetComputeRoot32BitConstants(rootParamIndex, num32BitValues, pSrcData, destOffsetIn32BitValues);
}

void CommandList::SetComputeRootConstantBufferView(UINT rootParamIndex, Buffer* pBuffer)
{
	ID3D12Resource* pD3DResource = pBuffer->GetD3DObject();
	m_D3DCommandList->SetComputeRootConstantBufferView(rootParamIndex, pD3DResource->GetGPUVirtualAddress());
}

void CommandList::SetComputeRootShaderResourceView(UINT rootParamIndex, Buffer* pBuffer)
{
	ID3D12Resource* pD3DResource = pBuffer->GetD3DObject();
	m_D3DCommandList->SetComputeRootShaderResourceView(rootParamIndex, pD3DResource->GetGPUVirtualAddress());
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
		(pCountBuffer != nullptr) ? pCountBuffer->GetD3DObject() : 0, countBufferOffset);
}

void CommandList::BeginQuery(QueryHeap* pQueryHeap, D3D12_QUERY_TYPE type, UINT index)
{
	m_D3DCommandList->BeginQuery(pQueryHeap->GetD3DObject(), type, index);
}

void CommandList::EndQuery(QueryHeap* pQueryHeap, D3D12_QUERY_TYPE type, UINT index)
{
	m_D3DCommandList->EndQuery(pQueryHeap->GetD3DObject(), type, index);
}

void CommandList::ResolveQueryData(QueryHeap* pQueryHeap, D3D12_QUERY_TYPE type, UINT startIndex, UINT numQueries, Buffer* pDestBuffer, UINT64 alignedDestBufferOffset)
{
	m_D3DCommandList->ResolveQueryData(pQueryHeap->GetD3DObject(), type, startIndex, numQueries, pDestBuffer->GetD3DObject(), alignedDestBufferOffset);
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

void CommandList::SetCompletionFence(Fence* pFence, UINT64 fenceValue)
{
	m_pCompletionFence = pFence;
	m_CompletionFenceValue = fenceValue;
}

bool CommandList::CompletedExecution()
{
	return ((m_pCompletionFence != nullptr) && m_pCompletionFence->ReceivedSignal(m_CompletionFenceValue));
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

CommandList* CommandListPool::Create(LPCWSTR pName)
{
	CommandList* pCommandList = nullptr;
	if (!m_CommandListQueue.empty())
	{
		CommandList* pExistingCommandList = m_CommandListQueue.front();
		if (pExistingCommandList->CompletedExecution())
		{
			m_CommandListQueue.pop();
			pCommandList = pExistingCommandList;

#ifdef ENABLE_GRAPHICS_DEBUGGING
			pCommandList->SetName(pName);
#endif // ENABLE_GRAPHICS_DEBUGGING
		}
	}
	if (pCommandList == nullptr)
		pCommandList = new CommandList(m_pDevice, m_Type, pName);

	m_CommandListQueue.push(pCommandList);
	return pCommandList;
}
