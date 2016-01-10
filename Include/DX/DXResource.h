#pragma once

#include "DXObject.h"

class DXDevice;
class DXResource;

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
	DXVertexBufferView(DXResource* pVertexBuffer, UINT sizeInBytes, UINT strideInBytes);
};

struct DXIndexBufferView : public D3D12_INDEX_BUFFER_VIEW
{
	DXIndexBufferView(DXResource* pIndexBuffer, UINT sizeInBytes, UINT strideInBytes);
};

struct DXConstantBufferViewDesc : public D3D12_CONSTANT_BUFFER_VIEW_DESC
{
	DXConstantBufferViewDesc(DXResource* pConstantBuffer, UINT sizeInBytes);
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

struct DXBufferResourceDesc : public D3D12_RESOURCE_DESC
{
	DXBufferResourceDesc(UINT64 sizeInBytes, D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE, UINT64 alignment = 0);
};

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

struct DXTex1DResourceDesc : public D3D12_RESOURCE_DESC
{
	DXTex1DResourceDesc(DXGI_FORMAT format, UINT64 width, D3D12_RESOURCE_FLAGS flags,
		UINT16 arraySize = 1, UINT16 mipLevels = 0,
		D3D12_TEXTURE_LAYOUT layout = D3D12_TEXTURE_LAYOUT_UNKNOWN,
		UINT64 alignment = 0);
};

struct DXTex2DResourceDesc : public D3D12_RESOURCE_DESC
{
	DXTex2DResourceDesc(DXGI_FORMAT format, UINT64 width, UINT height, D3D12_RESOURCE_FLAGS flags,
		UINT16 arraySize = 1, UINT16 mipLevels = 0,
		UINT sampleCount = 1, UINT sampleQuality = 0,
		D3D12_TEXTURE_LAYOUT layout = D3D12_TEXTURE_LAYOUT_UNKNOWN,
		UINT64 alignment = 0);
};

struct DXTex3DResourceDesc : public D3D12_RESOURCE_DESC
{
	DXTex3DResourceDesc(DXGI_FORMAT format, UINT64 width, UINT height, UINT16 depth, D3D12_RESOURCE_FLAGS flags,
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

struct DXTex2DUnorderedAccessViewDesc : public D3D12_UNORDERED_ACCESS_VIEW_DESC
{
	DXTex2DUnorderedAccessViewDesc(DXGI_FORMAT format, UINT mipSlice = 0, UINT planeSlice = 0);
};

class DXResource : public DXObject<ID3D12Resource>
{
public:
	DXResource(DXDevice* pDevice, const D3D12_HEAP_PROPERTIES* pHeapProperties, D3D12_HEAP_FLAGS heapFlags,
		const D3D12_RESOURCE_DESC* pDesc, D3D12_RESOURCE_STATES initialState, LPCWSTR pName, const D3D12_CLEAR_VALUE* pClearValue = nullptr);

	DXResource(ID3D12Resource* pDXObject, D3D12_RESOURCE_STATES initialState, LPCWSTR pName);
	
	UINT64 GetWidth() const;
	UINT GetHeight() const;

	D3D12_RESOURCE_STATES GetState() const;
	void SetState(D3D12_RESOURCE_STATES state);

	DXGI_FORMAT GetFormat() const;
	D3D12_GPU_VIRTUAL_ADDRESS GetGPUVirtualAddress();
			
	template <typename T>
	void Write(const T* pInputData, SIZE_T numBytes);
	
private:
	D3D12_RESOURCE_STATES m_State;
	DXGI_FORMAT m_Format;
	UINT64 m_Width;
	UINT m_Height;
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
