#pragma once

#include "D3DWrapper/D3DDescriptorHeap.h"

class D3DDevice;
class D3DBuffer;
class D3DResource;

struct D3DRenderEnv;

DXGI_FORMAT GetRenderTargetViewFormat(DXGI_FORMAT resourceFormat);
DXGI_FORMAT GetDepthStencilViewFormat(DXGI_FORMAT resourceFormat);
DXGI_FORMAT GetShaderResourceViewFormat(DXGI_FORMAT resourceFormat);
DXGI_FORMAT GetUnorderedAccessViewFormat(DXGI_FORMAT resourceFormat);

struct D3DResourceTransitionBarrier : D3D12_RESOURCE_BARRIER
{
	D3DResourceTransitionBarrier(D3DResource* pResource, D3D12_RESOURCE_STATES stateBefore, D3D12_RESOURCE_STATES stateAfter,
		UINT subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, D3D12_RESOURCE_BARRIER_FLAGS flags = D3D12_RESOURCE_BARRIER_FLAG_NONE);
};

struct D3DDepthStencilValue : public D3D12_DEPTH_STENCIL_VALUE
{
	D3DDepthStencilValue(FLOAT depth = 1.0f, UINT8 stencil = 0);
};

struct D3DClearValue : D3D12_CLEAR_VALUE
{
	D3DClearValue(DXGI_FORMAT format, const FLOAT color[4]);
	D3DClearValue(DXGI_FORMAT format, const D3DDepthStencilValue* pDepthStencilValue);
};

struct D3DVertexBufferView : public D3D12_VERTEX_BUFFER_VIEW
{
	D3DVertexBufferView(D3D12_GPU_VIRTUAL_ADDRESS bufferLocation, UINT sizeInBytes, UINT strideInBytes);
};

struct D3DIndexBufferView : public D3D12_INDEX_BUFFER_VIEW
{
	D3DIndexBufferView(D3D12_GPU_VIRTUAL_ADDRESS bufferLocation, UINT sizeInBytes, UINT strideInBytes);
};

struct D3DConstantBufferViewDesc : public D3D12_CONSTANT_BUFFER_VIEW_DESC
{
	D3DConstantBufferViewDesc(D3D12_GPU_VIRTUAL_ADDRESS bufferLocation, UINT sizeInBytes);
};

struct D3DRange : public D3D12_RANGE
{
	D3DRange(SIZE_T begin, SIZE_T end);
};

struct D3DHeapProperties : public D3D12_HEAP_PROPERTIES
{
	D3DHeapProperties(D3D12_HEAP_TYPE type);
	D3DHeapProperties(D3D12_CPU_PAGE_PROPERTY cpuPageProperty, D3D12_MEMORY_POOL memoryPoolPreference);
};

struct D3DConstantBufferDesc : public D3D12_RESOURCE_DESC
{
	D3DConstantBufferDesc(UINT64 sizeInBytes, UINT64 alignment = 0);
};

struct D3DVertexBufferDesc : public D3D12_RESOURCE_DESC
{
	D3DVertexBufferDesc(UINT numVertices, UINT strideInBytes, UINT64 alignment = 0);

	UINT NumVertices;
	UINT StrideInBytes;
};

struct D3DIndexBufferDesc : public D3D12_RESOURCE_DESC
{
	D3DIndexBufferDesc(UINT numIndices, UINT strideInBytes, UINT64 alignment = 0);

	UINT NumIndices;
	UINT StrideInBytes;
};

struct D3DStructuredBufferDesc : public D3D12_RESOURCE_DESC
{
	D3DStructuredBufferDesc(UINT numElements, UINT structureByteStride, bool createSRV, bool createUAV, UINT64 alignment = 0);

	UINT NumElements;
	UINT StructureByteStride;
};

struct D3DStructuredBufferSRVDesc : public D3D12_SHADER_RESOURCE_VIEW_DESC
{
	D3DStructuredBufferSRVDesc(UINT64 firstElement, UINT numElements,
		UINT structureByteStride, UINT shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING);
};

struct D3DStructuredBufferUAVDesc : public D3D12_UNORDERED_ACCESS_VIEW_DESC
{
	D3DStructuredBufferUAVDesc(UINT64 firstElement, UINT numElements, UINT structureByteStride);
};

struct D3DFormattedBufferDesc : public D3D12_RESOURCE_DESC
{
	D3DFormattedBufferDesc(UINT numElements, DXGI_FORMAT format, bool createSRV, bool createUAV, UINT64 alignment = 0);

	UINT NumElements;
	DXGI_FORMAT SRVFormat;
	DXGI_FORMAT UAVFormat;
};

struct D3DFormattedBufferSRVDesc : public D3D12_SHADER_RESOURCE_VIEW_DESC
{
	D3DFormattedBufferSRVDesc(UINT64 firstElement, UINT numElements,
		DXGI_FORMAT format, UINT shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING);
};

struct D3DFormattedBufferUAVDesc : public D3D12_UNORDERED_ACCESS_VIEW_DESC
{
	D3DFormattedBufferUAVDesc(UINT64 firstElement, UINT numElements, DXGI_FORMAT format);
};

struct D3DRawBufferSRVDesc : public D3D12_SHADER_RESOURCE_VIEW_DESC
{
	D3DRawBufferSRVDesc(UINT64 firstElement, UINT numElements,
		UINT shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING);
};

struct D3DRawBufferUAVDesc : public D3D12_UNORDERED_ACCESS_VIEW_DESC
{
	D3DRawBufferUAVDesc(UINT64 firstElement, UINT numElements);
};

struct D3DCounterBufferUAVDesc : public D3D12_UNORDERED_ACCESS_VIEW_DESC
{
	D3DCounterBufferUAVDesc(UINT64 firstElement, UINT numElements,
		UINT structureByteStride, UINT64 counterOffsetInBytes);
};

struct D3DColorTexture1DDesc : public D3D12_RESOURCE_DESC
{
	D3DColorTexture1DDesc(DXGI_FORMAT format, UINT64 width,
		bool createRTV, bool createSRV, bool createUAV,
		UINT16 arraySize = 1, UINT16 mipLevels = 1,
		D3D12_TEXTURE_LAYOUT layout = D3D12_TEXTURE_LAYOUT_UNKNOWN,
		UINT64 alignment = 0);
};

struct D3DColorTexture2DDesc : public D3D12_RESOURCE_DESC
{
	D3DColorTexture2DDesc(DXGI_FORMAT format, UINT64 width, UINT height,
		bool createRTV, bool createSRV, bool createUAV,
		UINT16 arraySize = 1, UINT16 mipLevels = 1,
		UINT sampleCount = 1, UINT sampleQuality = 0,
		D3D12_TEXTURE_LAYOUT layout = D3D12_TEXTURE_LAYOUT_UNKNOWN,
		UINT64 alignment = 0);
};

struct D3DColorTexture3DDesc : public D3D12_RESOURCE_DESC
{
	D3DColorTexture3DDesc(DXGI_FORMAT format, UINT64 width, UINT height, UINT16 depth,
		bool createRTV, bool createSRV, bool createUAV,
		UINT16 mipLevels = 1,
		D3D12_TEXTURE_LAYOUT layout = D3D12_TEXTURE_LAYOUT_UNKNOWN,
		UINT64 alignment = 0);
};

struct D3DDepthTexture1DDesc : public D3D12_RESOURCE_DESC
{
	D3DDepthTexture1DDesc(DXGI_FORMAT format, UINT64 width,
		bool createDSV, bool createSRV,
		UINT16 arraySize = 1, UINT16 mipLevels = 1,
		D3D12_TEXTURE_LAYOUT layout = D3D12_TEXTURE_LAYOUT_UNKNOWN,
		UINT64 alignment = 0);
};

struct D3DDepthTexture2DDesc : public D3D12_RESOURCE_DESC
{
	D3DDepthTexture2DDesc(DXGI_FORMAT format, UINT64 width, UINT height,
		bool createDSV, bool createSRV,
		UINT16 arraySize = 1, UINT16 mipLevels = 1,
		UINT sampleCount = 1, UINT sampleQuality = 0,
		D3D12_TEXTURE_LAYOUT layout = D3D12_TEXTURE_LAYOUT_UNKNOWN,
		UINT64 alignment = 0);
};

struct D3DDepthTexture3DDesc : public D3D12_RESOURCE_DESC
{
	D3DDepthTexture3DDesc(DXGI_FORMAT format, UINT64 width, UINT height, UINT16 depth,
		bool createDSV, bool createSRV,
		UINT16 mipLevels = 1,
		D3D12_TEXTURE_LAYOUT layout = D3D12_TEXTURE_LAYOUT_UNKNOWN,
		UINT64 alignment = 0);
};

struct D3DTex1DRenderTargetViewDesc : public D3D12_RENDER_TARGET_VIEW_DESC
{
	D3DTex1DRenderTargetViewDesc(UINT mipSlice = 0, UINT arraySize = 1, DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN);
};

struct D3DTex2DRenderTargetViewDesc : public D3D12_RENDER_TARGET_VIEW_DESC
{
	D3DTex2DRenderTargetViewDesc(UINT mipSlice = 0, UINT arraySize = 1, bool multisampled = false,
		DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN, UINT planeSlice = 0);
};

struct D3DTex3DRenderTargetViewDesc : public D3D12_RENDER_TARGET_VIEW_DESC
{
	D3DTex3DRenderTargetViewDesc(UINT mipSlice = 0, UINT firstDepthSlice = 0,
		UINT depthSliceCount = -1, DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN);
};

struct D3DTex1DDepthStencilViewDesc : public D3D12_DEPTH_STENCIL_VIEW_DESC
{
	D3DTex1DDepthStencilViewDesc(UINT mipSlice = 0, UINT arraySize = 1,
		DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN, D3D12_DSV_FLAGS flags = D3D12_DSV_FLAG_NONE);
};

struct D3DTex2DDepthStencilViewDesc : public D3D12_DEPTH_STENCIL_VIEW_DESC
{
	D3DTex2DDepthStencilViewDesc(DXGI_FORMAT format, UINT mipSlice = 0, UINT arraySize = 1,
		bool multisampled = false, D3D12_DSV_FLAGS flags = D3D12_DSV_FLAG_NONE);
};

struct D3DTex1DShaderResourceViewDesc : public D3D12_SHADER_RESOURCE_VIEW_DESC
{
	D3DTex1DShaderResourceViewDesc(DXGI_FORMAT format, UINT mostDetailedMip = 0, UINT mipLevels = -1, UINT arraySize = 1,
		FLOAT minLODClamp = 0.0f, UINT shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING);
};

struct D3DTex2DShaderResourceViewDesc : public D3D12_SHADER_RESOURCE_VIEW_DESC
{
	D3DTex2DShaderResourceViewDesc(DXGI_FORMAT format, UINT mostDetailedMip = 0, UINT mipLevels = -1, UINT arraySize = 1, bool multisampled = false,
		FLOAT minLODClamp = 0.0f, UINT shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING, UINT planeSlice = 0);
};

struct D3DTex3DShaderResourceViewDesc : public D3D12_SHADER_RESOURCE_VIEW_DESC
{
	D3DTex3DShaderResourceViewDesc(DXGI_FORMAT format, UINT mostDetailedMip = 0, UINT mipLevels = -1,
		FLOAT minLODClamp = 0.0f, UINT shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING);
};

struct D3DTexCubeShaderResourceViewDesc : public D3D12_SHADER_RESOURCE_VIEW_DESC
{
	D3DTexCubeShaderResourceViewDesc(DXGI_FORMAT format, UINT mostDetailedMip = 0, UINT mipLevels = -1, UINT numCubes = 1,
		FLOAT minLODClamp = 0.0f, UINT shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING);
};

struct D3DTex1DUnorderedAccessViewDesc : public D3D12_UNORDERED_ACCESS_VIEW_DESC
{
	D3DTex1DUnorderedAccessViewDesc(DXGI_FORMAT format, UINT mipSlice = 0);
};

struct D3DTex2DUnorderedAccessViewDesc : public D3D12_UNORDERED_ACCESS_VIEW_DESC
{
	D3DTex2DUnorderedAccessViewDesc(DXGI_FORMAT format, UINT mipSlice = 0, UINT planeSlice = 0);
};

class D3DResource : public DXObject<ID3D12Resource>
{
protected:
	D3DResource(ID3D12Resource* pDXObject, D3D12_RESOURCE_STATES initialState);
	D3DResource(const D3D12_RESOURCE_DESC* pDesc, D3D12_RESOURCE_STATES initialState);

public:
	DXGI_FORMAT GetFormat() const { return m_Desc.Format; }
	
	D3D12_RESOURCE_STATES GetState() const { return m_State; }
	void SetState(D3D12_RESOURCE_STATES state) { m_State = state; }

	D3D12_RESOURCE_STATES GetReadState() const { return m_ReadState; }
	D3D12_RESOURCE_STATES GetWriteState() const { return m_WriteState; }
	
	template <typename T>
	void Write(const T* pInputData, SIZE_T numBytes);

protected:
	D3D12_RESOURCE_DESC m_Desc;
	D3D12_RESOURCE_STATES m_State;
	D3D12_RESOURCE_STATES m_ReadState;
	D3D12_RESOURCE_STATES m_WriteState;
};

template <typename T>
void D3DResource::Write(const T* pInputData, SIZE_T numBytes)
{
	const UINT subresource = 0;
	T* pResourceData = nullptr;

	const D3DRange readRange(0, 0);
	DXVerify(GetDXObject()->Map(subresource, &readRange, reinterpret_cast<void**>(&pResourceData)));
	
	std::memcpy(pResourceData, pInputData, numBytes);
	
	const D3DRange writtenRange(0, numBytes);
	GetDXObject()->Unmap(subresource, &writtenRange);
}

class D3DColorTexture : public D3DResource
{
public:
	D3DColorTexture(D3DRenderEnv* pRenderEnv, const D3D12_HEAP_PROPERTIES* pHeapProps,
		const D3DColorTexture1DDesc* pTexDesc, D3D12_RESOURCE_STATES initialState,
		const FLOAT optimizedClearColor[4], LPCWSTR pName);

	D3DColorTexture(D3DRenderEnv* pRenderEnv, const D3D12_HEAP_PROPERTIES* pHeapProps,
		const D3DColorTexture2DDesc* pTexDesc, D3D12_RESOURCE_STATES initialState,
		const FLOAT optimizedClearColor[4], LPCWSTR pName);

	D3DColorTexture(D3DRenderEnv* pRenderEnv, const D3D12_HEAP_PROPERTIES* pHeapProps,
		const D3DColorTexture3DDesc* pTexDesc, D3D12_RESOURCE_STATES initialState,
		const FLOAT optimizedClearColor[4], LPCWSTR pName);

	D3DColorTexture(D3DRenderEnv* pRenderEnv, ID3D12Resource* pDXObject,
		D3D12_RESOURCE_STATES initialState, LPCWSTR pName);

	UINT64 GetWidth() const { return m_Desc.Width; }
	UINT GetHeight() const { return m_Desc.Height; }

	D3DDescriptorHandle GetRTVHandle();
	D3DDescriptorHandle GetSRVHandle();
	D3DDescriptorHandle GetUAVHandle();
				
private:
	void CreateCommittedResource(D3DRenderEnv* pRenderEnv, const D3D12_HEAP_PROPERTIES* pHeapProps,
		const D3D12_RESOURCE_DESC* pTexDesc, D3D12_RESOURCE_STATES initialState,
		const FLOAT optimizedClearColor[4], LPCWSTR pName);

	void CreateTex1DViews(D3DRenderEnv* pRenderEnv, const D3D12_RESOURCE_DESC* pTexDesc);
	void CreateTex2DViews(D3DRenderEnv* pRenderEnv, const D3D12_RESOURCE_DESC* pTexDesc);
	void CreateTex3DViews(D3DRenderEnv* pRenderEnv, const D3D12_RESOURCE_DESC* pTexDesc);

	void DetermineResourceStates(const D3D12_RESOURCE_DESC* pTexDesc);

private:
	D3DDescriptorHandle m_RTVHandle;
	D3DDescriptorHandle m_SRVHandle;
	D3DDescriptorHandle m_UAVHandle;
};

class D3DDepthTexture : public D3DResource
{
public:
	D3DDepthTexture(D3DRenderEnv* pRenderEnv, const D3D12_HEAP_PROPERTIES* pHeapProps,
		const D3DDepthTexture1DDesc* pTexDesc, D3D12_RESOURCE_STATES initialState,
		const D3DDepthStencilValue* pOptimizedClearDepth, LPCWSTR pName);

	D3DDepthTexture(D3DRenderEnv* pRenderEnv, const D3D12_HEAP_PROPERTIES* pHeapProps,
		const D3DDepthTexture2DDesc* pTexDesc, D3D12_RESOURCE_STATES initialState,
		const D3DDepthStencilValue* pOptimizedClearDepth, LPCWSTR pName);

	D3DDepthTexture(D3DRenderEnv* pRenderEnv, const D3D12_HEAP_PROPERTIES* pHeapProps,
		const D3DDepthTexture3DDesc* pTexDesc, D3D12_RESOURCE_STATES initialState,
		const D3DDepthStencilValue* pOptimizedClearDepth, LPCWSTR pName);

	UINT64 GetWidth() const { return m_Desc.Width; }
	UINT GetHeight() const { return m_Desc.Height; }

	D3DDescriptorHandle GetDSVHandle();
	D3DDescriptorHandle GetSRVHandle();

private:
	void CreateCommittedResource(D3DRenderEnv* pRenderEnv, const D3D12_HEAP_PROPERTIES* pHeapProps,
		const D3D12_RESOURCE_DESC* pTexDesc, D3D12_RESOURCE_STATES initialState,
		const D3DDepthStencilValue* pOptimizedClearDepth, LPCWSTR pName);

	void CreateTex1DViews(D3DRenderEnv* pRenderEnv, const D3D12_RESOURCE_DESC* pTexDesc);
	void CreateTex2DViews(D3DRenderEnv* pRenderEnv, const D3D12_RESOURCE_DESC* pTexDesc);
	void CreateTex3DViews(D3DRenderEnv* pRenderEnv, const D3D12_RESOURCE_DESC* pTexDesc);

	void DetermineResourceStates(const D3D12_RESOURCE_DESC* pTexDesc);

private:
	D3DDescriptorHandle m_DSVHandle;
	D3DDescriptorHandle m_SRVHandle;
};

class D3DBuffer : public D3DResource
{
public:
	D3DBuffer(D3DRenderEnv* pRenderEnv, const D3D12_HEAP_PROPERTIES* pHeapProps,
		const D3DConstantBufferDesc* pBufferDesc, D3D12_RESOURCE_STATES initialState, LPCWSTR pName);

	D3DBuffer(D3DRenderEnv* pRenderEnv, const D3D12_HEAP_PROPERTIES* pHeapProps,
		const D3DVertexBufferDesc* pBufferDesc, D3D12_RESOURCE_STATES initialState, LPCWSTR pName);

	D3DBuffer(D3DRenderEnv* pRenderEnv, const D3D12_HEAP_PROPERTIES* pHeapProps,
		const D3DIndexBufferDesc* pBufferDesc, D3D12_RESOURCE_STATES initialState, LPCWSTR pName);

	D3DBuffer(D3DRenderEnv* pRenderEnv, const D3D12_HEAP_PROPERTIES* pHeapProps,
		const D3DStructuredBufferDesc* pBufferDesc, D3D12_RESOURCE_STATES initialState, LPCWSTR pName);

	D3DBuffer(D3DRenderEnv* pRenderEnv, const D3D12_HEAP_PROPERTIES* pHeapProps,
		const D3DFormattedBufferDesc* pBufferDesc, D3D12_RESOURCE_STATES initialState, LPCWSTR pName);
	
	~D3DBuffer();
	
	D3DVertexBufferView* GetVBView();
	D3DIndexBufferView*  GetIBView();

	D3DDescriptorHandle GetSRVHandle();
	D3DDescriptorHandle GetUAVHandle();
	D3DDescriptorHandle GetCBVHandle();
		
private:
	void CreateCommittedResource(D3DRenderEnv* pRenderEnv, const D3D12_HEAP_PROPERTIES* pHeapProps,
		const D3D12_RESOURCE_DESC* pBufferDesc, D3D12_RESOURCE_STATES initialState, LPCWSTR pName);

	void CreateConstantBufferView(D3DRenderEnv* pRenderEnv, const D3DConstantBufferDesc* pBufferDesc);
	void CreateVertexBufferView(D3DRenderEnv* pRenderEnv, const D3DVertexBufferDesc* pBufferDesc);
	void CreateIndexBufferView(D3DRenderEnv* pRenderEnv, const D3DIndexBufferDesc* pBufferDesc);
	void CreateStructuredBufferViews(D3DRenderEnv* pRenderEnv, const D3DStructuredBufferDesc* pBufferDesc);
	void CreateFormattedBufferViews(D3DRenderEnv* pRenderEnv, const D3DFormattedBufferDesc* pBufferDesc);
		
private:
	D3DDescriptorHandle m_SRVHandle;
	D3DDescriptorHandle m_UAVHandle;
	D3DDescriptorHandle m_CBVHandle;
	D3DVertexBufferView* m_pVBView;
	D3DIndexBufferView* m_pIBView;
};

class D3DSampler
{
public:
	D3DSampler(D3DRenderEnv* pRenderEnv, const D3D12_SAMPLER_DESC* pDesc);
	D3DDescriptorHandle GetHandle() { return m_Handle; }

private:
	D3DDescriptorHandle m_Handle;
};