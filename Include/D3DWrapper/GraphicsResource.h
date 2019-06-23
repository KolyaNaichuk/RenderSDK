#pragma once

#include "D3DWrapper/RenderEnv.h"
#include "D3DWrapper/DescriptorHeap.h"
#include "D3DWrapper/CommandQueue.h"
#include "D3DWrapper/CommandList.h"
#include "D3DWrapper/Fence.h"

class GraphicsDevice;
class GraphicsResource;

DXGI_FORMAT GetRenderTargetViewFormat(DXGI_FORMAT resourceFormat);
DXGI_FORMAT GetDepthStencilViewFormat(DXGI_FORMAT resourceFormat);
DXGI_FORMAT GetShaderResourceViewFormat(DXGI_FORMAT resourceFormat);
DXGI_FORMAT GetUnorderedAccessViewFormat(DXGI_FORMAT resourceFormat);

UINT CalcSubresource(UINT mipSlice, UINT arraySlice, UINT mipLevels);
UINT CountMips(UINT width, UINT height);

struct ResourceTransitionBarrier : D3D12_RESOURCE_BARRIER
{
	ResourceTransitionBarrier(GraphicsResource* pResource = nullptr,
		D3D12_RESOURCE_STATES stateBefore = D3D12_RESOURCE_STATE_COMMON,
		D3D12_RESOURCE_STATES stateAfter = D3D12_RESOURCE_STATE_COMMON,
		UINT subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
		D3D12_RESOURCE_BARRIER_FLAGS flags = D3D12_RESOURCE_BARRIER_FLAG_NONE);
};

struct ResourceUAVBarrier : D3D12_RESOURCE_BARRIER
{
	ResourceUAVBarrier(GraphicsResource* pResource = nullptr,
		D3D12_RESOURCE_BARRIER_FLAGS flags = D3D12_RESOURCE_BARRIER_FLAG_NONE);
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

struct ConstantBufferViewDesc : public D3D12_CONSTANT_BUFFER_VIEW_DESC
{
	ConstantBufferViewDesc(D3D12_GPU_VIRTUAL_ADDRESS bufferLocation, UINT sizeInBytes);
};

struct VertexBufferView : public D3D12_VERTEX_BUFFER_VIEW
{
	VertexBufferView(D3D12_GPU_VIRTUAL_ADDRESS bufferLocation, UINT sizeInBytes, UINT strideInBytes);
};

DXGI_FORMAT GetIndexBufferFormat(UINT strideInBytes);

struct IndexBufferView : public D3D12_INDEX_BUFFER_VIEW
{
	IndexBufferView(D3D12_GPU_VIRTUAL_ADDRESS bufferLocation, UINT sizeInBytes, DXGI_FORMAT format);
};

struct StructuredBufferDesc : public D3D12_RESOURCE_DESC
{
	StructuredBufferDesc(UINT numElements, UINT structureByteStride, bool createSRV, bool createUAV, bool createVBView = false, UINT64 alignment = 0);

	UINT NumElements;
	UINT StructureByteStride;
	bool CreateVBView;
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
	FormattedBufferDesc(UINT numElements, DXGI_FORMAT format, bool createSRV, bool createUAV, bool createIBView = false, UINT64 alignment = 0);

	UINT NumElements;
	DXGI_FORMAT SRVFormat;
	DXGI_FORMAT UAVFormat;
	bool CreateIBView;
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
		UINT16 mipLevels = 1,
		D3D12_TEXTURE_LAYOUT layout = D3D12_TEXTURE_LAYOUT_UNKNOWN,
		UINT64 alignment = 0);

	ColorTexture1DDesc(DXGI_FORMAT format, UINT64 width,
		bool createRTV, bool createSRV, bool createUAV,
		UINT16 mipLevels, UINT16 arraySize,
		D3D12_TEXTURE_LAYOUT layout = D3D12_TEXTURE_LAYOUT_UNKNOWN,
		UINT64 alignment = 0);

	bool IsTextureArray;
};

struct ColorTexture2DDesc : public D3D12_RESOURCE_DESC
{
	ColorTexture2DDesc(DXGI_FORMAT format, UINT64 width, UINT height,
		bool createRTV, bool createSRV, bool createUAV,
		UINT16 mipLevels = 1,
		D3D12_TEXTURE_LAYOUT layout = D3D12_TEXTURE_LAYOUT_UNKNOWN,
		UINT64 alignment = 0);

	ColorTexture2DDesc(DXGI_FORMAT format, UINT64 width, UINT height,
		bool createRTV, bool createSRV, bool createUAV,
		UINT16 mipLevels, UINT sampleCount, UINT sampleQuality,
		D3D12_TEXTURE_LAYOUT layout = D3D12_TEXTURE_LAYOUT_UNKNOWN,
		UINT64 alignment = 0);

	ColorTexture2DDesc(DXGI_FORMAT format, UINT64 width, UINT height,
		bool createRTV, bool createSRV, bool createUAV,
		UINT16 mipLevels, UINT16 arraySize,
		D3D12_TEXTURE_LAYOUT layout = D3D12_TEXTURE_LAYOUT_UNKNOWN,
		UINT64 alignment = 0);

	ColorTexture2DDesc(DXGI_FORMAT format, UINT64 width, UINT height,
		bool createRTV, bool createSRV, bool createUAV,
		UINT16 mipLevels, UINT16 arraySize,
		UINT sampleCount, UINT sampleQuality,
		D3D12_TEXTURE_LAYOUT layout = D3D12_TEXTURE_LAYOUT_UNKNOWN,
		UINT64 alignment = 0);

	bool IsTextureArray;
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
		UINT16 mipLevels = 1,
		D3D12_TEXTURE_LAYOUT layout = D3D12_TEXTURE_LAYOUT_UNKNOWN,
		UINT64 alignment = 0);

	DepthTexture1DDesc(DXGI_FORMAT format, UINT64 width,
		bool createDSV, bool createSRV,
		UINT16 mipLevels, UINT16 arraySize,
		D3D12_TEXTURE_LAYOUT layout = D3D12_TEXTURE_LAYOUT_UNKNOWN,
		UINT64 alignment = 0);

	bool IsTextureArray;
};

struct DepthTexture2DDesc : public D3D12_RESOURCE_DESC
{
	DepthTexture2DDesc(DXGI_FORMAT format, UINT64 width, UINT height,
		bool createDSV, bool createSRV,
		UINT16 mipLevels = 1,
		D3D12_TEXTURE_LAYOUT layout = D3D12_TEXTURE_LAYOUT_UNKNOWN,
		UINT64 alignment = 0);

	DepthTexture2DDesc(DXGI_FORMAT format, UINT64 width, UINT height,
		bool createDSV, bool createSRV,
		UINT16 mipLevels,
		UINT sampleCount, UINT sampleQuality,
		D3D12_TEXTURE_LAYOUT layout = D3D12_TEXTURE_LAYOUT_UNKNOWN,
		UINT64 alignment = 0);

	DepthTexture2DDesc(DXGI_FORMAT format, UINT64 width, UINT height,
		bool createDSV, bool createSRV,
		UINT16 mipLevels, UINT16 arraySize,
		D3D12_TEXTURE_LAYOUT layout = D3D12_TEXTURE_LAYOUT_UNKNOWN,
		UINT64 alignment = 0);

	DepthTexture2DDesc(DXGI_FORMAT format, UINT64 width, UINT height,
		bool createDSV, bool createSRV,
		UINT16 mipLevels, UINT16 arraySize,
		UINT sampleCount, UINT sampleQuality,
		D3D12_TEXTURE_LAYOUT layout = D3D12_TEXTURE_LAYOUT_UNKNOWN,
		UINT64 alignment = 0);

	bool IsTextureArray;
};

struct Tex1DRenderTargetViewDesc : public D3D12_RENDER_TARGET_VIEW_DESC
{
	Tex1DRenderTargetViewDesc(UINT mipSlice, DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN);
};

struct Tex1DDepthStencilViewDesc : public D3D12_DEPTH_STENCIL_VIEW_DESC
{
	Tex1DDepthStencilViewDesc(UINT mipSlice, DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN, D3D12_DSV_FLAGS flags = D3D12_DSV_FLAG_NONE);
};

struct Tex1DShaderResourceViewDesc : public D3D12_SHADER_RESOURCE_VIEW_DESC
{
	Tex1DShaderResourceViewDesc(DXGI_FORMAT format, UINT mostDetailedMip, UINT mipLevels,
		FLOAT minLODClamp = 0.0f, UINT shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING);
};

struct Tex1DUnorderedAccessViewDesc : public D3D12_UNORDERED_ACCESS_VIEW_DESC
{
	Tex1DUnorderedAccessViewDesc(DXGI_FORMAT format, UINT mipSlice);
};

struct Tex1DArrayRenderTargetViewDesc : public D3D12_RENDER_TARGET_VIEW_DESC
{
	Tex1DArrayRenderTargetViewDesc(UINT mipSlice, UINT firstArraySlice, 
		UINT arraySize, DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN);
};

struct Tex1DArrayDepthStencilViewDesc : public D3D12_DEPTH_STENCIL_VIEW_DESC
{
	Tex1DArrayDepthStencilViewDesc(DXGI_FORMAT format, UINT mipSlice, UINT firstArraySlice,
		UINT arraySize, D3D12_DSV_FLAGS flags = D3D12_DSV_FLAG_NONE);
};

struct Tex1DArrayShaderResourceViewDesc : public D3D12_SHADER_RESOURCE_VIEW_DESC
{
	Tex1DArrayShaderResourceViewDesc(DXGI_FORMAT format, UINT mostDetailedMip, UINT mipLevels, UINT firstArraySlice, UINT arraySize,
		FLOAT minLODClamp = 0.0f, UINT shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING);
};

struct Tex1DArrayUnorderedAccessViewDesc : public D3D12_UNORDERED_ACCESS_VIEW_DESC
{
	Tex1DArrayUnorderedAccessViewDesc(DXGI_FORMAT format, UINT mipSlice, UINT firstArraySlice, UINT arraySize);
};

struct Tex2DRenderTargetViewDesc : public D3D12_RENDER_TARGET_VIEW_DESC
{
	Tex2DRenderTargetViewDesc(UINT mipSlice, bool multisampled,
		DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN, UINT planeSlice = 0);
};

struct Tex2DDepthStencilViewDesc : public D3D12_DEPTH_STENCIL_VIEW_DESC
{
	Tex2DDepthStencilViewDesc(DXGI_FORMAT format, UINT mipSlice,
		bool multisampled, D3D12_DSV_FLAGS flags = D3D12_DSV_FLAG_NONE);
};

struct Tex2DShaderResourceViewDesc : public D3D12_SHADER_RESOURCE_VIEW_DESC
{
	Tex2DShaderResourceViewDesc(DXGI_FORMAT format, UINT mostDetailedMip, UINT mipLevels, bool multisampled,
		FLOAT minLODClamp = 0.0f, UINT shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING, UINT planeSlice = 0);
};

struct Tex2DUnorderedAccessViewDesc : public D3D12_UNORDERED_ACCESS_VIEW_DESC
{
	Tex2DUnorderedAccessViewDesc(DXGI_FORMAT format, UINT mipSlice, UINT planeSlice = 0);
};

struct Tex2DArrayRenderTargetViewDesc : public D3D12_RENDER_TARGET_VIEW_DESC
{
	Tex2DArrayRenderTargetViewDesc(UINT mipSlice, UINT firstArraySlice, UINT arraySize,
		bool multisampled, DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN, UINT planeSlice = 0);
};

struct Tex2DArrayDepthStencilViewDesc : public D3D12_DEPTH_STENCIL_VIEW_DESC
{
	Tex2DArrayDepthStencilViewDesc(DXGI_FORMAT format, UINT mipSlice, UINT firstArraySlice, UINT arraySize,
		bool multisampled, D3D12_DSV_FLAGS flags = D3D12_DSV_FLAG_NONE);
};

struct Tex2DArrayShaderResourceViewDesc : public D3D12_SHADER_RESOURCE_VIEW_DESC
{
	Tex2DArrayShaderResourceViewDesc(DXGI_FORMAT format, UINT mostDetailedMip, UINT mipLevels, UINT firstArraySlice, UINT arraySize,
		bool multisampled, FLOAT minLODClamp = 0.0f, UINT shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING, UINT planeSlice = 0);
};

struct Tex2DArrayUnorderedAccessViewDesc : public D3D12_UNORDERED_ACCESS_VIEW_DESC
{
	Tex2DArrayUnorderedAccessViewDesc(DXGI_FORMAT format, UINT mipSlice, UINT firstArraySlice, UINT arraySize, UINT planeSlice = 0);
};

struct Tex3DRenderTargetViewDesc : public D3D12_RENDER_TARGET_VIEW_DESC
{
	Tex3DRenderTargetViewDesc(UINT mipSlice, UINT firstDepthSlice,
		UINT depthSliceCount, DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN);
};

struct Tex3DShaderResourceViewDesc : public D3D12_SHADER_RESOURCE_VIEW_DESC
{
	Tex3DShaderResourceViewDesc(DXGI_FORMAT format, UINT mostDetailedMip, UINT mipLevels,
		FLOAT minLODClamp = 0.0f, UINT shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING);
};

struct Tex3DUnorderedAccessViewDesc : public D3D12_UNORDERED_ACCESS_VIEW_DESC
{
	Tex3DUnorderedAccessViewDesc(DXGI_FORMAT format, UINT mipSlice, UINT firstDepthSlice, UINT depthSliceCount);
};

class GraphicsResource
{
protected:
	GraphicsResource(ComPtr<ID3D12Resource> d3dResource);
	GraphicsResource(const D3D12_RESOURCE_DESC* pDesc);

public:
	ID3D12Resource* GetD3DObject() { return m_D3DResource.Get(); }
	DXGI_FORMAT GetFormat() const { return m_Desc.Format; }

	void* Map(UINT subresource, const D3D12_RANGE* pReadRange = nullptr);
	void Unmap(UINT subresource, const D3D12_RANGE* pWrittenRange = nullptr);
			
	void Write(const void* pInputData, SIZE_T numBytes);
	void Read(void* pOutputData, SIZE_T numBytes);

protected:
	ComPtr<ID3D12Resource> m_D3DResource;
	D3D12_RESOURCE_DESC m_Desc;
};

class ColorTexture : public GraphicsResource
{
public:
	ColorTexture(RenderEnv* pRenderEnv, const HeapProperties* pHeapProps,
		const ColorTexture1DDesc* pTexDesc, D3D12_RESOURCE_STATES initialState,
		const FLOAT optimizedClearColor[4], LPCWSTR pName);
	
	ColorTexture(RenderEnv* pRenderEnv, const HeapProperties* pHeapProps,
		const ColorTexture2DDesc* pTexDesc, D3D12_RESOURCE_STATES initialState,
		const FLOAT optimizedClearColor[4], LPCWSTR pName);
	
	ColorTexture(RenderEnv* pRenderEnv, const HeapProperties* pHeapProps,
		const ColorTexture3DDesc* pTexDesc, D3D12_RESOURCE_STATES initialState,
		const FLOAT optimizedClearColor[4], LPCWSTR pName);

	ColorTexture(RenderEnv* pRenderEnv, ComPtr<ID3D12Resource> d3dResource,
		bool isTextureArray, LPCWSTR pName);

	UINT64 GetWidth() const { return m_Desc.Width; }
	UINT GetHeight() const { return m_Desc.Height; }
	UINT16 GetDepthOrArraySize() const { return m_Desc.DepthOrArraySize; }
	UINT GetMipLevels() const { return m_Desc.MipLevels; }

	DescriptorHandle GetRTVHandle(UINT mipSlice = 0);
	DescriptorHandle GetRTVHandle(UINT mipSlice, UINT arraySlice);

	DescriptorHandle GetSRVHandle();

	DescriptorHandle GetUAVHandle(UINT mipSlice = 0);
	DescriptorHandle GetUAVHandle(UINT mipSlice, UINT arraySlice);
				
private:
	void CreateCommittedResource(RenderEnv* pRenderEnv, const D3D12_HEAP_PROPERTIES* pHeapProps,
		const D3D12_RESOURCE_DESC* pTexDesc, D3D12_RESOURCE_STATES initialState,
		const FLOAT optimizedClearColor[4], LPCWSTR pName);

	void CreateTexture1DViews(RenderEnv* pRenderEnv, const D3D12_RESOURCE_DESC* pTexDesc, bool isTextureArray);
	void CreateTexture2DViews(RenderEnv* pRenderEnv, const D3D12_RESOURCE_DESC* pTexDesc, bool isTextureArray);
	void CreateTexture3DViews(RenderEnv* pRenderEnv, const D3D12_RESOURCE_DESC* pTexDesc);
	
private:
	DescriptorHandle m_FirstRTVHandle;
	DescriptorHandle m_FirstSRVHandle;
	DescriptorHandle m_FirstUAVHandle;
};

class DepthTexture : public GraphicsResource
{
public:
	DepthTexture(RenderEnv* pRenderEnv, const HeapProperties* pHeapProps,
		const DepthTexture1DDesc* pTexDesc, D3D12_RESOURCE_STATES initialState,
		const DepthStencilValue* pOptimizedClearDepth, LPCWSTR pName);
	
	DepthTexture(RenderEnv* pRenderEnv, const HeapProperties* pHeapProps,
		const DepthTexture2DDesc* pTexDesc, D3D12_RESOURCE_STATES initialState,
		const DepthStencilValue* pOptimizedClearDepth, LPCWSTR pName);

	UINT64 GetWidth() const { return m_Desc.Width; }
	UINT GetHeight() const { return m_Desc.Height; }

	DescriptorHandle GetDSVHandle(UINT mipSlice = 0);
	DescriptorHandle GetDSVHandle(UINT mipSlice, UINT arraySlice);

	DescriptorHandle GetSRVHandle();

private:
	void CreateCommittedResource(RenderEnv* pRenderEnv, const D3D12_HEAP_PROPERTIES* pHeapProps,
		const D3D12_RESOURCE_DESC* pTexDesc, D3D12_RESOURCE_STATES initialState,
		const DepthStencilValue* pOptimizedClearDepth, LPCWSTR pName);

	void CreateTexture1DViews(RenderEnv* pRenderEnv, const D3D12_RESOURCE_DESC* pTexDesc, bool isTextureArray);
	void CreateTexture2DViews(RenderEnv* pRenderEnv, const D3D12_RESOURCE_DESC* pTexDesc, bool isTextureArray);
	
private:
	DescriptorHandle m_FirstDSVHandle;
	DescriptorHandle m_FirstSRVHandle;
};

class Buffer : public GraphicsResource
{
public:
	Buffer(RenderEnv* pRenderEnv, const D3D12_HEAP_PROPERTIES* pHeapProps,
		const ConstantBufferDesc* pBufferDesc, D3D12_RESOURCE_STATES initialState, LPCWSTR pName);

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

	UINT64 GetWidth() const { return m_Desc.Width; }
	D3D12_GPU_VIRTUAL_ADDRESS GetGPUVirtualAddress() { return m_D3DResource->GetGPUVirtualAddress(); }
	
private:
	void CreateCommittedResource(RenderEnv* pRenderEnv, const D3D12_HEAP_PROPERTIES* pHeapProps,
		const D3D12_RESOURCE_DESC* pBufferDesc, D3D12_RESOURCE_STATES initialState, LPCWSTR pName);

	void CreateBufferView(RenderEnv* pRenderEnv, const ConstantBufferDesc* pBufferDesc);
	void CreateBufferViews(RenderEnv* pRenderEnv, const StructuredBufferDesc* pBufferDesc);
	void CreateBufferViews(RenderEnv* pRenderEnv, const FormattedBufferDesc* pBufferDesc);

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

struct TextureCopyLocation : public D3D12_TEXTURE_COPY_LOCATION
{
	TextureCopyLocation(GraphicsResource* pGraphicsResource, UINT subresourceIndex);
	TextureCopyLocation(GraphicsResource* pGraphicsResource, const D3D12_PLACED_SUBRESOURCE_FOOTPRINT& footprint);
};

struct RayTracingTrianglesGeometryDesc : D3D12_RAYTRACING_GEOMETRY_DESC
{
	RayTracingTrianglesGeometryDesc(DXGI_FORMAT vertexFormat, Buffer* pVertexBuffer, Buffer* pIndexBuffer,
		D3D12_RAYTRACING_GEOMETRY_FLAGS flags = D3D12_RAYTRACING_GEOMETRY_FLAG_NONE, Buffer* pTransformBuffer = nullptr);
};

struct RayTracingInstanceDesc : D3D12_RAYTRACING_INSTANCE_DESC
{
};

struct BuildRayTracingAccelerationStructureInputs : D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS
{
};

struct RayTracingAccelerationStructurePrebuildInfo : D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO
{
};

struct BuildRayTracingAccelerationStructureDesc : D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC
{
	BuildRayTracingAccelerationStructureDesc(const D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS* pInputs,
		Buffer* pDestBuffer, Buffer* pScratchBuffer, Buffer* pSourceBuffer = nullptr);
};

template <typename DestBufferDesc>
void UploadData(RenderEnv* pRenderEnv, Buffer* pDestBuffer, DestBufferDesc destBufferDesc,
	D3D12_RESOURCE_STATES destBufferStateAfter, const void* pUploadData, SIZE_T numUploadBytes)
{
	destBufferDesc.Flags = D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;

	Buffer* pUploadBuffer = new Buffer(pRenderEnv, pRenderEnv->m_pUploadHeapProps, &destBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, L"UploadData::pUploadBuffer");
	pUploadBuffer->Write(pUploadData, numUploadBytes);

	ResourceTransitionBarrier resourceTransitionBarrier(pDestBuffer, D3D12_RESOURCE_STATE_COPY_DEST, destBufferStateAfter);

	CommandList* pUploadCommandList = pRenderEnv->m_pCommandListPool->Create(L"pUploadCommandList");
	pUploadCommandList->Begin();
	pUploadCommandList->CopyResource(pDestBuffer, pUploadBuffer);
	pUploadCommandList->ResourceBarrier(1, &resourceTransitionBarrier);
	pUploadCommandList->End();

	++pRenderEnv->m_LastSubmissionFenceValue;
	pRenderEnv->m_pCommandQueue->ExecuteCommandLists(1, &pUploadCommandList, pRenderEnv->m_pFence, pRenderEnv->m_LastSubmissionFenceValue);
	pRenderEnv->m_pFence->WaitForSignalOnCPU(pRenderEnv->m_LastSubmissionFenceValue);

	SafeDelete(pUploadBuffer);
}