#pragma once

#include "D3DWrapper/DXObject.h"
#include "D3DWrapper/D3DResourceList.h"

class D3DDevice;
class D3DBuffer;
class D3DCommandAllocator;
class D3DCommandSignature;
class D3DPipelineState;
class D3DRootSignature;
class D3DRootSignature;
class D3DResource;
class D3DDescriptorHeap;

struct D3DViewport;
struct D3DRect;
struct D3DVertexBufferView;
struct D3DIndexBufferView;

class D3DCommandList : public DXObject<ID3D12GraphicsCommandList>
{
public:
	D3DCommandList(D3DDevice* pDevice, D3DCommandAllocator* pCommandAllocator, D3DPipelineState* pInitialState, LPCWSTR pName);
	
	void Reset(D3DCommandAllocator* pAllocator, D3DPipelineState* pInitialState = nullptr);
	void Close();
				
	void IASetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY primitiveTopology);
	void IASetVertexBuffers(UINT startSlot, UINT numViews, const D3DVertexBufferView* pViews);
	void IASetIndexBuffer(const D3DIndexBufferView* pView);

	void SetGraphicsRootSignature(D3DRootSignature* pRootSignature);
	void SetGraphicsRootDescriptorTable(UINT rootParameterIndex, D3D12_GPU_DESCRIPTOR_HANDLE baseHandle);

	void SetComputeRootSignature(D3DRootSignature* pRootSignature);
	void SetComputeRootDescriptorTable(UINT rootParameterIndex, D3D12_GPU_DESCRIPTOR_HANDLE baseHandle);

	void SetDescriptorHeaps(D3DDescriptorHeap* pCBVSRVUAVDescriptorHeap, D3DDescriptorHeap* pSamplerDescriptorHeap = nullptr);

	void RSSetViewports(UINT numViewports, const D3DViewport* pViewports);
	void RSSetScissorRects(UINT numRects, const D3DRect* pRects);

	void OMSetRenderTargets(UINT numRenderTargets = 0, const D3D12_CPU_DESCRIPTOR_HANDLE* renderTargetViewHandles = nullptr,
		BOOL singleRenderTargetViewsRange = FALSE, const D3D12_CPU_DESCRIPTOR_HANDLE* pDepthStencilHandle = nullptr);
	
	void DrawInstanced(UINT vertexCountPerInstance, UINT instanceCount, UINT startVertexLocation, UINT startInstanceLocation);
	void DrawIndexedInstanced(UINT indexCountPerInstance, UINT instanceCount, UINT startIndexLocation, INT baseVertexLocation, UINT startInstanceLocation);
	void Dispatch(UINT numThreadGroupsX, UINT numThreadGroupsY, UINT numThreadGroupsZ);
	
	void ExecuteIndirect(D3DCommandSignature* pCommandSignature, UINT maxCommandCount,
		D3DBuffer* pArgumentBuffer, UINT64 argumentBufferOffset,
		D3DBuffer* pCountBuffer, UINT64 countBufferOffset);

	void ClearRenderTargetView(D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle, const FLOAT clearColor[4]);
	void ClearDepthStencilView(D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle, FLOAT depth = 1.0f, UINT8 stencil = 0);
	void ClearDepthView(D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle, FLOAT depth = 1.0f);
	void ClearStencilView(D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle, UINT8 stencil = 0);
	void ClearUnorderedAccessView(D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle, D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle, D3DResource* pResource, const UINT clearValue[4]);
	void ClearUnorderedAccessView(D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle, D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle, D3DResource* pResource, const FLOAT clearValue[4]);
	
	void CopyResource(D3DResource* pDestResouce, D3DResource* pSourceResource);
	void CopyBufferRegion(D3DBuffer* pDestBuffer, UINT64 destOffset, D3DBuffer* pSourceBuffer, UINT64 sourceOffset, UINT64 numBytes);

	void ResourceBarrier(UINT numBarriers, const D3D12_RESOURCE_BARRIER* pBarriers);

	D3DRequiredResourceStateList* GetResourceTransitions();
	void SetResourceTransitions(D3DRequiredResourceStateList* pResourceTransitions);
	
private:
	D3DRequiredResourceStateList* m_pResourceTransitions;
};

class D3DCommandListPool
{
public:
	D3DCommandListPool(D3DDevice* pDevice, D3D12_COMMAND_LIST_TYPE type);
	~D3DCommandListPool();

	D3DCommandList* Create(D3DCommandAllocator* pCommandAllocator, D3DPipelineState* pInitialState, LPCWSTR pName);
	void Release(D3DCommandList* pCommandList);

private:
	D3DDevice* m_pDevice;
	const D3D12_COMMAND_LIST_TYPE m_Type;
	std::queue<D3DCommandList*> m_CommandListQueue;
};
