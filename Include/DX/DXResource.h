#pragma once

#include "DX/DXDescriptorHeap.h"

class DXDevice;
class DXBuffer;
class DXResource;

struct DXRenderEnvironment;

DXGI_FORMAT GetRenderTargetViewFormat(DXGI_FORMAT resourceFormat);
DXGI_FORMAT GetDepthStencilViewFormat(DXGI_FORMAT resourceFormat);
DXGI_FORMAT GetShaderResourceViewFormat(DXGI_FORMAT resourceFormat);
DXGI_FORMAT GetUnorderedAccessViewFormat(DXGI_FORMAT resourceFormat);

struct DXResourceTransitionBarrier : D3D12_RESOURCE_BARRIER
{
	DXResourceTransitionBarrier(DXResource* pResource, D3D12_RESOURCE_STATES stateBefore, D3D12_RESOURCE_STATES stateAfter,
		UINT subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, D3D12_RESOURCE_BARRIER_FLAGS flags = D3D12_RESOURCE_BARRIER_FLAG_NONE);
};

struct DXDepthStencilValue : public D3D12_DEPTH_STENCIL_VALUE
{
	DXDepthStencilValue(FLOAT depth = 1.0f, UINT8 stencil = 0);
};

struct DXClearValue : D3D12_CLEAR_VALUE
{
	DXClearValue(DXGI_FORMAT format, const FLOAT color[4]);
	DXClearValue(DXGI_FORMAT format, const DXDepthStencilValue* pDepthStencilValue);
};

struct DXVertexBufferView : public D3D12_VERTEX_BUFFER_VIEW
{
	DXVertexBufferView(D3D12_GPU_VIRTUAL_ADDRESS bufferLocation, UINT sizeInBytes, UINT strideInBytes);
};

struct DXIndexBufferView : public D3D12_INDEX_BUFFER_VIEW
{
	DXIndexBufferView(D3D12_GPU_VIRTUAL_ADDRESS bufferLocation, UINT sizeInBytes, UINT strideInBytes);
};

struct DXConstantBufferViewDesc : public D3D12_CONSTANT_BUFFER_VIEW_DESC
{
	DXConstantBufferViewDesc(D3D12_GPU_VIRTUAL_ADDRESS bufferLocation, UINT sizeInBytes);
};

struct DXRange : public D3D12_RANGE
{
	DXRange(SIZE_T begin, SIZE_T end);
};

struct DXHeapProperties : public D3D12_HEAP_PROPERTIES
{
	DXHeapProperties(D3D12_HEAP_TYPE type);
	DXHeapProperties(D3D12_CPU_PAGE_PROPERTY cpuPageProperty, D3D12_MEMORY_POOL memoryPoolPreference);
};

struct DXConstantBufferDesc : public D3D12_RESOURCE_DESC
{
	DXConstantBufferDesc(UINT64 sizeInBytes, UINT64 alignment = 0);
};

struct DXVertexBufferDesc : public D3D12_RESOURCE_DESC
{
	DXVertexBufferDesc(UINT numVertices, UINT strideInBytes, UINT64 alignment = 0);

	UINT NumVertices;
	UINT StrideInBytes;
};

struct DXIndexBufferDesc : public D3D12_RESOURCE_DESC
{
	DXIndexBufferDesc(UINT numIndices, UINT strideInBytes, UINT64 alignment = 0);

	UINT NumIndices;
	UINT StrideInBytes;
};

struct DXStructuredBufferDesc : public D3D12_RESOURCE_DESC
{
	DXStructuredBufferDesc(UINT numElements, UINT structureByteStride, bool createSRV, bool createUAV, UINT64 alignment = 0);

	UINT NumElements;
	UINT StructureByteStride;
};

struct DXStructuredBufferSRVDesc : public D3D12_SHADER_RESOURCE_VIEW_DESC
{
	DXStructuredBufferSRVDesc(UINT64 firstElement, UINT numElements,
		UINT structureByteStride, UINT shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING);
};

struct DXStructuredBufferUAVDesc : public D3D12_UNORDERED_ACCESS_VIEW_DESC
{
	DXStructuredBufferUAVDesc(UINT64 firstElement, UINT numElements, UINT structureByteStride);
};

struct DXFormattedBufferDesc : public D3D12_RESOURCE_DESC
{
	DXFormattedBufferDesc(UINT numElements, DXGI_FORMAT format, bool createSRV, bool createUAV, UINT64 alignment = 0);

	UINT NumElements;
	DXGI_FORMAT SRVFormat;
	DXGI_FORMAT UAVFormat;
};

struct DXFormattedBufferSRVDesc : public D3D12_SHADER_RESOURCE_VIEW_DESC
{
	DXFormattedBufferSRVDesc(UINT64 firstElement, UINT numElements,
		DXGI_FORMAT format, UINT shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING);
};

struct DXFormattedBufferUAVDesc : public D3D12_UNORDERED_ACCESS_VIEW_DESC
{
	DXFormattedBufferUAVDesc(UINT64 firstElement, UINT numElements, DXGI_FORMAT format);
};

struct DXRawBufferSRVDesc : public D3D12_SHADER_RESOURCE_VIEW_DESC
{
	DXRawBufferSRVDesc(UINT64 firstElement, UINT numElements,
		UINT shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING);
};

struct DXRawBufferUAVDesc : public D3D12_UNORDERED_ACCESS_VIEW_DESC
{
	DXRawBufferUAVDesc(UINT64 firstElement, UINT numElements);
};

struct DXCounterBufferUAVDesc : public D3D12_UNORDERED_ACCESS_VIEW_DESC
{
	DXCounterBufferUAVDesc(UINT64 firstElement, UINT numElements,
		UINT structureByteStride, UINT64 counterOffsetInBytes);
};

struct DXColorTexture1DDesc : public D3D12_RESOURCE_DESC
{
	DXColorTexture1DDesc(DXGI_FORMAT format, UINT64 width,
		bool createRTV, bool createSRV, bool createUAV,
		UINT16 arraySize = 1, UINT16 mipLevels = 1,
		D3D12_TEXTURE_LAYOUT layout = D3D12_TEXTURE_LAYOUT_UNKNOWN,
		UINT64 alignment = 0);
};

struct DXColorTexture2DDesc : public D3D12_RESOURCE_DESC
{
	DXColorTexture2DDesc(DXGI_FORMAT format, UINT64 width, UINT height,
		bool createRTV, bool createSRV, bool createUAV,
		UINT16 arraySize = 1, UINT16 mipLevels = 1,
		UINT sampleCount = 1, UINT sampleQuality = 0,
		D3D12_TEXTURE_LAYOUT layout = D3D12_TEXTURE_LAYOUT_UNKNOWN,
		UINT64 alignment = 0);
};

struct DXColorTexture3DDesc : public D3D12_RESOURCE_DESC
{
	DXColorTexture3DDesc(DXGI_FORMAT format, UINT64 width, UINT height, UINT16 depth,
		bool createRTV, bool createSRV, bool createUAV,
		UINT16 mipLevels = 1,
		D3D12_TEXTURE_LAYOUT layout = D3D12_TEXTURE_LAYOUT_UNKNOWN,
		UINT64 alignment = 0);
};

struct DXDepthTexture1DDesc : public D3D12_RESOURCE_DESC
{
	DXDepthTexture1DDesc(DXGI_FORMAT format, UINT64 width,
		bool createDSV, bool createSRV,
		UINT16 arraySize = 1, UINT16 mipLevels = 1,
		D3D12_TEXTURE_LAYOUT layout = D3D12_TEXTURE_LAYOUT_UNKNOWN,
		UINT64 alignment = 0);
};

struct DXDepthTexture2DDesc : public D3D12_RESOURCE_DESC
{
	DXDepthTexture2DDesc(DXGI_FORMAT format, UINT64 width, UINT height,
		bool createDSV, bool createSRV,
		UINT16 arraySize = 1, UINT16 mipLevels = 1,
		UINT sampleCount = 1, UINT sampleQuality = 0,
		D3D12_TEXTURE_LAYOUT layout = D3D12_TEXTURE_LAYOUT_UNKNOWN,
		UINT64 alignment = 0);
};

struct DXDepthTexture3DDesc : public D3D12_RESOURCE_DESC
{
	DXDepthTexture3DDesc(DXGI_FORMAT format, UINT64 width, UINT height, UINT16 depth,
		bool createDSV, bool createSRV,
		UINT16 mipLevels = 1,
		D3D12_TEXTURE_LAYOUT layout = D3D12_TEXTURE_LAYOUT_UNKNOWN,
		UINT64 alignment = 0);
};

struct DXTex1DRenderTargetViewDesc : public D3D12_RENDER_TARGET_VIEW_DESC
{
	DXTex1DRenderTargetViewDesc(UINT mipSlice = 0, UINT arraySize = 1, DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN);
};

struct DXTex2DRenderTargetViewDesc : public D3D12_RENDER_TARGET_VIEW_DESC
{
	DXTex2DRenderTargetViewDesc(UINT mipSlice = 0, UINT arraySize = 1, bool multisampled = false,
		DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN, UINT planeSlice = 0);
};

struct DXTex3DRenderTargetViewDesc : public D3D12_RENDER_TARGET_VIEW_DESC
{
	DXTex3DRenderTargetViewDesc(UINT mipSlice = 0, UINT firstDepthSlice = 0,
		UINT depthSliceCount = -1, DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN);
};

struct DXTex1DDepthStencilViewDesc : public D3D12_DEPTH_STENCIL_VIEW_DESC
{
	DXTex1DDepthStencilViewDesc(UINT mipSlice = 0, UINT arraySize = 1,
		DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN, D3D12_DSV_FLAGS flags = D3D12_DSV_FLAG_NONE);
};

struct DXTex2DDepthStencilViewDesc : public D3D12_DEPTH_STENCIL_VIEW_DESC
{
	DXTex2DDepthStencilViewDesc(DXGI_FORMAT format, UINT mipSlice = 0, UINT arraySize = 1,
		bool multisampled = false, D3D12_DSV_FLAGS flags = D3D12_DSV_FLAG_NONE);
};

struct DXTex1DShaderResourceViewDesc : public D3D12_SHADER_RESOURCE_VIEW_DESC
{
	DXTex1DShaderResourceViewDesc(DXGI_FORMAT format, UINT mostDetailedMip = 0, UINT mipLevels = -1, UINT arraySize = 1,
		FLOAT minLODClamp = 0.0f, UINT shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING);
};

struct DXTex2DShaderResourceViewDesc : public D3D12_SHADER_RESOURCE_VIEW_DESC
{
	DXTex2DShaderResourceViewDesc(DXGI_FORMAT format, UINT mostDetailedMip = 0, UINT mipLevels = -1, UINT arraySize = 1, bool multisampled = false,
		FLOAT minLODClamp = 0.0f, UINT shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING, UINT planeSlice = 0);
};

struct DXTex3DShaderResourceViewDesc : public D3D12_SHADER_RESOURCE_VIEW_DESC
{
	DXTex3DShaderResourceViewDesc(DXGI_FORMAT format, UINT mostDetailedMip = 0, UINT mipLevels = -1,
		FLOAT minLODClamp = 0.0f, UINT shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING);
};

struct DXTexCubeShaderResourceViewDesc : public D3D12_SHADER_RESOURCE_VIEW_DESC
{
	DXTexCubeShaderResourceViewDesc(DXGI_FORMAT format, UINT mostDetailedMip = 0, UINT mipLevels = -1, UINT numCubes = 1,
		FLOAT minLODClamp = 0.0f, UINT shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING);
};

struct DXTex1DUnorderedAccessViewDesc : public D3D12_UNORDERED_ACCESS_VIEW_DESC
{
	DXTex1DUnorderedAccessViewDesc(DXGI_FORMAT format, UINT mipSlice = 0);
};

struct DXTex2DUnorderedAccessViewDesc : public D3D12_UNORDERED_ACCESS_VIEW_DESC
{
	DXTex2DUnorderedAccessViewDesc(DXGI_FORMAT format, UINT mipSlice = 0, UINT planeSlice = 0);
};

class DXResource : public DXObject<ID3D12Resource>
{
protected:
	DXResource(ID3D12Resource* pDXObject, D3D12_RESOURCE_STATES initialState);
	DXResource(const D3D12_RESOURCE_DESC* pDesc, D3D12_RESOURCE_STATES initialState);

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
void DXResource::Write(const T* pInputData, SIZE_T numBytes)
{
	const UINT subresource = 0;
	T* pResourceData = nullptr;

	const DXRange readRange(0, 0);
	DXVerify(GetDXObject()->Map(subresource, &readRange, reinterpret_cast<void**>(&pResourceData)));
	
	std::memcpy(pResourceData, pInputData, numBytes);
	
	const DXRange writtenRange(0, numBytes);
	GetDXObject()->Unmap(subresource, &writtenRange);
}

class DXColorTexture : public DXResource
{
public:
	DXColorTexture(DXRenderEnvironment* pRenderEnv, const D3D12_HEAP_PROPERTIES* pHeapProps,
		const DXColorTexture1DDesc* pTexDesc, D3D12_RESOURCE_STATES initialState,
		const FLOAT optimizedClearColor[4], LPCWSTR pName);

	DXColorTexture(DXRenderEnvironment* pRenderEnv, const D3D12_HEAP_PROPERTIES* pHeapProps,
		const DXColorTexture2DDesc* pTexDesc, D3D12_RESOURCE_STATES initialState,
		const FLOAT optimizedClearColor[4], LPCWSTR pName);

	DXColorTexture(DXRenderEnvironment* pRenderEnv, const D3D12_HEAP_PROPERTIES* pHeapProps,
		const DXColorTexture3DDesc* pTexDesc, D3D12_RESOURCE_STATES initialState,
		const FLOAT optimizedClearColor[4], LPCWSTR pName);

	DXColorTexture(DXRenderEnvironment* pRenderEnv, ID3D12Resource* pDXObject,
		D3D12_RESOURCE_STATES initialState, LPCWSTR pName);

	UINT64 GetWidth() const { return m_Desc.Width; }
	UINT GetHeight() const { return m_Desc.Height; }

	DXDescriptorHandle GetRTVHandle();
	DXDescriptorHandle GetSRVHandle();
	DXDescriptorHandle GetUAVHandle();
				
private:
	void CreateCommittedResource(DXRenderEnvironment* pRenderEnv, const D3D12_HEAP_PROPERTIES* pHeapProps,
		const D3D12_RESOURCE_DESC* pTexDesc, D3D12_RESOURCE_STATES initialState,
		const FLOAT optimizedClearColor[4], LPCWSTR pName);

	void CreateTex1DViews(DXRenderEnvironment* pRenderEnv, const D3D12_RESOURCE_DESC* pTexDesc);
	void CreateTex2DViews(DXRenderEnvironment* pRenderEnv, const D3D12_RESOURCE_DESC* pTexDesc);
	void CreateTex3DViews(DXRenderEnvironment* pRenderEnv, const D3D12_RESOURCE_DESC* pTexDesc);

	void DetermineResourceStates(const D3D12_RESOURCE_DESC* pTexDesc);

private:
	DXDescriptorHandle m_RTVHandle;
	DXDescriptorHandle m_SRVHandle;
	DXDescriptorHandle m_UAVHandle;
};

class DXDepthTexture : public DXResource
{
public:
	DXDepthTexture(DXRenderEnvironment* pRenderEnv, const D3D12_HEAP_PROPERTIES* pHeapProps,
		const DXDepthTexture1DDesc* pTexDesc, D3D12_RESOURCE_STATES initialState,
		const DXDepthStencilValue* pOptimizedClearDepth, LPCWSTR pName);

	DXDepthTexture(DXRenderEnvironment* pRenderEnv, const D3D12_HEAP_PROPERTIES* pHeapProps,
		const DXDepthTexture2DDesc* pTexDesc, D3D12_RESOURCE_STATES initialState,
		const DXDepthStencilValue* pOptimizedClearDepth, LPCWSTR pName);

	DXDepthTexture(DXRenderEnvironment* pRenderEnv, const D3D12_HEAP_PROPERTIES* pHeapProps,
		const DXDepthTexture3DDesc* pTexDesc, D3D12_RESOURCE_STATES initialState,
		const DXDepthStencilValue* pOptimizedClearDepth, LPCWSTR pName);

	UINT64 GetWidth() const { return m_Desc.Width; }
	UINT GetHeight() const { return m_Desc.Height; }

	DXDescriptorHandle GetDSVHandle();
	DXDescriptorHandle GetSRVHandle();

private:
	void CreateCommittedResource(DXRenderEnvironment* pRenderEnv, const D3D12_HEAP_PROPERTIES* pHeapProps,
		const D3D12_RESOURCE_DESC* pTexDesc, D3D12_RESOURCE_STATES initialState,
		const DXDepthStencilValue* pOptimizedClearDepth, LPCWSTR pName);

	void CreateTex1DViews(DXRenderEnvironment* pRenderEnv, const D3D12_RESOURCE_DESC* pTexDesc);
	void CreateTex2DViews(DXRenderEnvironment* pRenderEnv, const D3D12_RESOURCE_DESC* pTexDesc);
	void CreateTex3DViews(DXRenderEnvironment* pRenderEnv, const D3D12_RESOURCE_DESC* pTexDesc);

	void DetermineResourceStates(const D3D12_RESOURCE_DESC* pTexDesc);

private:
	DXDescriptorHandle m_DSVHandle;
	DXDescriptorHandle m_SRVHandle;
};

class DXBuffer : public DXResource
{
public:
	DXBuffer(DXRenderEnvironment* pRenderEnv, const D3D12_HEAP_PROPERTIES* pHeapProps,
		const DXConstantBufferDesc* pBufferDesc, D3D12_RESOURCE_STATES initialState, LPCWSTR pName);

	DXBuffer(DXRenderEnvironment* pRenderEnv, const D3D12_HEAP_PROPERTIES* pHeapProps,
		const DXVertexBufferDesc* pBufferDesc, D3D12_RESOURCE_STATES initialState, LPCWSTR pName);

	DXBuffer(DXRenderEnvironment* pRenderEnv, const D3D12_HEAP_PROPERTIES* pHeapProps,
		const DXIndexBufferDesc* pBufferDesc, D3D12_RESOURCE_STATES initialState, LPCWSTR pName);

	DXBuffer(DXRenderEnvironment* pRenderEnv, const D3D12_HEAP_PROPERTIES* pHeapProps,
		const DXStructuredBufferDesc* pBufferDesc, D3D12_RESOURCE_STATES initialState, LPCWSTR pName);

	DXBuffer(DXRenderEnvironment* pRenderEnv, const D3D12_HEAP_PROPERTIES* pHeapProps,
		const DXFormattedBufferDesc* pBufferDesc, D3D12_RESOURCE_STATES initialState, LPCWSTR pName);
	
	~DXBuffer();
	
	DXVertexBufferView* GetVBView();
	DXIndexBufferView*  GetIBView();

	DXDescriptorHandle GetSRVHandle();
	DXDescriptorHandle GetUAVHandle();
	DXDescriptorHandle GetCBVHandle();
		
private:
	void CreateCommittedResource(DXRenderEnvironment* pRenderEnv, const D3D12_HEAP_PROPERTIES* pHeapProps,
		const D3D12_RESOURCE_DESC* pBufferDesc, D3D12_RESOURCE_STATES initialState, LPCWSTR pName);

	void CreateConstantBufferView(DXRenderEnvironment* pRenderEnv, const DXConstantBufferDesc* pBufferDesc);
	void CreateVertexBufferView(DXRenderEnvironment* pRenderEnv, const DXVertexBufferDesc* pBufferDesc);
	void CreateIndexBufferView(DXRenderEnvironment* pRenderEnv, const DXIndexBufferDesc* pBufferDesc);
	void CreateStructuredBufferViews(DXRenderEnvironment* pRenderEnv, const DXStructuredBufferDesc* pBufferDesc);
	void CreateFormattedBufferViews(DXRenderEnvironment* pRenderEnv, const DXFormattedBufferDesc* pBufferDesc);
		
private:
	DXDescriptorHandle m_SRVHandle;
	DXDescriptorHandle m_UAVHandle;
	DXDescriptorHandle m_CBVHandle;
	DXVertexBufferView* m_pVBView;
	DXIndexBufferView* m_pIBView;
};

class DXSampler
{
public:
	DXSampler(DXRenderEnvironment* pRenderEnv, const D3D12_SAMPLER_DESC* pDesc);
	DXDescriptorHandle GetHandle() { return m_Handle; }

private:
	DXDescriptorHandle m_Handle;
};