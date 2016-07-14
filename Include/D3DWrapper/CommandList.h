#pragma once

#include "D3DWrapper/BindingResourceList.h"

class GraphicsDevice;
class Buffer;
class CommandAllocator;
class CommandSignature;
class PipelineState;
class RootSignature;
class RootSignature;
class GraphicsResource;
class DescriptorHeap;

struct Viewport;
struct Rect;
struct VertexBufferView;
struct IndexBufferView;

class CommandList
{
public:
	CommandList(GraphicsDevice* pDevice, CommandAllocator* pCommandAllocator, PipelineState* pInitialState, LPCWSTR pName);
	
	ID3D12GraphicsCommandList* GetD3DObject() { return m_D3DCommandList.Get(); }
	void SetName(LPCWSTR pName);

	void Reset(CommandAllocator* pAllocator, PipelineState* pInitialState = nullptr);
	void Close();
				
	void IASetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY primitiveTopology);
	void IASetVertexBuffers(UINT startSlot, UINT numViews, const VertexBufferView* pViews);
	void IASetIndexBuffer(const IndexBufferView* pView);

	void SetGraphicsRootSignature(RootSignature* pRootSignature);
	void SetGraphicsRootDescriptorTable(UINT rootParameterIndex, D3D12_GPU_DESCRIPTOR_HANDLE baseHandle);

	void SetComputeRootSignature(RootSignature* pRootSignature);
	void SetComputeRootDescriptorTable(UINT rootParameterIndex, D3D12_GPU_DESCRIPTOR_HANDLE baseHandle);

	void SetDescriptorHeaps(DescriptorHeap* pCBVSRVUAVDescriptorHeap, DescriptorHeap* pSamplerDescriptorHeap = nullptr);

	void RSSetViewports(UINT numViewports, const Viewport* pViewports);
	void RSSetScissorRects(UINT numRects, const Rect* pRects);

	void OMSetRenderTargets(UINT numRenderTargets = 0, const D3D12_CPU_DESCRIPTOR_HANDLE* renderTargetViewHandles = nullptr,
		BOOL singleRenderTargetViewsRange = FALSE, const D3D12_CPU_DESCRIPTOR_HANDLE* pDepthStencilHandle = nullptr);
	
	void DrawInstanced(UINT vertexCountPerInstance, UINT instanceCount, UINT startVertexLocation, UINT startInstanceLocation);
	void DrawIndexedInstanced(UINT indexCountPerInstance, UINT instanceCount, UINT startIndexLocation, INT baseVertexLocation, UINT startInstanceLocation);
	void Dispatch(UINT numThreadGroupsX, UINT numThreadGroupsY, UINT numThreadGroupsZ);
	
	void ExecuteIndirect(CommandSignature* pCommandSignature, UINT maxCommandCount,
		Buffer* pArgumentBuffer, UINT64 argumentBufferOffset,
		Buffer* pCountBuffer, UINT64 countBufferOffset);

	void ClearRenderTargetView(D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle, const FLOAT clearColor[4]);
	void ClearDepthStencilView(D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle, FLOAT depth = 1.0f, UINT8 stencil = 0);
	void ClearDepthView(D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle, FLOAT depth = 1.0f);
	void ClearStencilView(D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle, UINT8 stencil = 0);
	void ClearUnorderedAccessView(D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle, D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle, GraphicsResource* pResource, const UINT clearValue[4]);
	void ClearUnorderedAccessView(D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle, D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle, GraphicsResource* pResource, const FLOAT clearValue[4]);
	
	void CopyResource(GraphicsResource* pDestResouce, GraphicsResource* pSourceResource);
	void CopyBufferRegion(Buffer* pDestBuffer, UINT64 destOffset, Buffer* pSourceBuffer, UINT64 sourceOffset, UINT64 numBytes);

	void ResourceBarrier(UINT numBarriers, const D3D12_RESOURCE_BARRIER* pBarriers);

	RequiredResourceStateList* GetRequiredResourceStates();
	void SetRequiredResourceStates(RequiredResourceStateList* pRequiredResourceStates);
	
private:
	ComPtr<ID3D12GraphicsCommandList> m_D3DCommandList;
	RequiredResourceStateList* m_pRequiredResourceStates;
};

class CommandListPool
{
public:
	CommandListPool(GraphicsDevice* pDevice, D3D12_COMMAND_LIST_TYPE type);
	~CommandListPool();

	CommandList* Create(CommandAllocator* pCommandAllocator, PipelineState* pInitialState, LPCWSTR pName);
	void Release(CommandList* pCommandList);

private:
	GraphicsDevice* m_pDevice;
	const D3D12_COMMAND_LIST_TYPE m_Type;
	std::queue<CommandList*> m_CommandListQueue;
};
