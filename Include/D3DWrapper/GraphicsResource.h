#pragma once

#include "D3DWrapper/DescriptorHeap.h"
#include "D3DWrapper/CommandQueue.h"
#include "D3DWrapper/Fence.h"

class GraphicsDevice;
class GraphicsResource;
struct RenderEnv;

DXGI_FORMAT GetRenderTargetViewFormat(DXGI_FORMAT resourceFormat);
DXGI_FORMAT GetDepthStencilViewFormat(DXGI_FORMAT resourceFormat);
DXGI_FORMAT GetShaderResourceViewFormat(DXGI_FORMAT resourceFormat);
DXGI_FORMAT GetUnorderedAccessViewFormat(DXGI_FORMAT resourceFormat);

struct ResourceBarrier : D3D12_RESOURCE_BARRIER
{
	ResourceBarrier(GraphicsResource* pResource, D3D12_RESOURCE_STATES stateBefore, D3D12_RESOURCE_STATES stateAfter,
		UINT subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, D3D12_RESOURCE_BARRIER_FLAGS flags = D3D12_RESOURCE_BARRIER_FLAG_NONE);
};

struct DepthStencilValue : public D3D12_DEPTH_STENCIL_VALUE
{
	DepthStencilValue(FLOAT depth = 1.0f, UINT8 stencil = 0);
};

struct ClearValue : D3D12_CLEAR_VALUE
{
	ClearValue(DXGI_FORMAT format, const FLOAT color[4]);
	ClearValue(DXGI_FORMAT format, const DepthStencilValue* pDepthStencilValue);
};

struct VertexBufferView : public D3D12_VERTEX_BUFFER_VIEW
{
	VertexBufferView(D3D12_GPU_VIRTUAL_ADDRESS bufferLocation, UINT sizeInBytes, UINT strideInBytes);
};

struct IndexBufferView : public D3D12_INDEX_BUFFER_VIEW
{
	IndexBufferView(D3D12_GPU_VIRTUAL_ADDRESS bufferLocation, UINT sizeInBytes, UINT strideInBytes);
};

struct ConstantBufferViewDesc : public D3D12_CONSTANT_BUFFER_VIEW_DESC
{
	ConstantBufferViewDesc(D3D12_GPU_VIRTUAL_ADDRESS bufferLocation, UINT sizeInBytes);
};

struct MemoryRange : public D3D12_RANGE
{
	MemoryRange(SIZE_T begin, SIZE_T end);
};

struct HeapProperties : public D3D12_HEAP_PROPERTIES
{
	HeapProperties(D3D12_HEAP_TYPE type);
	HeapProperties(D3D12_CPU_PAGE_PROPERTY cpuPageProperty, D3D12_MEMORY_POOL memoryPoolPreference);
};

struct ConstantBufferDesc : public D3D12_RESOURCE_DESC
{
	ConstantBufferDesc(UINT64 sizeInBytes, UINT64 alignment = 0);
};

struct VertexBufferDesc : public D3D12_RESOURCE_DESC
{
	VertexBufferDesc(UINT numVertices, UINT strideInBytes, UINT64 alignment = 0);

	UINT NumVertices;
	UINT StrideInBytes;
};

struct IndexBufferDesc : public D3D12_RESOURCE_DESC
{
	IndexBufferDesc(UINT numIndices, UINT strideInBytes, UINT64 alignment = 0);

	UINT NumIndices;
	UINT StrideInBytes;
};

struct StructuredBufferDesc : public D3D12_RESOURCE_DESC
{
	StructuredBufferDesc(UINT numElements, UINT structureByteStride, bool createSRV, bool createUAV, UINT64 alignment = 0);

	UINT NumElements;
	UINT StructureByteStride;
};

struct StructuredBufferSRVDesc : public D3D12_SHADER_RESOURCE_VIEW_DESC
{
	StructuredBufferSRVDesc(UINT64 firstElement, UINT numElements,
		UINT structureByteStride, UINT shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING);
};

struct StructuredBufferUAVDesc : public D3D12_UNORDERED_ACCESS_VIEW_DESC
{
	StructuredBufferUAVDesc(UINT64 firstElement, UINT numElements, UINT structureByteStride);
};

struct FormattedBufferDesc : public D3D12_RESOURCE_DESC
{
	FormattedBufferDesc(UINT numElements, DXGI_FORMAT format, bool createSRV, bool createUAV, UINT64 alignment = 0);

	UINT NumElements;
	DXGI_FORMAT SRVFormat;
	DXGI_FORMAT UAVFormat;
};

struct FormattedBufferSRVDesc : public D3D12_SHADER_RESOURCE_VIEW_DESC
{
	FormattedBufferSRVDesc(UINT64 firstElement, UINT numElements,
		DXGI_FORMAT format, UINT shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING);
};

struct FormattedBufferUAVDesc : public D3D12_UNORDERED_ACCESS_VIEW_DESC
{
	FormattedBufferUAVDesc(UINT64 firstElement, UINT numElements, DXGI_FORMAT format);
};

struct RawBufferSRVDesc : public D3D12_SHADER_RESOURCE_VIEW_DESC
{
	RawBufferSRVDesc(UINT64 firstElement, UINT numElements,
		UINT shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING);
};

struct RawBufferUAVDesc : public D3D12_UNORDERED_ACCESS_VIEW_DESC
{
	RawBufferUAVDesc(UINT64 firstElement, UINT numElements);
};

struct CounterBufferUAVDesc : public D3D12_UNORDERED_ACCESS_VIEW_DESC
{
	CounterBufferUAVDesc(UINT64 firstElement, UINT numElements,
		UINT structureByteStride, UINT64 counterOffsetInBytes);
};

struct ColorTexture1DDesc : public D3D12_RESOURCE_DESC
{
	ColorTexture1DDesc(DXGI_FORMAT format, UINT64 width,
		bool createRTV, bool createSRV, bool createUAV,
		UINT16 arraySize = 1, UINT16 mipLevels = 1,
		D3D12_TEXTURE_LAYOUT layout = D3D12_TEXTURE_LAYOUT_UNKNOWN,
		UINT64 alignment = 0);
};

struct ColorTexture2DDesc : public D3D12_RESOURCE_DESC
{
	ColorTexture2DDesc(DXGI_FORMAT format, UINT64 width, UINT height,
		bool createRTV, bool createSRV, bool createUAV,
		UINT16 arraySize = 1, UINT16 mipLevels = 1,
		UINT sampleCount = 1, UINT sampleQuality = 0,
		D3D12_TEXTURE_LAYOUT layout = D3D12_TEXTURE_LAYOUT_UNKNOWN,
		UINT64 alignment = 0);
};

struct ColorTexture3DDesc : public D3D12_RESOURCE_DESC
{
	ColorTexture3DDesc(DXGI_FORMAT format, UINT64 width, UINT height, UINT16 depth,
		bool createRTV, bool createSRV, bool createUAV,
		UINT16 mipLevels = 1,
		D3D12_TEXTURE_LAYOUT layout = D3D12_TEXTURE_LAYOUT_UNKNOWN,
		UINT64 alignment = 0);
};

struct DepthTexture1DDesc : public D3D12_RESOURCE_DESC
{
	DepthTexture1DDesc(DXGI_FORMAT format, UINT64 width,
		bool createDSV, bool createSRV,
		UINT16 arraySize = 1, UINT16 mipLevels = 1,
		D3D12_TEXTURE_LAYOUT layout = D3D12_TEXTURE_LAYOUT_UNKNOWN,
		UINT64 alignment = 0);
};

struct DepthTexture2DDesc : public D3D12_RESOURCE_DESC
{
	DepthTexture2DDesc(DXGI_FORMAT format, UINT64 width, UINT height,
		bool createDSV, bool createSRV,
		UINT16 arraySize = 1, UINT16 mipLevels = 1,
		UINT sampleCount = 1, UINT sampleQuality = 0,
		D3D12_TEXTURE_LAYOUT layout = D3D12_TEXTURE_LAYOUT_UNKNOWN,
		UINT64 alignment = 0);
};

struct DepthTexture3DDesc : public D3D12_RESOURCE_DESC
{
	DepthTexture3DDesc(DXGI_FORMAT format, UINT64 width, UINT height, UINT16 depth,
		bool createDSV, bool createSRV,
		UINT16 mipLevels = 1,
		D3D12_TEXTURE_LAYOUT layout = D3D12_TEXTURE_LAYOUT_UNKNOWN,
		UINT64 alignment = 0);
};

struct Tex1DRenderTargetViewDesc : public D3D12_RENDER_TARGET_VIEW_DESC
{
	Tex1DRenderTargetViewDesc(UINT mipSlice = 0, UINT arraySize = 1, DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN);
};

struct Tex2DRenderTargetViewDesc : public D3D12_RENDER_TARGET_VIEW_DESC
{
	Tex2DRenderTargetViewDesc(UINT mipSlice = 0, UINT arraySize = 1, bool multisampled = false,
		DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN, UINT planeSlice = 0);
};

struct Tex3DRenderTargetViewDesc : public D3D12_RENDER_TARGET_VIEW_DESC
{
	Tex3DRenderTargetViewDesc(UINT mipSlice = 0, UINT firstDepthSlice = 0,
		UINT depthSliceCount = -1, DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN);
};

struct Tex1DDepthStencilViewDesc : public D3D12_DEPTH_STENCIL_VIEW_DESC
{
	Tex1DDepthStencilViewDesc(UINT mipSlice = 0, UINT arraySize = 1,
		DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN, D3D12_DSV_FLAGS flags = D3D12_DSV_FLAG_NONE);
};

struct Tex2DDepthStencilViewDesc : public D3D12_DEPTH_STENCIL_VIEW_DESC
{
	Tex2DDepthStencilViewDesc(DXGI_FORMAT format, UINT mipSlice = 0, UINT arraySize = 1,
		bool multisampled = false, D3D12_DSV_FLAGS flags = D3D12_DSV_FLAG_NONE);
};

struct Tex1DShaderResourceViewDesc : public D3D12_SHADER_RESOURCE_VIEW_DESC
{
	Tex1DShaderResourceViewDesc(DXGI_FORMAT format, UINT mostDetailedMip = 0, UINT mipLevels = -1, UINT arraySize = 1,
		FLOAT minLODClamp = 0.0f, UINT shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING);
};

struct Tex2DShaderResourceViewDesc : public D3D12_SHADER_RESOURCE_VIEW_DESC
{
	Tex2DShaderResourceViewDesc(DXGI_FORMAT format, UINT mostDetailedMip = 0, UINT mipLevels = -1, UINT arraySize = 1, bool multisampled = false,
		FLOAT minLODClamp = 0.0f, UINT shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING, UINT planeSlice = 0);
};

struct Tex3DShaderResourceViewDesc : public D3D12_SHADER_RESOURCE_VIEW_DESC
{
	Tex3DShaderResourceViewDesc(DXGI_FORMAT format, UINT mostDetailedMip = 0, UINT mipLevels = -1,
		FLOAT minLODClamp = 0.0f, UINT shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING);
};

struct TexCubeShaderResourceViewDesc : public D3D12_SHADER_RESOURCE_VIEW_DESC
{
	TexCubeShaderResourceViewDesc(DXGI_FORMAT format, UINT mostDetailedMip = 0, UINT mipLevels = -1, UINT numCubes = 1,
		FLOAT minLODClamp = 0.0f, UINT shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING);
};

struct Tex1DUnorderedAccessViewDesc : public D3D12_UNORDERED_ACCESS_VIEW_DESC
{
	Tex1DUnorderedAccessViewDesc(DXGI_FORMAT format, UINT mipSlice = 0);
};

struct Tex2DUnorderedAccessViewDesc : public D3D12_UNORDERED_ACCESS_VIEW_DESC
{
	Tex2DUnorderedAccessViewDesc(DXGI_FORMAT format, UINT mipSlice = 0, UINT planeSlice = 0);
};

struct Tex3DUnorderedAccessViewDesc : public D3D12_UNORDERED_ACCESS_VIEW_DESC
{
	Tex3DUnorderedAccessViewDesc(DXGI_FORMAT format, UINT mipSlice = 0, UINT firstDepthSlice = 0, UINT depthSliceCount = -1);
};

class GraphicsResource
{
protected:
	GraphicsResource(ComPtr<ID3D12Resource> d3dResource, D3D12_RESOURCE_STATES initialState);
	GraphicsResource(const D3D12_RESOURCE_DESC* pDesc, D3D12_RESOURCE_STATES initialState);

public:
	ID3D12Resource* GetD3DObject() { return m_D3DResource.Get(); }
	DXGI_FORMAT GetFormat() const { return m_Desc.Format; }
	
	D3D12_RESOURCE_STATES GetState() const { return m_State; }
	void SetState(D3D12_RESOURCE_STATES state) { m_State = state; }

	D3D12_RESOURCE_STATES GetReadState() const { return m_ReadState; }
	D3D12_RESOURCE_STATES GetWriteState() const { return m_WriteState; }
	
	void Write(const void* pInputData, SIZE_T numBytes);
	void Read(void* pOutputData, SIZE_T numBytes);

protected:
	ComPtr<ID3D12Resource> m_D3DResource;
	D3D12_RESOURCE_DESC m_Desc;
	D3D12_RESOURCE_STATES m_State;
	D3D12_RESOURCE_STATES m_ReadState;
	D3D12_RESOURCE_STATES m_WriteState;
};

class ColorTexture : public GraphicsResource
{
public:
	ColorTexture(RenderEnv* pRenderEnv, const D3D12_HEAP_PROPERTIES* pHeapProps,
		const ColorTexture1DDesc* pTexDesc, D3D12_RESOURCE_STATES initialState,
		const FLOAT optimizedClearColor[4], LPCWSTR pName);

	ColorTexture(RenderEnv* pRenderEnv, const D3D12_HEAP_PROPERTIES* pHeapProps,
		const ColorTexture2DDesc* pTexDesc, D3D12_RESOURCE_STATES initialState,
		const FLOAT optimizedClearColor[4], LPCWSTR pName);

	ColorTexture(RenderEnv* pRenderEnv, const D3D12_HEAP_PROPERTIES* pHeapProps,
		const ColorTexture3DDesc* pTexDesc, D3D12_RESOURCE_STATES initialState,
		const FLOAT optimizedClearColor[4], LPCWSTR pName);

	ColorTexture(RenderEnv* pRenderEnv, ComPtr<ID3D12Resource> d3dResource,
		D3D12_RESOURCE_STATES initialState, LPCWSTR pName);

	UINT64 GetWidth() const { return m_Desc.Width; }
	UINT GetHeight() const { return m_Desc.Height; }
	UINT16 GetDepthOrArraySize() const { return m_Desc.DepthOrArraySize; }

	DescriptorHandle GetRTVHandle();
	DescriptorHandle GetSRVHandle();
	DescriptorHandle GetUAVHandle();
				
private:
	void CreateCommittedResource(RenderEnv* pRenderEnv, const D3D12_HEAP_PROPERTIES* pHeapProps,
		const D3D12_RESOURCE_DESC* pTexDesc, D3D12_RESOURCE_STATES initialState,
		const FLOAT optimizedClearColor[4], LPCWSTR pName);

	void CreateTex1DViews(RenderEnv* pRenderEnv, const D3D12_RESOURCE_DESC* pTexDesc);
	void CreateTex2DViews(RenderEnv* pRenderEnv, const D3D12_RESOURCE_DESC* pTexDesc);
	void CreateTex3DViews(RenderEnv* pRenderEnv, const D3D12_RESOURCE_DESC* pTexDesc);

	void DetermineResourceStates(const D3D12_RESOURCE_DESC* pTexDesc);

private:
	DescriptorHandle m_RTVHandle;
	DescriptorHandle m_SRVHandle;
	DescriptorHandle m_UAVHandle;
};

class DepthTexture : public GraphicsResource
{
public:
	DepthTexture(RenderEnv* pRenderEnv, const D3D12_HEAP_PROPERTIES* pHeapProps,
		const DepthTexture1DDesc* pTexDesc, D3D12_RESOURCE_STATES initialState,
		const DepthStencilValue* pOptimizedClearDepth, LPCWSTR pName);

	DepthTexture(RenderEnv* pRenderEnv, const D3D12_HEAP_PROPERTIES* pHeapProps,
		const DepthTexture2DDesc* pTexDesc, D3D12_RESOURCE_STATES initialState,
		const DepthStencilValue* pOptimizedClearDepth, LPCWSTR pName);

	DepthTexture(RenderEnv* pRenderEnv, const D3D12_HEAP_PROPERTIES* pHeapProps,
		const DepthTexture3DDesc* pTexDesc, D3D12_RESOURCE_STATES initialState,
		const DepthStencilValue* pOptimizedClearDepth, LPCWSTR pName);

	UINT64 GetWidth() const { return m_Desc.Width; }
	UINT GetHeight() const { return m_Desc.Height; }

	DescriptorHandle GetDSVHandle();
	DescriptorHandle GetSRVHandle();

private:
	void CreateCommittedResource(RenderEnv* pRenderEnv, const D3D12_HEAP_PROPERTIES* pHeapProps,
		const D3D12_RESOURCE_DESC* pTexDesc, D3D12_RESOURCE_STATES initialState,
		const DepthStencilValue* pOptimizedClearDepth, LPCWSTR pName);

	void CreateTex1DViews(RenderEnv* pRenderEnv, const D3D12_RESOURCE_DESC* pTexDesc);
	void CreateTex2DViews(RenderEnv* pRenderEnv, const D3D12_RESOURCE_DESC* pTexDesc);
	void CreateTex3DViews(RenderEnv* pRenderEnv, const D3D12_RESOURCE_DESC* pTexDesc);

	void DetermineResourceStates(const D3D12_RESOURCE_DESC* pTexDesc);

private:
	DescriptorHandle m_DSVHandle;
	DescriptorHandle m_SRVHandle;
};

class Buffer : public GraphicsResource
{
public:
	Buffer(RenderEnv* pRenderEnv, const D3D12_HEAP_PROPERTIES* pHeapProps,
		const ConstantBufferDesc* pBufferDesc, D3D12_RESOURCE_STATES initialState, LPCWSTR pName);

	Buffer(RenderEnv* pRenderEnv, const D3D12_HEAP_PROPERTIES* pHeapProps,
		const VertexBufferDesc* pBufferDesc, D3D12_RESOURCE_STATES initialState, LPCWSTR pName);

	Buffer(RenderEnv* pRenderEnv, const D3D12_HEAP_PROPERTIES* pHeapProps,
		const IndexBufferDesc* pBufferDesc, D3D12_RESOURCE_STATES initialState, LPCWSTR pName);

	Buffer(RenderEnv* pRenderEnv, const D3D12_HEAP_PROPERTIES* pHeapProps,
		const StructuredBufferDesc* pBufferDesc, D3D12_RESOURCE_STATES initialState, LPCWSTR pName);

	Buffer(RenderEnv* pRenderEnv, const D3D12_HEAP_PROPERTIES* pHeapProps,
		const FormattedBufferDesc* pBufferDesc, D3D12_RESOURCE_STATES initialState, LPCWSTR pName);
	
	~Buffer();
	
	VertexBufferView* GetVBView();
	IndexBufferView*  GetIBView();

	DescriptorHandle GetSRVHandle();
	DescriptorHandle GetUAVHandle();
	DescriptorHandle GetCBVHandle();
		
private:
	void CreateCommittedResource(RenderEnv* pRenderEnv, const D3D12_HEAP_PROPERTIES* pHeapProps,
		const D3D12_RESOURCE_DESC* pBufferDesc, D3D12_RESOURCE_STATES initialState, LPCWSTR pName);

	void CreateConstantBufferView(RenderEnv* pRenderEnv, const ConstantBufferDesc* pBufferDesc);
	void CreateVertexBufferView(RenderEnv* pRenderEnv, const VertexBufferDesc* pBufferDesc);
	void CreateIndexBufferView(RenderEnv* pRenderEnv, const IndexBufferDesc* pBufferDesc);
	void CreateStructuredBufferViews(RenderEnv* pRenderEnv, const StructuredBufferDesc* pBufferDesc);
	void CreateFormattedBufferViews(RenderEnv* pRenderEnv, const FormattedBufferDesc* pBufferDesc);
		
private:
	DescriptorHandle m_SRVHandle;
	DescriptorHandle m_UAVHandle;
	DescriptorHandle m_CBVHandle;
	VertexBufferView* m_pVBView;
	IndexBufferView* m_pIBView;
};

class Sampler
{
public:
	Sampler(RenderEnv* pRenderEnv, const D3D12_SAMPLER_DESC* pDesc);
	DescriptorHandle GetHandle() { return m_Handle; }

private:
	DescriptorHandle m_Handle;
};

template <typename DestBufferDesc>
void UploadData(RenderEnv* pRenderEnv, Buffer* pDestBuffer, const DestBufferDesc* pDestBufferDesc, const void* pUploadData, SIZE_T numUploadBytes)
{
	assert(pDestBuffer->GetState() == D3D12_RESOURCE_STATE_COPY_DEST);

	Buffer* pUploadBuffer = new Buffer(pRenderEnv, pRenderEnv->m_pUploadHeapProps, pDestBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, L"MeshRenderResources::pUploadBuffer");
	pUploadBuffer->Write(pUploadData, numUploadBytes);

	ResourceBarrier resourceBarrier(pDestBuffer, pDestBuffer->GetState(), pDestBuffer->GetReadState());

	CommandList* pUploadCommandList = pRenderEnv->m_pCommandListPool->Create(L"pUploadCommandList");
	pUploadCommandList->Begin();
	pUploadCommandList->CopyResource(pDestBuffer, pUploadBuffer);
	pUploadCommandList->ResourceBarrier(1, &resourceBarrier);
	pUploadCommandList->End();

	++pRenderEnv->m_LastSubmissionFenceValue;
	pRenderEnv->m_pCommandQueue->ExecuteCommandLists(pRenderEnv, 1, &pUploadCommandList, pRenderEnv->m_pFence, pRenderEnv->m_LastSubmissionFenceValue);
	pRenderEnv->m_pFence->WaitForSignalOnCPU(pRenderEnv->m_LastSubmissionFenceValue);

	pDestBuffer->SetState(pDestBuffer->GetReadState());
	SafeDelete(pUploadBuffer);
}