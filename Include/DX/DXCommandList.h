#pragma once

#include "DX/DXObject.h"
#include "DX/DXResourceList.h"

class DXDevice;
class DXBuffer;
class DXCommandAllocator;
class DXCommandSignature;
class DXPipelineState;
class DXRootSignature;
class DXRootSignature;
class DXResource;
class DXDescriptorHeap;

struct DXViewport;
struct DXRect;
struct DXVertexBufferView;
struct DXIndexBufferView;

class DXCommandList : public DXObject<ID3D12GraphicsCommandList>
{
public:
	DXCommandList(DXDevice* pDevice, DXCommandAllocator* pCommandAllocator, DXPipelineState* pInitialState, LPCWSTR pName);
	
	void Reset(DXCommandAllocator* pAllocator, DXPipelineState* pInitialState = nullptr);
	void Close();
				
	void IASetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY primitiveTopology);
	void IASetVertexBuffers(UINT startSlot, UINT numViews, const DXVertexBufferView* pViews);
	void IASetIndexBuffer(const DXIndexBufferView* pView);

	void SetGraphicsRootSignature(DXRootSignature* pRootSignature);
	void SetGraphicsRootDescriptorTable(UINT rootParameterIndex, D3D12_GPU_DESCRIPTOR_HANDLE baseHandle);

	void SetComputeRootSignature(DXRootSignature* pRootSignature);
	void SetComputeRootDescriptorTable(UINT rootParameterIndex, D3D12_GPU_DESCRIPTOR_HANDLE baseHandle);

	void SetDescriptorHeaps(DXDescriptorHeap* pCBVSRVUAVDescriptorHeap, DXDescriptorHeap* pSamplerDescriptorHeap = nullptr);

	void RSSetViewports(UINT numViewports, const DXViewport* pViewports);
	void RSSetScissorRects(UINT numRects, const DXRect* pRects);

	void OMSetRenderTargets(UINT numRenderTargets = 0, const D3D12_CPU_DESCRIPTOR_HANDLE* renderTargetViewHandles = nullptr,
		BOOL singleRenderTargetViewsRange = FALSE, const D3D12_CPU_DESCRIPTOR_HANDLE* pDepthStencilHandle = nullptr);
	
	void DrawInstanced(UINT vertexCountPerInstance, UINT instanceCount, UINT startVertexLocation, UINT startInstanceLocation);
	void DrawIndexedInstanced(UINT indexCountPerInstance, UINT instanceCount, UINT startIndexLocation, INT baseVertexLocation, UINT startInstanceLocation);
	void Dispatch(UINT numThreadGroupsX, UINT numThreadGroupsY, UINT numThreadGroupsZ);
	
	void ExecuteIndirect(DXCommandSignature* pCommandSignature, UINT maxCommandCount,
		DXBuffer* pArgumentBuffer, UINT64 argumentBufferOffset,
		DXBuffer* pCountBuffer, UINT64 countBufferOffset);

	void ClearRenderTargetView(D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle, const FLOAT clearColor[4]);
	void ClearDepthStencilView(D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle, FLOAT depth = 1.0f, UINT8 stencil = 0);
	void ClearDepthView(D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle, FLOAT depth = 1.0f);
	void ClearStencilView(D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle, UINT8 stencil = 0);
	void ClearUnorderedAccessView(D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle, D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle, DXResource* pResource, const UINT clearValue[4]);
	void ClearUnorderedAccessView(D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle, D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle, DXResource* pResource, const FLOAT clearValue[4]);
	
	void CopyResource(DXResource* pDest, DXResource* pSource);
	void ResourceBarrier(UINT numBarriers, const D3D12_RESOURCE_BARRIER* pBarriers);

	DXResourceTransitionList* GetResourceTransitions();
	void SetResourceTransitions(DXResourceTransitionList* pResourceTransitions);
	
private:
	DXResourceTransitionList* m_pResourceTransitions;
};

class DXCommandListPool
{
public:
	DXCommandListPool(DXDevice* pDevice, D3D12_COMMAND_LIST_TYPE type);
	~DXCommandListPool();

	DXCommandList* Create(DXCommandAllocator* pCommandAllocator, DXPipelineState* pInitialState, LPCWSTR pName);
	void Release(DXCommandList* pCommandList);

private:
	DXDevice* m_pDevice;
	const D3D12_COMMAND_LIST_TYPE m_Type;
	std::queue<DXCommandList*> m_CommandListQueue;
};
