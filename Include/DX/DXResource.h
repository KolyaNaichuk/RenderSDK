#pragma once

#include "DX/DXDescriptorHeap.h"

class DXDevice;
class DXBuffer;

struct DXRenderEnvironment;

DXGI_FORMAT GetRenderTargetViewFormat(DXGI_FORMAT resourceFormat);
DXGI_FORMAT GetDepthStencilViewFormat(DXGI_FORMAT resourceFormat);
DXGI_FORMAT GetShaderResourceViewFormat(DXGI_FORMAT resourceFormat);
DXGI_FORMAT GetUnorderedAccessViewFormat(DXGI_FORMAT resourceFormat);

struct DXColorClearValue : D3D12_CLEAR_VALUE
{
	DXColorClearValue(DXGI_FORMAT format, const FLOAT color[4]);
};

struct DXDepthStencilClearValue : D3D12_CLEAR_VALUE
{
	DXDepthStencilClearValue(DXGI_FORMAT format, FLOAT depth = 1.0f, UINT8 stencil = 0);
};

struct DXVertexBufferView : public D3D12_VERTEX_BUFFER_VIEW
{
	DXVertexBufferView(DXBuffer* pBuffer, UINT sizeInBytes, UINT strideInBytes);
};

struct DXIndexBufferView : public D3D12_INDEX_BUFFER_VIEW
{
	DXIndexBufferView(DXBuffer* pBuffer, UINT sizeInBytes, UINT strideInBytes);
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

// Kolya: should verify if the following structures are used
struct DXBufferShaderResourceViewDesc : public D3D12_SHADER_RESOURCE_VIEW_DESC
{
	DXBufferShaderResourceViewDesc(UINT64 firstElement, UINT numElements,
		UINT structureByteStride, DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN,
		UINT shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING);
};

struct DXRawBufferShaderResourceViewDesc : public D3D12_SHADER_RESOURCE_VIEW_DESC
{
	DXRawBufferShaderResourceViewDesc(UINT64 firstElement, UINT numElements,
		UINT shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING);
};

struct DXBufferUnorderedAccessViewDesc : public D3D12_UNORDERED_ACCESS_VIEW_DESC
{
	DXBufferUnorderedAccessViewDesc(UINT64 firstElement, UINT numElements,
		UINT structureByteStride, DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN);
};

struct DXCounterBufferUnorderedAccessViewDesc : public D3D12_UNORDERED_ACCESS_VIEW_DESC
{
	DXCounterBufferUnorderedAccessViewDesc(UINT64 firstElement, UINT numElements,
		UINT structureByteStride, UINT64 counterOffsetInBytes);
};

struct DXRawBufferUnorderedAccessViewDesc : public D3D12_UNORDERED_ACCESS_VIEW_DESC
{
	DXRawBufferUnorderedAccessViewDesc(UINT64 firstElement, UINT numElements);
};

///////////////////////////////////////////////////////////////////////////////////////////

struct DXConstantBufferDesc : public D3D12_RESOURCE_DESC
{
	DXConstantBufferDesc(UINT64 sizeInBytes, D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE, UINT64 alignment = 0);
};

struct DXVertexBufferDesc : public D3D12_RESOURCE_DESC
{
	DXVertexBufferDesc(UINT64 sizeInBytes, D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE, UINT64 alignment = 0);
};

struct DXIndexBufferDesc : public D3D12_RESOURCE_DESC
{
	DXIndexBufferDesc(UINT64 sizeInBytes, D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE, UINT64 alignment = 0);
};

struct DXStructuredBufferDesc : public D3D12_RESOURCE_DESC
{
	DXStructuredBufferDesc(UINT numElements, UINT structureByteStride,
		D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE, UINT64 alignment = 0);

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

struct DXTex1DResourceDesc : public D3D12_RESOURCE_DESC
{
	DXTex1DResourceDesc(DXGI_FORMAT format, UINT64 width,
		D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE,
		UINT16 arraySize = 1, UINT16 mipLevels = 0,
		D3D12_TEXTURE_LAYOUT layout = D3D12_TEXTURE_LAYOUT_UNKNOWN,
		UINT64 alignment = 0);
};

struct DXTex2DResourceDesc : public D3D12_RESOURCE_DESC
{
	DXTex2DResourceDesc(DXGI_FORMAT format, UINT64 width, UINT height,
		D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE,
		UINT16 arraySize = 1, UINT16 mipLevels = 0,
		UINT sampleCount = 1, UINT sampleQuality = 0,
		D3D12_TEXTURE_LAYOUT layout = D3D12_TEXTURE_LAYOUT_UNKNOWN,
		UINT64 alignment = 0);
};

struct DXTex3DResourceDesc : public D3D12_RESOURCE_DESC
{
	DXTex3DResourceDesc(DXGI_FORMAT format, UINT64 width, UINT height, UINT16 depth,
		D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE,
		UINT16 mipLevels = 0,
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
	DXResource(ID3D12Resource* pDXObject, D3D12_RESOURCE_STATES initialState, LPCWSTR pName);
	DXResource(const D3D12_RESOURCE_DESC* pDesc, D3D12_RESOURCE_STATES initialState, LPCWSTR pName);

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

class DXRenderTarget : public DXResource
{
public:
	DXRenderTarget(DXRenderEnvironment* pEnv, const D3D12_HEAP_PROPERTIES* pHeapProps,
		const DXTex1DResourceDesc* pTexDesc, D3D12_RESOURCE_STATES initialState, LPCWSTR pName);

	DXRenderTarget(DXRenderEnvironment* pEnv, const D3D12_HEAP_PROPERTIES* pHeapProps,
		const DXTex2DResourceDesc* pTexDesc, D3D12_RESOURCE_STATES initialState, LPCWSTR pName);

	DXRenderTarget(DXRenderEnvironment* pEnv, const D3D12_HEAP_PROPERTIES* pHeapProps,
		const DXTex3DResourceDesc* pTexDesc, D3D12_RESOURCE_STATES initialState, LPCWSTR pName);

	DXRenderTarget(DXRenderEnvironment* pEnv, ID3D12Resource* pDXObject,
		D3D12_RESOURCE_STATES initialState, LPCWSTR pName);

	UINT64 GetWidth() const { return m_Desc.Width; }
	UINT GetHeight() const { return m_Desc.Height; }

	DXDescriptorHandle GetRTVHandle() { return m_RTVHandle; }
	DXDescriptorHandle GetSRVHandle() { return m_SRVHandle; }
	DXDescriptorHandle GetUAVHandle() { return m_UAVHandle; }
				
private:
	void CreateCommittedResource(DXRenderEnvironment* pEnv, const D3D12_HEAP_PROPERTIES* pHeapProps,
		const D3D12_RESOURCE_DESC* pTexDesc, D3D12_RESOURCE_STATES initialState);

	void CreateTex1DViews(DXRenderEnvironment* pEnv, const D3D12_RESOURCE_DESC* pTexDesc);
	void CreateTex2DViews(DXRenderEnvironment* pEnv, const D3D12_RESOURCE_DESC* pTexDesc);
	void CreateTex3DViews(DXRenderEnvironment* pEnv, const D3D12_RESOURCE_DESC* pTexDesc);
	
private:
	DXDescriptorHandle m_RTVHandle;
	DXDescriptorHandle m_SRVHandle;
	DXDescriptorHandle m_UAVHandle;
};

class DXDepthStencilTexture : public DXResource
{
public:
	DXDepthStencilTexture(DXRenderEnvironment* pEnv, const D3D12_HEAP_PROPERTIES* pHeapProps,
		const DXTex1DResourceDesc* pTexDesc, D3D12_RESOURCE_STATES initialState,
		const DXDepthStencilClearValue* pOptimizedClearValue, LPCWSTR pName);

	DXDepthStencilTexture(DXRenderEnvironment* pEnv, const D3D12_HEAP_PROPERTIES* pHeapProps,
		const DXTex2DResourceDesc* pTexDesc, D3D12_RESOURCE_STATES initialState,
		const DXDepthStencilClearValue* pOptimizedClearValue, LPCWSTR pName);

	DXDepthStencilTexture(DXRenderEnvironment* pEnv, const D3D12_HEAP_PROPERTIES* pHeapProps,
		const DXTex3DResourceDesc* pTexDesc, D3D12_RESOURCE_STATES initialState,
		const DXDepthStencilClearValue* pOptimizedClearValue, LPCWSTR pName);

	UINT64 GetWidth() const { return m_Desc.Width; }
	UINT GetHeight() const { return m_Desc.Height; }

	DXDescriptorHandle GetDSVHandle() { return m_DSVHandle; }
	DXDescriptorHandle GetSRVHandle() { return m_SRVHandle; }

private:
	void CreateCommittedResource(DXRenderEnvironment* pEnv, const D3D12_HEAP_PROPERTIES* pHeapProps,
		const D3D12_RESOURCE_DESC* pTexDesc, D3D12_RESOURCE_STATES initialState,
		const DXDepthStencilClearValue* pOptimizedClearValue);

	void CreateTex1DViews(DXRenderEnvironment* pEnv, const D3D12_RESOURCE_DESC* pTexDesc);
	void CreateTex2DViews(DXRenderEnvironment* pEnv, const D3D12_RESOURCE_DESC* pTexDesc);
	void CreateTex3DViews(DXRenderEnvironment* pEnv, const D3D12_RESOURCE_DESC* pTexDesc);

private:
	DXDescriptorHandle m_DSVHandle;
	DXDescriptorHandle m_SRVHandle;
};

class DXBuffer : public DXResource
{
public:
	DXBuffer(DXRenderEnvironment* pEnv, const D3D12_HEAP_PROPERTIES* pHeapProps,
		const DXConstantBufferDesc* pBufferDesc, D3D12_RESOURCE_STATES initialState, LPCWSTR pName);

	DXBuffer(DXRenderEnvironment* pEnv, const D3D12_HEAP_PROPERTIES* pHeapProps,
		const DXVertexBufferDesc* pBufferDesc, D3D12_RESOURCE_STATES initialState, LPCWSTR pName);

	DXBuffer(DXRenderEnvironment* pEnv, const D3D12_HEAP_PROPERTIES* pHeapProps,
		const DXIndexBufferDesc* pBufferDesc, D3D12_RESOURCE_STATES initialState, LPCWSTR pName);

	DXBuffer(DXRenderEnvironment* pEnv, const D3D12_HEAP_PROPERTIES* pHeapProps,
		const DXStructuredBufferDesc* pBufferDesc, D3D12_RESOURCE_STATES initialState, LPCWSTR pName);
		
	DXDescriptorHandle GetSRVHandle() { return m_SRVHandle; }
	DXDescriptorHandle GetUAVHandle() { return m_UAVHandle; }
	DXDescriptorHandle GetCBVHandle() { return m_CBVHandle; }

	D3D12_GPU_VIRTUAL_ADDRESS GetGPUVirtualAddress() { return m_GPUVirtualAddress; }

private:
	void CreateCommittedResource(DXRenderEnvironment* pEnv, const D3D12_HEAP_PROPERTIES* pHeapProps,
		const D3D12_RESOURCE_DESC* pBufferDesc, D3D12_RESOURCE_STATES initialState);

	void CreateConstantBufferView(DXRenderEnvironment* pEnv, const DXConstantBufferDesc* pBufferDesc);
	void CreateStructuredBufferViews(DXRenderEnvironment* pEnv, const DXStructuredBufferDesc* pBufferDesc);
	
private:
	DXDescriptorHandle m_SRVHandle;
	DXDescriptorHandle m_UAVHandle;
	DXDescriptorHandle m_CBVHandle;
	D3D12_GPU_VIRTUAL_ADDRESS m_GPUVirtualAddress;
};

class DXSampler
{
public:
	DXSampler(DXRenderEnvironment* pEnv, const D3D12_SAMPLER_DESC* pDesc);
	DXDescriptorHandle GetHandle() { return m_Handle; }

private:
	DXDescriptorHandle m_Handle;
};