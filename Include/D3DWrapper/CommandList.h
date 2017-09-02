#pragma once

#include "D3DWrapper/Common.h"

class GraphicsDevice;
class Buffer;
class CommandSignature;
class PipelineState;
class RootSignature;
class RootSignature;
class GraphicsResource;
class DescriptorHeap;
class Fence;

struct Viewport;
struct Rect;
struct VertexBufferView;
struct IndexBufferView;

class CommandList
{
public:
	CommandList(GraphicsDevice* pDevice, D3D12_COMMAND_LIST_TYPE type, LPCWSTR pName);
	
	ID3D12GraphicsCommandList* GetD3DObject() { return m_D3DCommandList.Get(); }
	void SetName(LPCWSTR pName);

	void Begin(PipelineState* pPipelineState = nullptr);
	void End();

	void SetPipelineState(PipelineState* pPipelineState);
				
	void IASetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY primitiveTopology);
	void IASetVertexBuffers(UINT startSlot, UINT numViews, const VertexBufferView* pViews);
	void IASetIndexBuffer(const IndexBufferView* pView);

	void SetGraphicsRootSignature(RootSignature* pRootSignature);
	void SetGraphicsRootDescriptorTable(UINT rootParamIndex, D3D12_GPU_DESCRIPTOR_HANDLE baseHandle);
	void SetGraphicsRoot32BitConstant(UINT rootParamIndex, UINT srcData, UINT destOffsetIn32BitValues);
	void SetGraphicsRootConstantBufferView(UINT rootParamIndex, Buffer* pBuffer);
	
	void SetComputeRootSignature(RootSignature* pRootSignature);
	void SetComputeRootDescriptorTable(UINT rootParamIndex, D3D12_GPU_DESCRIPTOR_HANDLE baseHandle);
	void SetComputeRoot32BitConstant(UINT rootParamIndex, UINT srcData, UINT destOffsetIn32BitValues);
	void SetComputeRootConstantBufferView(UINT rootParamIndex, Buffer* pBuffer);

	void SetDescriptorHeaps(DescriptorHeap* pSRVDescriptorHeap, DescriptorHeap* pSamplerDescriptorHeap = nullptr);

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
	
	void CopyResource(GraphicsResource* pDestResource, GraphicsResource* pSourceResource);

	void CopyTextureRegion(const D3D12_TEXTURE_COPY_LOCATION* pDestLocation, UINT destX, UINT destY, UINT destZ,
		const D3D12_TEXTURE_COPY_LOCATION* pSourceLocation, const D3D12_BOX* pSourceBox);

	void CopyBufferRegion(Buffer* pDestBuffer, UINT64 destOffsetInBytes, Buffer* pSourceBuffer, UINT64 sourceOffsetInBytes, UINT64 numBytesToCopy);

	void ResourceBarrier(UINT numBarriers, const D3D12_RESOURCE_BARRIER* pBarriers);
		
	void SetCompletionFence(Fence* pFence, UINT64 fenceValue);
	bool CompletedExecution();

private:
	ComPtr<ID3D12GraphicsCommandList> m_D3DCommandList;
	ComPtr<ID3D12CommandAllocator> m_D3DCommandAllocator;	
	Fence* m_pCompletionFence;
	UINT64 m_CompletionFenceValue;
};

class CommandListPool
{
public:
	CommandListPool(GraphicsDevice* pDevice, D3D12_COMMAND_LIST_TYPE type);
	~CommandListPool();

	CommandList* Create(LPCWSTR pName);

private:
	GraphicsDevice* m_pDevice;
	const D3D12_COMMAND_LIST_TYPE m_Type;
	std::queue<CommandList*> m_CommandListQueue;
};
