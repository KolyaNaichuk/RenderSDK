#include "D3DWrapper/GraphicsResource.h"
#include "D3DWrapper/GraphicsDevice.h"
#include "D3DWrapper/RenderEnv.h"
#include "D3DWrapper/GraphicsUtils.h"

DepthStencilValue::DepthStencilValue(FLOAT depth, UINT8 stencil)
{
	Depth = depth;
	Stencil = stencil;
}

ClearValue::ClearValue(DXGI_FORMAT format, const FLOAT color[4])
{
	Format = format;
	Color[0] = color[0];
	Color[1] = color[1];
	Color[2] = color[2];
	Color[3] = color[3];
}

ClearValue::ClearValue(DXGI_FORMAT format, const DepthStencilValue* pDepthStencilValue)
{
	Format = format;
	DepthStencil = *pDepthStencilValue;
}

DXGI_FORMAT GetRenderTargetViewFormat(DXGI_FORMAT resourceFormat)
{
	return resourceFormat;
}

DXGI_FORMAT GetDepthStencilViewFormat(DXGI_FORMAT resourceFormat)
{
	switch (resourceFormat)
	{
		case DXGI_FORMAT_R32_TYPELESS:
			return DXGI_FORMAT_D32_FLOAT;
		case DXGI_FORMAT_R16_TYPELESS:
			return DXGI_FORMAT_D16_UNORM;
		case DXGI_FORMAT_R24G8_TYPELESS:
			return DXGI_FORMAT_D24_UNORM_S8_UINT;
	}
	assert(false);
	return resourceFormat;
}

DXGI_FORMAT GetShaderResourceViewFormat(DXGI_FORMAT resourceFormat)
{
	switch (resourceFormat)
	{
		case DXGI_FORMAT_R32_TYPELESS:
		case DXGI_FORMAT_R32_FLOAT:
			return DXGI_FORMAT_R32_FLOAT;
		case DXGI_FORMAT_R16G16_UINT:
			return DXGI_FORMAT_R16G16_UINT;
		case DXGI_FORMAT_R11G11B10_FLOAT:
			return DXGI_FORMAT_R11G11B10_FLOAT;
		case DXGI_FORMAT_R32_UINT:
			return DXGI_FORMAT_R32_UINT;
		case DXGI_FORMAT_R24G8_TYPELESS:
			return DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
		case DXGI_FORMAT_R10G10B10A2_UNORM:
			return DXGI_FORMAT_R10G10B10A2_UNORM;
		case DXGI_FORMAT_R8G8B8A8_SNORM:
			return DXGI_FORMAT_R8G8B8A8_SNORM;
		case DXGI_FORMAT_R16G16_SNORM:
			return DXGI_FORMAT_R16G16_SNORM;
		case DXGI_FORMAT_R8G8B8A8_UNORM:
			return DXGI_FORMAT_R8G8B8A8_UNORM;
		case DXGI_FORMAT_R32G32B32A32_FLOAT:
			return DXGI_FORMAT_R32G32B32A32_FLOAT;
		case DXGI_FORMAT_R16G16B16A16_FLOAT:
			return DXGI_FORMAT_R16G16B16A16_FLOAT;
		case DXGI_FORMAT_R32G32_FLOAT:
			return DXGI_FORMAT_R32G32_FLOAT;
		case DXGI_FORMAT_R16G16_FLOAT:
			return DXGI_FORMAT_R16G16_FLOAT;
		case DXGI_FORMAT_R16_UINT:
			return DXGI_FORMAT_R16_UINT;
		case DXGI_FORMAT_R16_TYPELESS:
			return DXGI_FORMAT_R16_FLOAT;
		case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
			return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
			return DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
		case DXGI_FORMAT_R8_UNORM:
			return DXGI_FORMAT_R8_UNORM;
		case DXGI_FORMAT_B8G8R8X8_UNORM:
			return DXGI_FORMAT_B8G8R8X8_UNORM;
		case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
			return DXGI_FORMAT_B8G8R8X8_UNORM_SRGB;
	}
	assert(false);
	return resourceFormat;
}

DXGI_FORMAT GetUnorderedAccessViewFormat(DXGI_FORMAT resourceFormat)
{
	switch (resourceFormat)
	{
		case DXGI_FORMAT_R8G8B8A8_UNORM:
			return DXGI_FORMAT_R8G8B8A8_UNORM;
		case DXGI_FORMAT_R10G10B10A2_UNORM:
			return DXGI_FORMAT_R10G10B10A2_UNORM;
		case DXGI_FORMAT_R16G16B16A16_FLOAT:
			return DXGI_FORMAT_R16G16B16A16_FLOAT;
		case DXGI_FORMAT_R32G32B32A32_FLOAT:
			return DXGI_FORMAT_R32G32B32A32_FLOAT;
		case DXGI_FORMAT_R32G32_FLOAT:
			return DXGI_FORMAT_R32G32_FLOAT;
		case DXGI_FORMAT_R32_FLOAT:
			return DXGI_FORMAT_R32_FLOAT;
		case DXGI_FORMAT_R32_UINT:
			return DXGI_FORMAT_R32_UINT;
		case DXGI_FORMAT_R16G16_SNORM:
			return DXGI_FORMAT_R16G16_SNORM;
		case DXGI_FORMAT_R8G8B8A8_SNORM:
			return DXGI_FORMAT_R8G8B8A8_SNORM;
		case DXGI_FORMAT_R11G11B10_FLOAT:
			return DXGI_FORMAT_R11G11B10_FLOAT;
	}
	assert(false);
	return resourceFormat;
}

UINT CalcSubresource(UINT mipSlice, UINT arraySlice, UINT mipLevels)
{
	return (arraySlice * mipLevels + mipSlice);
}

UINT CountMips(UINT width, UINT height)
{
	UINT mipLevels = 1;
	while (height > 1 || width > 1)
	{
		if (height > 1)
			height >>= 1;

		if (width > 1)
			width >>= 1;

		++mipLevels;
	}
	return mipLevels;
}

ResourceTransitionBarrier::ResourceTransitionBarrier(GraphicsResource* pResource, D3D12_RESOURCE_STATES stateBefore, D3D12_RESOURCE_STATES stateAfter, UINT subresource, D3D12_RESOURCE_BARRIER_FLAGS flags)
{
	Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	Flags = flags;
	Transition.pResource = (pResource != nullptr) ? pResource->GetD3DObject() : nullptr;
	Transition.StateBefore = stateBefore;
	Transition.StateAfter = stateAfter;
	Transition.Subresource = subresource;
}

ResourceUAVBarrier::ResourceUAVBarrier(GraphicsResource* pResource, D3D12_RESOURCE_BARRIER_FLAGS flags)
{
	Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
	Flags = flags;
	UAV.pResource = (pResource != nullptr) ? pResource->GetD3DObject() : nullptr;
}

MemoryRange::MemoryRange(SIZE_T begin, SIZE_T end)
{
	Begin = begin;
	End = end;
}

ConstantBufferDesc::ConstantBufferDesc(UINT64 sizeInBytes, UINT64 alignment)
{
	Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	Alignment = alignment;
	Width = sizeInBytes;
	Height = 1;
	DepthOrArraySize = 1;
	MipLevels = 1;
	Format = DXGI_FORMAT_UNKNOWN;
	SampleDesc.Count = 1;
	SampleDesc.Quality = 0;
	Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	Flags = D3D12_RESOURCE_FLAG_NONE;
}

ConstantBufferViewDesc::ConstantBufferViewDesc(D3D12_GPU_VIRTUAL_ADDRESS bufferLocation, UINT sizeInBytes)
{
	BufferLocation = bufferLocation;
	SizeInBytes = sizeInBytes;
}

VertexBufferDesc::VertexBufferDesc(UINT numVertices, UINT strideInBytes, UINT64 alignment)
{
	Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	Alignment = alignment;
	Width = numVertices * strideInBytes;
	Height = 1;
	DepthOrArraySize = 1;
	MipLevels = 1;
	Format = DXGI_FORMAT_UNKNOWN;
	SampleDesc.Count = 1;
	SampleDesc.Quality = 0;
	Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	Flags = D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
	NumVertices = numVertices;
	StrideInBytes = strideInBytes;
}

VertexBufferView::VertexBufferView(D3D12_GPU_VIRTUAL_ADDRESS bufferLocation, UINT sizeInBytes, UINT strideInBytes)
{
	BufferLocation = bufferLocation;
	SizeInBytes = sizeInBytes;
	StrideInBytes = strideInBytes;
}

IndexBufferDesc::IndexBufferDesc(UINT numIndices, UINT strideInBytes, UINT64 alignment)
{
	Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	Alignment = alignment;
	Width = numIndices * strideInBytes;
	Height = 1;
	DepthOrArraySize = 1;
	MipLevels = 1;
	Format = DXGI_FORMAT_UNKNOWN;
	SampleDesc.Count = 1;
	SampleDesc.Quality = 0;
	Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	Flags = D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
	NumIndices = numIndices;
	StrideInBytes = strideInBytes;
}

IndexBufferView::IndexBufferView(D3D12_GPU_VIRTUAL_ADDRESS bufferLocation, UINT sizeInBytes, UINT strideInBytes)
{
	assert((strideInBytes == sizeof(u16)) || (strideInBytes == sizeof(u32)));

	BufferLocation = bufferLocation;
	SizeInBytes = sizeInBytes;
	Format = (strideInBytes == sizeof(u16)) ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;
}

StructuredBufferDesc::StructuredBufferDesc(UINT numElements, UINT structureByteStride, bool createSRV, bool createUAV, UINT64 alignment)
{
	Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	Alignment = alignment;
	Width = numElements * structureByteStride;
	Height = 1;
	DepthOrArraySize = 1;
	MipLevels = 1;
	Format = DXGI_FORMAT_UNKNOWN;
	SampleDesc.Count = 1;
	SampleDesc.Quality = 0;
	Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	
	Flags = D3D12_RESOURCE_FLAG_NONE;
	if (createUAV)
		Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	if (!createSRV)
		Flags |= D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
	
	NumElements = numElements;
	StructureByteStride = structureByteStride;
}

StructuredBufferSRVDesc::StructuredBufferSRVDesc(UINT64 firstElement, UINT numElements,
	UINT structureByteStride, UINT shader4ComponentMapping)
{
	Format = DXGI_FORMAT_UNKNOWN;
	ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	Shader4ComponentMapping = shader4ComponentMapping;
	Buffer.FirstElement = firstElement;
	Buffer.NumElements = numElements;
	Buffer.StructureByteStride = structureByteStride;
	Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
}

StructuredBufferUAVDesc::StructuredBufferUAVDesc(UINT64 firstElement, UINT numElements, UINT structureByteStride)
{
	Format = DXGI_FORMAT_UNKNOWN;
	ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
	Buffer.FirstElement = firstElement;
	Buffer.NumElements = numElements;
	Buffer.StructureByteStride = structureByteStride;
	Buffer.CounterOffsetInBytes = 0;
	Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;
}

FormattedBufferDesc::FormattedBufferDesc(UINT numElements, DXGI_FORMAT format, bool createSRV, bool createUAV, UINT64 alignment)
{
	Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	Alignment = alignment;
	Width = numElements * GetSizeInBytes(format);
	Height = 1;
	DepthOrArraySize = 1;
	MipLevels = 1;
	Format = DXGI_FORMAT_UNKNOWN;
	SampleDesc.Count = 1;
	SampleDesc.Quality = 0;
	Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	Flags = D3D12_RESOURCE_FLAG_NONE;
	if (createUAV)
		Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	if (!createSRV)
		Flags |= D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;

	NumElements = numElements;
	SRVFormat = format;
	UAVFormat = format;
}

FormattedBufferSRVDesc::FormattedBufferSRVDesc(UINT64 firstElement, UINT numElements,
	DXGI_FORMAT format, UINT shader4ComponentMapping)
{
	Format = format;
	ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	Shader4ComponentMapping = shader4ComponentMapping;
	Buffer.FirstElement = firstElement;
	Buffer.NumElements = numElements;
	Buffer.StructureByteStride = 0;
	Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
}

FormattedBufferUAVDesc::FormattedBufferUAVDesc(UINT64 firstElement, UINT numElements, DXGI_FORMAT format)
{
	Format = format;
	ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
	Buffer.FirstElement = firstElement;
	Buffer.NumElements = numElements;
	Buffer.StructureByteStride = 0;
	Buffer.CounterOffsetInBytes = 0;
	Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;
}

RawBufferSRVDesc::RawBufferSRVDesc(UINT64 firstElement, UINT numElements, UINT shader4ComponentMapping)
{
	Format = DXGI_FORMAT_R32_TYPELESS;
	ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	Shader4ComponentMapping = shader4ComponentMapping;
	Buffer.FirstElement = firstElement;
	Buffer.NumElements = numElements;
	Buffer.StructureByteStride = 0;
	Buffer.Flags = D3D12_BUFFER_SRV_FLAG_RAW;
}

RawBufferUAVDesc::RawBufferUAVDesc(UINT64 firstElement, UINT numElements)
{
	Format = DXGI_FORMAT_R32_TYPELESS;
	ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
	Buffer.FirstElement = firstElement;
	Buffer.NumElements = numElements;
	Buffer.StructureByteStride = 0;
	Buffer.CounterOffsetInBytes = 0;
	Buffer.Flags = D3D12_BUFFER_UAV_FLAG_RAW;
}

CounterBufferUAVDesc::CounterBufferUAVDesc(UINT64 firstElement, UINT numElements,
	UINT structureByteStride, UINT64 counterOffsetInBytes)
{
	Format = DXGI_FORMAT_UNKNOWN;
	ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
	Buffer.FirstElement = firstElement;
	Buffer.NumElements = numElements;
	Buffer.StructureByteStride = structureByteStride;
	Buffer.CounterOffsetInBytes = counterOffsetInBytes;
	Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;
}

ColorTexture1DDesc::ColorTexture1DDesc(DXGI_FORMAT format, UINT64 width, bool createRTV, bool createSRV, bool createUAV,
	UINT16 mipLevels, D3D12_TEXTURE_LAYOUT layout, UINT64 alignment)
{
	Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE1D;
	Alignment = alignment;
	Width = width;
	Height = 1;
	DepthOrArraySize = 1;
	MipLevels = mipLevels;
	Format = format;
	SampleDesc.Count = 1;
	SampleDesc.Quality = 0;
	Layout = layout;
	IsTextureArray = false;

	Flags = D3D12_RESOURCE_FLAG_NONE;
	if (createRTV)
		Flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
	if (!createSRV)
		Flags |= D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
	if (createUAV)
		Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
}

ColorTexture1DDesc::ColorTexture1DDesc(DXGI_FORMAT format, UINT64 width, bool createRTV, bool createSRV, bool createUAV,
	UINT16 mipLevels, UINT16 arraySize, D3D12_TEXTURE_LAYOUT layout, UINT64 alignment)
{
	Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE1D;
	Alignment = alignment;
	Width = width;
	Height = 1;
	DepthOrArraySize = arraySize;
	MipLevels = mipLevels;
	Format = format;
	SampleDesc.Count = 1;
	SampleDesc.Quality = 0;
	Layout = layout;
	IsTextureArray = true;

	Flags = D3D12_RESOURCE_FLAG_NONE;
	if (createRTV)
		Flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
	if (!createSRV)
		Flags |= D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
	if (createUAV)
		Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
}

ColorTexture2DDesc::ColorTexture2DDesc(DXGI_FORMAT format, UINT64 width, UINT height, bool createRTV, bool createSRV, bool createUAV,
	UINT16 mipLevels, D3D12_TEXTURE_LAYOUT layout, UINT64 alignment)
{
	Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	Alignment = alignment;
	Width = width;
	Height = height;
	DepthOrArraySize = 1;
	MipLevels = mipLevels;
	Format = format;
	SampleDesc.Count = 1;
	SampleDesc.Quality = 0;
	Layout = layout;
	IsTextureArray = false;

	Flags = D3D12_RESOURCE_FLAG_NONE;
	if (createRTV)
		Flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
	if (!createSRV)
		Flags |= D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
	if (createUAV)
		Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
}

ColorTexture2DDesc::ColorTexture2DDesc(DXGI_FORMAT format, UINT64 width, UINT height, bool createRTV, bool createSRV, bool createUAV,
	UINT16 mipLevels, UINT sampleCount, UINT sampleQuality, D3D12_TEXTURE_LAYOUT layout, UINT64 alignment)
{
	Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	Alignment = alignment;
	Width = width;
	Height = height;
	DepthOrArraySize = 1;
	MipLevels = mipLevels;
	Format = format;
	SampleDesc.Count = sampleCount;
	SampleDesc.Quality = sampleQuality;
	Layout = layout;
	IsTextureArray = false;
	
	Flags = D3D12_RESOURCE_FLAG_NONE;
	if (createRTV)
		Flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
	if (!createSRV)
		Flags |= D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
	if (createUAV)
		Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
}

ColorTexture2DDesc::ColorTexture2DDesc(DXGI_FORMAT format, UINT64 width, UINT height, bool createRTV, bool createSRV, bool createUAV,
	UINT16 mipLevels, UINT16 arraySize, D3D12_TEXTURE_LAYOUT layout, UINT64 alignment)
{
	Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	Alignment = alignment;
	Width = width;
	Height = height;
	DepthOrArraySize = arraySize;
	MipLevels = mipLevels;
	Format = format;
	SampleDesc.Count = 1;
	SampleDesc.Quality = 0;
	Layout = layout;
	IsTextureArray = true;

	Flags = D3D12_RESOURCE_FLAG_NONE;
	if (createRTV)
		Flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
	if (!createSRV)
		Flags |= D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
	if (createUAV)
		Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
}

ColorTexture2DDesc::ColorTexture2DDesc(DXGI_FORMAT format, UINT64 width, UINT height, bool createRTV, bool createSRV, bool createUAV,
	UINT16 mipLevels, UINT16 arraySize, UINT sampleCount, UINT sampleQuality, D3D12_TEXTURE_LAYOUT layout, UINT64 alignment)
{
	Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	Alignment = alignment;
	Width = width;
	Height = height;
	DepthOrArraySize = arraySize;
	MipLevels = mipLevels;
	Format = format;
	SampleDesc.Count = sampleCount;
	SampleDesc.Quality = sampleQuality;
	Layout = layout;
	IsTextureArray = true;

	Flags = D3D12_RESOURCE_FLAG_NONE;
	if (createRTV)
		Flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
	if (!createSRV)
		Flags |= D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
	if (createUAV)
		Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
}

ColorTexture3DDesc::ColorTexture3DDesc(DXGI_FORMAT format, UINT64 width, UINT height, UINT16 depth,
	bool createRTV, bool createSRV, bool createUAV, UINT16 mipLevels, D3D12_TEXTURE_LAYOUT layout, UINT64 alignment)
{
	Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE3D;
	Alignment = alignment;
	Width = width;
	Height = height;
	DepthOrArraySize = depth;
	MipLevels = mipLevels;
	Format = format;
	SampleDesc.Count = 1;
	SampleDesc.Quality = 0;
	Layout = layout;
	
	Flags = D3D12_RESOURCE_FLAG_NONE;
	if (createRTV)
		Flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
	if (!createSRV)
		Flags |= D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
	if (createUAV)
		Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
}

DepthTexture1DDesc::DepthTexture1DDesc(DXGI_FORMAT format, UINT64 width, bool createDSV, bool createSRV,
	UINT16 mipLevels, D3D12_TEXTURE_LAYOUT layout, UINT64 alignment)
{
	Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE1D;
	Alignment = alignment;
	Width = width;
	Height = 1;
	DepthOrArraySize = 1;
	MipLevels = mipLevels;
	Format = format;
	SampleDesc.Count = 1;
	SampleDesc.Quality = 0;
	Layout = layout;
	IsTextureArray = false;
	
	Flags = D3D12_RESOURCE_FLAG_NONE;
	if (createDSV)
		Flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
	if (!createSRV)
		Flags |= D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
}

DepthTexture1DDesc::DepthTexture1DDesc(DXGI_FORMAT format, UINT64 width, bool createDSV, bool createSRV,
	UINT16 mipLevels, UINT16 arraySize, D3D12_TEXTURE_LAYOUT layout, UINT64 alignment)
{
	Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE1D;
	Alignment = alignment;
	Width = width;
	Height = 1;
	DepthOrArraySize = arraySize;
	MipLevels = mipLevels;
	Format = format;
	SampleDesc.Count = 1;
	SampleDesc.Quality = 0;
	Layout = layout;
	IsTextureArray = true;

	Flags = D3D12_RESOURCE_FLAG_NONE;
	if (createDSV)
		Flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
	if (!createSRV)
		Flags |= D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
}

DepthTexture2DDesc::DepthTexture2DDesc(DXGI_FORMAT format, UINT64 width, UINT height, bool createDSV, bool createSRV,
	UINT16 mipLevels, D3D12_TEXTURE_LAYOUT layout, UINT64 alignment)
{
	Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	Alignment = alignment;
	Width = width;
	Height = height;
	DepthOrArraySize = 1;
	MipLevels = mipLevels;
	Format = format;
	SampleDesc.Count = 1;
	SampleDesc.Quality = 0;
	Layout = layout;
	IsTextureArray = false;

	Flags = D3D12_RESOURCE_FLAG_NONE;
	if (createDSV)
		Flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
	if (!createSRV)
		Flags |= D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
}

DepthTexture2DDesc::DepthTexture2DDesc(DXGI_FORMAT format, UINT64 width, UINT height, bool createDSV, bool createSRV,
	UINT16 mipLevels, UINT sampleCount, UINT sampleQuality, D3D12_TEXTURE_LAYOUT layout, UINT64 alignment)
{
	Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	Alignment = alignment;
	Width = width;
	Height = height;
	DepthOrArraySize = 1;
	MipLevels = mipLevels;
	Format = format;
	SampleDesc.Count = sampleCount;
	SampleDesc.Quality = sampleQuality;
	Layout = layout;
	IsTextureArray = false;
	
	Flags = D3D12_RESOURCE_FLAG_NONE;
	if (createDSV)
		Flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
	if (!createSRV)
		Flags |= D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
}

DepthTexture2DDesc::DepthTexture2DDesc(DXGI_FORMAT format, UINT64 width, UINT height, bool createDSV, bool createSRV,
	UINT16 mipLevels, UINT16 arraySize, D3D12_TEXTURE_LAYOUT layout, UINT64 alignment)
{
	Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	Alignment = alignment;
	Width = width;
	Height = height;
	DepthOrArraySize = arraySize;
	MipLevels = mipLevels;
	Format = format;
	SampleDesc.Count = 1;
	SampleDesc.Quality = 0;
	Layout = layout;
	IsTextureArray = true;

	Flags = D3D12_RESOURCE_FLAG_NONE;
	if (createDSV)
		Flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
	if (!createSRV)
		Flags |= D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
}

DepthTexture2DDesc::DepthTexture2DDesc(DXGI_FORMAT format, UINT64 width, UINT height, bool createDSV, bool createSRV,
	UINT16 mipLevels, UINT16 arraySize, UINT sampleCount, UINT sampleQuality, D3D12_TEXTURE_LAYOUT layout, UINT64 alignment)
{
	Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	Alignment = alignment;
	Width = width;
	Height = height;
	DepthOrArraySize = arraySize;
	MipLevels = mipLevels;
	Format = format;
	SampleDesc.Count = sampleCount;
	SampleDesc.Quality = sampleQuality;
	Layout = layout;
	IsTextureArray = true;

	Flags = D3D12_RESOURCE_FLAG_NONE;
	if (createDSV)
		Flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
	if (!createSRV)
		Flags |= D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
}

Tex1DRenderTargetViewDesc::Tex1DRenderTargetViewDesc(UINT mipSlice, DXGI_FORMAT format)
{
	Format = format;
	ViewDimension = D3D12_RTV_DIMENSION_TEXTURE1D;
	Texture1D.MipSlice = mipSlice;
}

Tex1DDepthStencilViewDesc::Tex1DDepthStencilViewDesc(UINT mipSlice, DXGI_FORMAT format, D3D12_DSV_FLAGS flags)
{
	Format = format;
	Flags = flags;
	ViewDimension = D3D12_DSV_DIMENSION_TEXTURE1D;
	Texture1D.MipSlice = mipSlice;
}

Tex1DShaderResourceViewDesc::Tex1DShaderResourceViewDesc(DXGI_FORMAT format, UINT mostDetailedMip,
	UINT mipLevels, FLOAT minLODClamp, UINT shader4ComponentMapping)
{
	Format = format;
	Shader4ComponentMapping = shader4ComponentMapping;
	ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1D;
	Texture1D.MostDetailedMip = mostDetailedMip;
	Texture1D.MipLevels = mipLevels;
	Texture1D.ResourceMinLODClamp = minLODClamp;
}

Tex1DUnorderedAccessViewDesc::Tex1DUnorderedAccessViewDesc(DXGI_FORMAT format, UINT mipSlice)
{
	Format = format;
	ViewDimension = D3D12_UAV_DIMENSION_TEXTURE1D;
	Texture1D.MipSlice = mipSlice;
}

Tex1DArrayRenderTargetViewDesc::Tex1DArrayRenderTargetViewDesc(UINT mipSlice, UINT firstArraySlice, UINT arraySize, DXGI_FORMAT format)
{
	Format = format;
	ViewDimension = D3D12_RTV_DIMENSION_TEXTURE1DARRAY;
	Texture1DArray.MipSlice = mipSlice;
	Texture1DArray.FirstArraySlice = firstArraySlice;
	Texture1DArray.ArraySize = arraySize;
}

Tex1DArrayDepthStencilViewDesc::Tex1DArrayDepthStencilViewDesc(DXGI_FORMAT format, UINT mipSlice, UINT firstArraySlice, UINT arraySize, D3D12_DSV_FLAGS flags)
{
	Format = format;
	Flags = flags;
	ViewDimension = D3D12_DSV_DIMENSION_TEXTURE1DARRAY;
	Texture1DArray.MipSlice = mipSlice;
	Texture1DArray.FirstArraySlice = firstArraySlice;
	Texture1DArray.ArraySize = arraySize;
}

Tex1DArrayShaderResourceViewDesc::Tex1DArrayShaderResourceViewDesc(DXGI_FORMAT format, UINT mostDetailedMip, UINT mipLevels,
	UINT firstArraySlice, UINT arraySize, FLOAT minLODClamp, UINT shader4ComponentMapping)
{
	Format = format;
	Shader4ComponentMapping = shader4ComponentMapping;
	ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1DARRAY;
	Texture1DArray.MostDetailedMip = mostDetailedMip;
	Texture1DArray.MipLevels = mipLevels;
	Texture1DArray.FirstArraySlice = firstArraySlice;
	Texture1DArray.ArraySize = arraySize;
	Texture1DArray.ResourceMinLODClamp = minLODClamp;
}

Tex1DArrayUnorderedAccessViewDesc::Tex1DArrayUnorderedAccessViewDesc(DXGI_FORMAT format, UINT mipSlice, UINT firstArraySlice, UINT arraySize)
{
	Format = format;
	ViewDimension = D3D12_UAV_DIMENSION_TEXTURE1DARRAY;
	Texture1DArray.MipSlice = mipSlice;
	Texture1DArray.FirstArraySlice = firstArraySlice;
	Texture1DArray.ArraySize = arraySize;
}

Tex2DRenderTargetViewDesc::Tex2DRenderTargetViewDesc(UINT mipSlice, bool multisampled, DXGI_FORMAT format, UINT planeSlice)
{
	Format = format;
	if (multisampled)
	{
		ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DMS;
	}
	else
	{
		ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
		Texture2D.MipSlice = mipSlice;
		Texture2D.PlaneSlice = planeSlice;
	}
}

Tex2DDepthStencilViewDesc::Tex2DDepthStencilViewDesc(DXGI_FORMAT format, UINT mipSlice, bool multisampled, D3D12_DSV_FLAGS flags)
{
	Format = format;
	Flags = flags;

	if (multisampled)
	{
		ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DMS;
	}
	else
	{
		ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
		Texture2D.MipSlice = mipSlice;
	}
}

Tex2DShaderResourceViewDesc::Tex2DShaderResourceViewDesc(DXGI_FORMAT format, UINT mostDetailedMip, UINT mipLevels, bool multisampled,
	FLOAT minLODClamp, UINT shader4ComponentMapping, UINT planeSlice)
{
	Format = format;
	Shader4ComponentMapping = shader4ComponentMapping;

	if (multisampled)
	{
		ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMS;
	}
	else
	{
		ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		Texture2D.MostDetailedMip = mostDetailedMip;
		Texture2D.MipLevels = mipLevels;
		Texture2D.PlaneSlice = planeSlice;
		Texture2D.ResourceMinLODClamp = minLODClamp;
	}
}

Tex2DUnorderedAccessViewDesc::Tex2DUnorderedAccessViewDesc(DXGI_FORMAT format, UINT mipSlice, UINT planeSlice)
{
	Format = format;
	ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
	Texture2D.MipSlice = mipSlice;
	Texture2D.PlaneSlice = planeSlice;
}

Tex2DArrayRenderTargetViewDesc::Tex2DArrayRenderTargetViewDesc(UINT mipSlice, UINT firstArraySlice, UINT arraySize, bool multisampled, DXGI_FORMAT format, UINT planeSlice)
{
	Format = format;
	if (multisampled)
	{
		ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DMSARRAY;
		Texture2DMSArray.FirstArraySlice = firstArraySlice;
		Texture2DMSArray.ArraySize = arraySize;
	}
	else
	{
		ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
		Texture2DArray.MipSlice = mipSlice;
		Texture2DArray.FirstArraySlice = firstArraySlice;
		Texture2DArray.ArraySize = arraySize;
		Texture2DArray.PlaneSlice = planeSlice;
	}
}

Tex2DArrayDepthStencilViewDesc::Tex2DArrayDepthStencilViewDesc(DXGI_FORMAT format, UINT mipSlice, UINT firstArraySlice, UINT arraySize, bool multisampled, D3D12_DSV_FLAGS flags)
{
	Format = format;
	Flags = flags;

	if (multisampled)
	{
		ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DMSARRAY;
		Texture2DMSArray.FirstArraySlice = firstArraySlice;
		Texture2DMSArray.ArraySize = arraySize;
	}
	else
	{
		ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
		Texture2DArray.MipSlice = mipSlice;
		Texture2DArray.FirstArraySlice = firstArraySlice;
		Texture2DArray.ArraySize = arraySize;
	}
}

Tex2DArrayShaderResourceViewDesc::Tex2DArrayShaderResourceViewDesc(DXGI_FORMAT format, UINT mostDetailedMip, UINT mipLevels,
	UINT firstArraySlice, UINT arraySize, bool multisampled, FLOAT minLODClamp, UINT shader4ComponentMapping, UINT planeSlice)
{
	Format = format;
	Shader4ComponentMapping = shader4ComponentMapping;

	if (multisampled)
	{
		ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMSARRAY;
		Texture2DMSArray.FirstArraySlice = firstArraySlice;
		Texture2DMSArray.ArraySize = arraySize;
	}
	else
	{
		ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
		Texture2DArray.MostDetailedMip = mostDetailedMip;
		Texture2DArray.MipLevels = mipLevels;
		Texture2DArray.FirstArraySlice = firstArraySlice;
		Texture2DArray.ArraySize = arraySize;
		Texture2DArray.PlaneSlice = planeSlice;
		Texture2DArray.ResourceMinLODClamp = minLODClamp;
	}
}

Tex2DArrayUnorderedAccessViewDesc::Tex2DArrayUnorderedAccessViewDesc(DXGI_FORMAT format, UINT mipSlice, UINT firstArraySlice, UINT arraySize, UINT planeSlice)
{
	Format = format;
	ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
	Texture2DArray.MipSlice = mipSlice;
	Texture2DArray.FirstArraySlice = firstArraySlice;
	Texture2DArray.ArraySize = arraySize;
	Texture2DArray.PlaneSlice = planeSlice;
}

Tex3DRenderTargetViewDesc::Tex3DRenderTargetViewDesc(UINT mipSlice, UINT firstDepthSlice, UINT depthSliceCount, DXGI_FORMAT format)
{
	ViewDimension = D3D12_RTV_DIMENSION_TEXTURE3D;
	Format = format;
	Texture3D.MipSlice = mipSlice;
	Texture3D.FirstWSlice = firstDepthSlice;
	Texture3D.WSize = depthSliceCount;
}

Tex3DShaderResourceViewDesc::Tex3DShaderResourceViewDesc(DXGI_FORMAT format, UINT mostDetailedMip, UINT mipLevels, FLOAT minLODClamp, UINT shader4ComponentMapping)
{
	Format = format;
	Shader4ComponentMapping = shader4ComponentMapping;
	ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
	Texture3D.MostDetailedMip = mostDetailedMip;
	Texture3D.MipLevels = mipLevels;
	Texture3D.ResourceMinLODClamp = minLODClamp;
}

Tex3DUnorderedAccessViewDesc::Tex3DUnorderedAccessViewDesc(DXGI_FORMAT format, UINT mipSlice, UINT firstDepthSlice, UINT depthSliceCount)
{
	Format = format;
	ViewDimension = D3D12_UAV_DIMENSION_TEXTURE3D;
	Texture3D.MipSlice = mipSlice;
	Texture3D.FirstWSlice = firstDepthSlice;
	Texture3D.WSize = depthSliceCount;
}

GraphicsResource::GraphicsResource(ComPtr<ID3D12Resource> d3dResource)
	: m_D3DResource(d3dResource)
	, m_Desc(d3dResource->GetDesc())
{
}

GraphicsResource::GraphicsResource(const D3D12_RESOURCE_DESC* pDesc)
	: m_Desc(*pDesc)
{
}

void* GraphicsResource::Map(UINT subresource, const D3D12_RANGE* pReadRange)
{
	void* pResourceData = nullptr;
	VerifyD3DResult(GetD3DObject()->Map(subresource, pReadRange, reinterpret_cast<void**>(&pResourceData)));
	return pResourceData;
}

void GraphicsResource::Unmap(UINT subresource, const D3D12_RANGE* pWrittenRange)
{
	GetD3DObject()->Unmap(subresource, pWrittenRange);
}

void GraphicsResource::Write(const void* pInputData, SIZE_T numBytes)
{
	const UINT subresource = 0;
	
	const MemoryRange readRange(0, 0);
	void* pResourceData = Map(subresource, &readRange);

	std::memcpy(pResourceData, pInputData, numBytes);
	
	const MemoryRange writtenRange(0, numBytes);
	Unmap(subresource, &writtenRange);
}

void GraphicsResource::Read(void* pOutputData, SIZE_T numBytes)
{
	const UINT subresource = 0;

	const MemoryRange readRange(0, numBytes);
	void* pResourceData = Map(subresource, &readRange);

	std::memcpy(pOutputData, pResourceData, numBytes);
	
	const MemoryRange writtenRange(0, 0);
	Unmap(subresource, &writtenRange);
}

HeapProperties::HeapProperties(D3D12_HEAP_TYPE type)
{
	Type = type;
	CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	CreationNodeMask = 1;
	VisibleNodeMask = 1;
}

HeapProperties::HeapProperties(D3D12_CPU_PAGE_PROPERTY cpuPageProperty, D3D12_MEMORY_POOL memoryPoolPreference)
{
	Type = D3D12_HEAP_TYPE_CUSTOM;
	CPUPageProperty = cpuPageProperty;
	MemoryPoolPreference = memoryPoolPreference;
	CreationNodeMask = 1;
	VisibleNodeMask = 1;
}

ColorTexture::ColorTexture(RenderEnv* pRenderEnv, const HeapProperties* pHeapProps,
	const ColorTexture1DDesc* pTexDesc, D3D12_RESOURCE_STATES initialState,
	const FLOAT optimizedClearColor[4], LPCWSTR pName)
	: GraphicsResource(pTexDesc)
{
	CreateCommittedResource(pRenderEnv, pHeapProps, pTexDesc, initialState, optimizedClearColor, pName);
	CreateTexture1DViews(pRenderEnv, pTexDesc, pTexDesc->IsTextureArray);
}

ColorTexture::ColorTexture(RenderEnv* pRenderEnv, const HeapProperties* pHeapProps,
	const ColorTexture2DDesc* pTexDesc, D3D12_RESOURCE_STATES initialState,
	const FLOAT optimizedClearColor[4], LPCWSTR pName)
	: GraphicsResource(pTexDesc)
{
	CreateCommittedResource(pRenderEnv, pHeapProps, pTexDesc, initialState, optimizedClearColor, pName);
	CreateTexture2DViews(pRenderEnv, pTexDesc, pTexDesc->IsTextureArray);
}

ColorTexture::ColorTexture(RenderEnv* pRenderEnv, const HeapProperties* pHeapProps,
	const ColorTexture3DDesc* pTexDesc, D3D12_RESOURCE_STATES initialState,
	const FLOAT optimizedClearColor[4], LPCWSTR pName)
	: GraphicsResource(pTexDesc)
{
	CreateCommittedResource(pRenderEnv, pHeapProps, pTexDesc, initialState, optimizedClearColor, pName);
	CreateTexture3DViews(pRenderEnv, pTexDesc);
}

ColorTexture::ColorTexture(RenderEnv* pRenderEnv, ComPtr<ID3D12Resource> d3dResource, bool isTextureArray, LPCWSTR pName)
	: GraphicsResource(d3dResource)
{
#ifdef ENABLE_GRAPHICS_DEBUGGING
	VerifyD3DResult(m_D3DResource->SetName(pName));
#endif // ENABLE_GRAPHICS_DEBUGGING

	switch (m_Desc.Dimension)
	{
		case D3D12_RESOURCE_DIMENSION_TEXTURE1D:
		{
			CreateTexture1DViews(pRenderEnv, &m_Desc, isTextureArray);
			break;
		}
		case D3D12_RESOURCE_DIMENSION_TEXTURE2D:
		{
			CreateTexture2DViews(pRenderEnv, &m_Desc, isTextureArray);
			break;
		}
		case D3D12_RESOURCE_DIMENSION_TEXTURE3D:
		{
			CreateTexture3DViews(pRenderEnv, &m_Desc);
			break;
		}
		default:
		{
			assert(false);
		}
	}
}

DescriptorHandle ColorTexture::GetRTVHandle(UINT mipSlice)
{
	assert(m_FirstRTVHandle.IsValid());
	return DescriptorHandle(m_FirstRTVHandle, mipSlice);
}

DescriptorHandle ColorTexture::GetRTVHandle(UINT mipSlice, UINT arraySlice)
{
	assert(m_FirstRTVHandle.IsValid());
	return DescriptorHandle(m_FirstRTVHandle, m_Desc.MipLevels + CalcSubresource(mipSlice, arraySlice, m_Desc.MipLevels));
}

DescriptorHandle ColorTexture::GetSRVHandle()
{
	assert(m_FirstSRVHandle.IsValid());
	return m_FirstSRVHandle;
}

DescriptorHandle ColorTexture::GetUAVHandle(UINT mipSlice)
{
	assert(m_FirstUAVHandle.IsValid());
	return DescriptorHandle(m_FirstUAVHandle, mipSlice);
}

DescriptorHandle ColorTexture::GetUAVHandle(UINT mipSlice, UINT arraySlice)
{
	assert(m_FirstUAVHandle.IsValid());
	return DescriptorHandle(m_FirstUAVHandle, m_Desc.MipLevels + CalcSubresource(mipSlice, arraySlice, m_Desc.MipLevels));
}

void ColorTexture::CreateCommittedResource(RenderEnv* pRenderEnv, const D3D12_HEAP_PROPERTIES* pHeapProps,
	const D3D12_RESOURCE_DESC* pTexDesc, D3D12_RESOURCE_STATES initialState,
	const FLOAT optimizedClearColor[4], LPCWSTR pName)
{
	ID3D12Device* pD3DDevice = pRenderEnv->m_pDevice->GetD3DObject();

	D3D12_HEAP_FLAGS heapFlags = D3D12_HEAP_FLAG_NONE;
	if ((pTexDesc->Flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET) != 0)
	{
		DXGI_FORMAT rtvFormat = GetRenderTargetViewFormat(pTexDesc->Format);
		ClearValue optimizedClearValue(rtvFormat, optimizedClearColor);

		VerifyD3DResult(pD3DDevice->CreateCommittedResource(pHeapProps, heapFlags, pTexDesc,
			initialState, &optimizedClearValue, IID_PPV_ARGS(&m_D3DResource)));
	}
	else
	{
		VerifyD3DResult(pD3DDevice->CreateCommittedResource(pHeapProps, heapFlags, pTexDesc,
			initialState, nullptr, IID_PPV_ARGS(&m_D3DResource)));
	}

#ifdef ENABLE_GRAPHICS_DEBUGGING
	VerifyD3DResult(m_D3DResource->SetName(pName));
#endif // ENABLE_GRAPHICS_DEBUGGING
}

void ColorTexture::CreateTexture1DViews(RenderEnv* pRenderEnv, const D3D12_RESOURCE_DESC* pTexDesc, bool isTextureArray)
{
	ID3D12Device* pD3DDevice = pRenderEnv->m_pDevice->GetD3DObject();
	if ((pTexDesc->Flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET) != 0)
	{
		assert(pRenderEnv->m_pShaderInvisibleRTVHeap != nullptr);
		if (isTextureArray)
		{
			for (UINT mipSlice = 0; mipSlice < pTexDesc->MipLevels; ++mipSlice)
			{
				Tex1DArrayRenderTargetViewDesc viewDesc(mipSlice, 0, pTexDesc->DepthOrArraySize);
				DescriptorHandle rtvHandle = pRenderEnv->m_pShaderInvisibleRTVHeap->Allocate();
				pD3DDevice->CreateRenderTargetView(GetD3DObject(), &viewDesc, rtvHandle);

				if (mipSlice == 0)
					m_FirstRTVHandle = rtvHandle;
			}
			for (UINT arraySlice = 0; arraySlice < pTexDesc->DepthOrArraySize; ++arraySlice)
			{
				for (UINT mipSlice = 0; mipSlice < pTexDesc->MipLevels; ++mipSlice)
				{
					Tex1DArrayRenderTargetViewDesc viewDesc(mipSlice, arraySlice, 1);
					DescriptorHandle rtvHandle = pRenderEnv->m_pShaderInvisibleRTVHeap->Allocate();
					pD3DDevice->CreateRenderTargetView(GetD3DObject(), &viewDesc, rtvHandle);
				}
			}
		}
		else
		{
			for (UINT mipSlice = 0; mipSlice < pTexDesc->MipLevels; ++mipSlice)
			{
				Tex1DRenderTargetViewDesc viewDesc(mipSlice);
				DescriptorHandle rtvHandle = pRenderEnv->m_pShaderInvisibleRTVHeap->Allocate();
				pD3DDevice->CreateRenderTargetView(GetD3DObject(), &viewDesc, rtvHandle);

				if (mipSlice == 0)
					m_FirstRTVHandle = rtvHandle;
			}
		}
	}
	if ((pTexDesc->Flags & D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE) == 0)
	{
		assert(pRenderEnv->m_pShaderInvisibleSRVHeap != nullptr);
		const DXGI_FORMAT viewFormat = GetShaderResourceViewFormat(pTexDesc->Format);

		if (isTextureArray)
		{
			Tex1DArrayShaderResourceViewDesc viewDesc(viewFormat, 0, pTexDesc->MipLevels, 0, pTexDesc->DepthOrArraySize);
			m_FirstSRVHandle = pRenderEnv->m_pShaderInvisibleSRVHeap->Allocate();
			pD3DDevice->CreateShaderResourceView(GetD3DObject(), &viewDesc, m_FirstSRVHandle);
		}
		else
		{
			Tex1DShaderResourceViewDesc viewDesc(viewFormat, 0, pTexDesc->MipLevels);
			m_FirstSRVHandle = pRenderEnv->m_pShaderInvisibleSRVHeap->Allocate();
			pD3DDevice->CreateShaderResourceView(GetD3DObject(), &viewDesc, m_FirstSRVHandle);
		}
	}
	if ((pTexDesc->Flags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS) != 0)
	{
		assert(pRenderEnv->m_pShaderInvisibleSRVHeap != nullptr);
		const DXGI_FORMAT viewFormat = GetUnorderedAccessViewFormat(pTexDesc->Format);

		if (isTextureArray)
		{
			for (UINT mipSlice = 0; mipSlice < pTexDesc->MipLevels; ++mipSlice)
			{
				Tex1DArrayUnorderedAccessViewDesc viewDesc(viewFormat, mipSlice, 0, pTexDesc->DepthOrArraySize);
				DescriptorHandle uavHandle = pRenderEnv->m_pShaderInvisibleSRVHeap->Allocate();
				pD3DDevice->CreateUnorderedAccessView(GetD3DObject(), nullptr, &viewDesc, uavHandle);

				if (mipSlice == 0)
					m_FirstUAVHandle = uavHandle;
			}
			for (UINT arraySlice = 0; arraySlice < pTexDesc->DepthOrArraySize; ++arraySlice)
			{
				for (UINT mipSlice = 0; mipSlice < pTexDesc->MipLevels; ++mipSlice)
				{
					Tex1DArrayUnorderedAccessViewDesc viewDesc(viewFormat, mipSlice, arraySlice, 1);
					DescriptorHandle uavHandle = pRenderEnv->m_pShaderInvisibleSRVHeap->Allocate();
					pD3DDevice->CreateUnorderedAccessView(GetD3DObject(), nullptr, &viewDesc, uavHandle);
				}
			}
		}
		else
		{
			for (UINT mipSlice = 0; mipSlice < pTexDesc->MipLevels; ++mipSlice)
			{
				Tex1DUnorderedAccessViewDesc viewDesc(viewFormat, mipSlice);
				DescriptorHandle uavHandle = pRenderEnv->m_pShaderInvisibleSRVHeap->Allocate();
				pD3DDevice->CreateUnorderedAccessView(GetD3DObject(), nullptr, &viewDesc, uavHandle);

				if (mipSlice == 0)
					m_FirstUAVHandle = uavHandle;
			}
		}
	}
}

void ColorTexture::CreateTexture2DViews(RenderEnv* pRenderEnv, const D3D12_RESOURCE_DESC* pTexDesc, bool isTextureArray)
{
	ID3D12Device* pD3DDevice = pRenderEnv->m_pDevice->GetD3DObject();
	const bool multisampled = pTexDesc->SampleDesc.Count > 1;

	if ((pTexDesc->Flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET) != 0)
	{
		assert(pRenderEnv->m_pShaderInvisibleRTVHeap != nullptr);
		if (isTextureArray)
		{
			for (UINT mipSlice = 0; mipSlice < pTexDesc->MipLevels; ++mipSlice)
			{
				Tex2DArrayRenderTargetViewDesc viewDesc(mipSlice, 0, pTexDesc->DepthOrArraySize, multisampled);
				DescriptorHandle rtvHandle = pRenderEnv->m_pShaderInvisibleRTVHeap->Allocate();
				pD3DDevice->CreateRenderTargetView(GetD3DObject(), &viewDesc, rtvHandle);

				if (mipSlice == 0)
					m_FirstRTVHandle = rtvHandle;
			}
			for (UINT arraySlice = 0; arraySlice < pTexDesc->DepthOrArraySize; ++arraySlice)
			{
				for (UINT mipSlice = 0; mipSlice < pTexDesc->MipLevels; ++mipSlice)
				{
					Tex2DArrayRenderTargetViewDesc viewDesc(mipSlice, arraySlice, 1, multisampled);
					DescriptorHandle rtvHandle = pRenderEnv->m_pShaderInvisibleRTVHeap->Allocate();
					pD3DDevice->CreateRenderTargetView(GetD3DObject(), &viewDesc, rtvHandle);
				}
			}
		}
		else
		{
			for (UINT mipSlice = 0; mipSlice < pTexDesc->MipLevels; ++mipSlice)
			{
				Tex2DRenderTargetViewDesc viewDesc(mipSlice, multisampled);
				DescriptorHandle rtvHandle = pRenderEnv->m_pShaderInvisibleRTVHeap->Allocate();
				pD3DDevice->CreateRenderTargetView(GetD3DObject(), &viewDesc, rtvHandle);

				if (mipSlice == 0)
					m_FirstRTVHandle = rtvHandle;
			}
		}
	}
	if ((pTexDesc->Flags & D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE) == 0)
	{
		assert(pRenderEnv->m_pShaderInvisibleSRVHeap != nullptr);
		const DXGI_FORMAT viewFormat = GetShaderResourceViewFormat(pTexDesc->Format);

		if (isTextureArray)
		{
			Tex2DArrayShaderResourceViewDesc viewDesc(viewFormat, 0, pTexDesc->MipLevels, 0, pTexDesc->DepthOrArraySize, multisampled);
			m_FirstSRVHandle = pRenderEnv->m_pShaderInvisibleSRVHeap->Allocate();
			pD3DDevice->CreateShaderResourceView(GetD3DObject(), &viewDesc, m_FirstSRVHandle);
		}
		else
		{
			Tex2DShaderResourceViewDesc viewDesc(viewFormat, 0, pTexDesc->MipLevels, multisampled);
			m_FirstSRVHandle = pRenderEnv->m_pShaderInvisibleSRVHeap->Allocate();
			pD3DDevice->CreateShaderResourceView(GetD3DObject(), &viewDesc, m_FirstSRVHandle);
		}
	}
	if ((pTexDesc->Flags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS) != 0)
	{
		assert(pRenderEnv->m_pShaderInvisibleSRVHeap != nullptr);
		const DXGI_FORMAT viewFormat = GetUnorderedAccessViewFormat(pTexDesc->Format);

		if (isTextureArray)
		{
			for (UINT mipSlice = 0; mipSlice < pTexDesc->MipLevels; ++mipSlice)
			{
				Tex2DArrayUnorderedAccessViewDesc viewDesc(viewFormat, mipSlice, 0, pTexDesc->DepthOrArraySize);
				DescriptorHandle uavHandle = pRenderEnv->m_pShaderInvisibleSRVHeap->Allocate();
				pD3DDevice->CreateUnorderedAccessView(GetD3DObject(), nullptr, &viewDesc, uavHandle);

				if (mipSlice == 0)
					m_FirstUAVHandle = uavHandle;
			}
			for (UINT arraySlice = 0; arraySlice < pTexDesc->DepthOrArraySize; ++arraySlice)
			{
				for (UINT mipSlice = 0; mipSlice < pTexDesc->MipLevels; ++mipSlice)
				{
					Tex2DArrayUnorderedAccessViewDesc viewDesc(viewFormat, mipSlice, arraySlice, 1);
					DescriptorHandle uavHandle = pRenderEnv->m_pShaderInvisibleSRVHeap->Allocate();
					pD3DDevice->CreateUnorderedAccessView(GetD3DObject(), nullptr, &viewDesc, uavHandle);
				}
			}
		}
		else
		{
			for (UINT mipSlice = 0; mipSlice < pTexDesc->MipLevels; ++mipSlice)
			{
				Tex2DUnorderedAccessViewDesc viewDesc(viewFormat, mipSlice);
				DescriptorHandle uavHandle = pRenderEnv->m_pShaderInvisibleSRVHeap->Allocate();
				pD3DDevice->CreateUnorderedAccessView(GetD3DObject(), nullptr, &viewDesc, uavHandle);

				if (mipSlice == 0)
					m_FirstUAVHandle = uavHandle;
			}
		}
	}
}

void ColorTexture::CreateTexture3DViews(RenderEnv* pRenderEnv, const D3D12_RESOURCE_DESC* pTexDesc)
{
	ID3D12Device* pD3DDevice = pRenderEnv->m_pDevice->GetD3DObject();

	if ((pTexDesc->Flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET) != 0)
	{
		assert(pRenderEnv->m_pShaderInvisibleRTVHeap != nullptr);
		for (UINT mipSlice = 0; mipSlice < pTexDesc->MipLevels; ++mipSlice)
		{
			Tex3DRenderTargetViewDesc viewDesc(mipSlice, 0, pTexDesc->DepthOrArraySize);
			DescriptorHandle rtvHandle = pRenderEnv->m_pShaderInvisibleRTVHeap->Allocate();
			pD3DDevice->CreateRenderTargetView(GetD3DObject(), &viewDesc, rtvHandle);

			if (mipSlice == 0)
				m_FirstRTVHandle = rtvHandle;
		}
	}
	if ((pTexDesc->Flags & D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE) == 0)
	{
		assert(pRenderEnv->m_pShaderInvisibleSRVHeap != nullptr);
		const DXGI_FORMAT viewFormat = GetShaderResourceViewFormat(pTexDesc->Format);

		Tex3DShaderResourceViewDesc viewDesc(viewFormat, 0, pTexDesc->MipLevels);
		m_FirstSRVHandle = pRenderEnv->m_pShaderInvisibleSRVHeap->Allocate();
		pD3DDevice->CreateShaderResourceView(GetD3DObject(), &viewDesc, m_FirstSRVHandle);
	}
	if ((pTexDesc->Flags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS) != 0)
	{
		assert(pRenderEnv->m_pShaderInvisibleSRVHeap != nullptr);
		const DXGI_FORMAT viewFormat = GetUnorderedAccessViewFormat(pTexDesc->Format);

		for (UINT mipSlice = 0; mipSlice < pTexDesc->MipLevels; ++mipSlice)
		{
			Tex3DUnorderedAccessViewDesc viewDesc(viewFormat, mipSlice, 0, pTexDesc->DepthOrArraySize);
			DescriptorHandle uavHandle = pRenderEnv->m_pShaderInvisibleSRVHeap->Allocate();
			pD3DDevice->CreateUnorderedAccessView(GetD3DObject(), nullptr, &viewDesc, uavHandle);

			if (mipSlice == 0)
				m_FirstUAVHandle = uavHandle;
		}
	}
}

DepthTexture::DepthTexture(RenderEnv* pRenderEnv, const HeapProperties* pHeapProps,
	const DepthTexture1DDesc* pTexDesc, D3D12_RESOURCE_STATES initialState,
	const DepthStencilValue* pOptimizedClearDepth, LPCWSTR pName)
	: GraphicsResource(pTexDesc)
{
	CreateCommittedResource(pRenderEnv, pHeapProps, pTexDesc, initialState, pOptimizedClearDepth, pName);
	CreateTexture1DViews(pRenderEnv, pTexDesc, pTexDesc->IsTextureArray);
}

DepthTexture::DepthTexture(RenderEnv* pRenderEnv, const HeapProperties* pHeapProps,
	const DepthTexture2DDesc* pTexDesc, D3D12_RESOURCE_STATES initialState,
	const DepthStencilValue* pOptimizedClearDepth, LPCWSTR pName)
	: GraphicsResource(pTexDesc)
{
	CreateCommittedResource(pRenderEnv, pHeapProps, pTexDesc, initialState, pOptimizedClearDepth, pName);
	CreateTexture2DViews(pRenderEnv, pTexDesc, pTexDesc->IsTextureArray);
}

DescriptorHandle DepthTexture::GetDSVHandle(UINT mipSlice)
{
	assert(m_FirstDSVHandle.IsValid());
	return DescriptorHandle(m_FirstDSVHandle, mipSlice);
}

DescriptorHandle DepthTexture::GetDSVHandle(UINT mipSlice, UINT arraySlice)
{
	assert(m_FirstDSVHandle.IsValid());
	return DescriptorHandle(m_FirstDSVHandle, m_Desc.MipLevels + CalcSubresource(mipSlice, arraySlice, m_Desc.MipLevels));
}

DescriptorHandle DepthTexture::GetSRVHandle()
{
	assert(m_FirstSRVHandle.IsValid());
	return m_FirstSRVHandle;
}

void DepthTexture::CreateCommittedResource(RenderEnv* pRenderEnv, const D3D12_HEAP_PROPERTIES* pHeapProps,
	const D3D12_RESOURCE_DESC* pTexDesc, D3D12_RESOURCE_STATES initialState,
	const DepthStencilValue* pOptimizedClearDepth, LPCWSTR pName)
{
	ID3D12Device* pD3DDevice = pRenderEnv->m_pDevice->GetD3DObject();
	
	D3D12_HEAP_FLAGS heapFlags = D3D12_HEAP_FLAG_NONE;
	DXGI_FORMAT dsvFormat = GetDepthStencilViewFormat(pTexDesc->Format);
	ClearValue optimizedClearValue(dsvFormat, pOptimizedClearDepth);

	VerifyD3DResult(pD3DDevice->CreateCommittedResource(pHeapProps, heapFlags, pTexDesc,
		initialState, &optimizedClearValue, IID_PPV_ARGS(&m_D3DResource)));

#ifdef ENABLE_GRAPHICS_DEBUGGING
	VerifyD3DResult(m_D3DResource->SetName(pName));
#endif // ENABLE_GRAPHICS_DEBUGGING
}

void DepthTexture::CreateTexture1DViews(RenderEnv* pRenderEnv, const D3D12_RESOURCE_DESC* pTexDesc, bool isTextureArray)
{
	ID3D12Device* pD3DDevice = pRenderEnv->m_pDevice->GetD3DObject();
	
	if ((pTexDesc->Flags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL) != 0)
	{
		assert(pRenderEnv->m_pShaderInvisibleDSVHeap != nullptr);
		const DXGI_FORMAT viewFormat = GetDepthStencilViewFormat(pTexDesc->Format);

		if (isTextureArray)
		{
			for (UINT mipSlice = 0; mipSlice < pTexDesc->MipLevels; ++mipSlice)
			{
				Tex1DArrayDepthStencilViewDesc viewDesc(viewFormat, mipSlice, 0, pTexDesc->DepthOrArraySize);
				DescriptorHandle dsvHandle = pRenderEnv->m_pShaderInvisibleDSVHeap->Allocate();
				pD3DDevice->CreateDepthStencilView(GetD3DObject(), &viewDesc, dsvHandle);

				if (mipSlice == 0)
					m_FirstDSVHandle = dsvHandle;
			}
			for (UINT arraySlice = 0; arraySlice < pTexDesc->DepthOrArraySize; ++arraySlice)
			{
				for (UINT mipSlice = 0; mipSlice < pTexDesc->MipLevels; ++mipSlice)
				{
					Tex1DArrayDepthStencilViewDesc viewDesc(viewFormat, mipSlice, arraySlice, 1);
					DescriptorHandle dsvHandle = pRenderEnv->m_pShaderInvisibleDSVHeap->Allocate();
					pD3DDevice->CreateDepthStencilView(GetD3DObject(), &viewDesc, dsvHandle);
				}
			}
		}
		else
		{
			for (UINT mipSlice = 0; mipSlice < pTexDesc->MipLevels; ++mipSlice)
			{
				Tex1DDepthStencilViewDesc viewDesc(mipSlice, viewFormat);
				DescriptorHandle dsvHandle = pRenderEnv->m_pShaderInvisibleDSVHeap->Allocate();
				pD3DDevice->CreateDepthStencilView(GetD3DObject(), &viewDesc, m_FirstDSVHandle);

				if (mipSlice == 0)
					m_FirstDSVHandle = dsvHandle;
			}
		}
	}
	if ((pTexDesc->Flags & D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE) == 0)
	{
		assert(pRenderEnv->m_pShaderInvisibleSRVHeap != nullptr);
		const DXGI_FORMAT viewFormat = GetShaderResourceViewFormat(pTexDesc->Format);
		
		if (isTextureArray)
		{
			Tex1DArrayShaderResourceViewDesc viewDesc(viewFormat, 0, pTexDesc->MipLevels, 0, pTexDesc->DepthOrArraySize);
			m_FirstSRVHandle = pRenderEnv->m_pShaderInvisibleSRVHeap->Allocate();
			pD3DDevice->CreateShaderResourceView(GetD3DObject(), &viewDesc, m_FirstSRVHandle);
		}
		else
		{
			Tex1DShaderResourceViewDesc viewDesc(viewFormat, 0, pTexDesc->MipLevels);
			m_FirstSRVHandle = pRenderEnv->m_pShaderInvisibleSRVHeap->Allocate();
			pD3DDevice->CreateShaderResourceView(GetD3DObject(), &viewDesc, m_FirstSRVHandle);
		}
	}
}

void DepthTexture::CreateTexture2DViews(RenderEnv* pRenderEnv, const D3D12_RESOURCE_DESC* pTexDesc, bool isTextureArray)
{
	ID3D12Device* pD3DDevice = pRenderEnv->m_pDevice->GetD3DObject();
	const bool multisampled = pTexDesc->SampleDesc.Count > 1;
	
	if ((pTexDesc->Flags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL) != 0)
	{
		assert(pRenderEnv->m_pShaderInvisibleDSVHeap != nullptr);
		const DXGI_FORMAT viewFormat = GetDepthStencilViewFormat(pTexDesc->Format);

		if (isTextureArray)
		{
			for (UINT mipSlice = 0; mipSlice < pTexDesc->MipLevels; ++mipSlice)
			{
				Tex2DArrayDepthStencilViewDesc viewDesc(viewFormat, mipSlice, 0, pTexDesc->DepthOrArraySize, multisampled);
				DescriptorHandle dsvHandle = pRenderEnv->m_pShaderInvisibleDSVHeap->Allocate();
				pD3DDevice->CreateDepthStencilView(GetD3DObject(), &viewDesc, dsvHandle);

				if (mipSlice == 0)
					m_FirstDSVHandle = dsvHandle;
			}
			for (UINT arraySlice = 0; arraySlice < pTexDesc->DepthOrArraySize; ++arraySlice)
			{
				for (UINT mipSlice = 0; mipSlice < pTexDesc->MipLevels; ++mipSlice)
				{
					Tex2DArrayDepthStencilViewDesc viewDesc(viewFormat, mipSlice, arraySlice, 1, multisampled);
					DescriptorHandle dsvHandle = pRenderEnv->m_pShaderInvisibleDSVHeap->Allocate();
					pD3DDevice->CreateDepthStencilView(GetD3DObject(), &viewDesc, dsvHandle);
				}
			}
		}
		else
		{
			for (UINT mipSlice = 0; mipSlice < pTexDesc->MipLevels; ++mipSlice)
			{
				Tex2DDepthStencilViewDesc viewDesc(viewFormat, mipSlice, multisampled);
				DescriptorHandle dsvHandle = pRenderEnv->m_pShaderInvisibleDSVHeap->Allocate();
				pD3DDevice->CreateDepthStencilView(GetD3DObject(), &viewDesc, dsvHandle);

				if (mipSlice == 0)
					m_FirstDSVHandle = dsvHandle;
			}
		}
	}
	if ((pTexDesc->Flags & D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE) == 0)
	{
		assert(pRenderEnv->m_pShaderInvisibleSRVHeap != nullptr);
		const DXGI_FORMAT viewFormat = GetShaderResourceViewFormat(pTexDesc->Format);

		if (isTextureArray)
		{
			Tex2DArrayShaderResourceViewDesc viewDesc(viewFormat, 0, pTexDesc->MipLevels, 0, pTexDesc->DepthOrArraySize, multisampled);
			m_FirstSRVHandle = pRenderEnv->m_pShaderInvisibleSRVHeap->Allocate();
			pD3DDevice->CreateShaderResourceView(GetD3DObject(), &viewDesc, m_FirstSRVHandle);
		}
		else
		{
			Tex2DShaderResourceViewDesc viewDesc(viewFormat, 0, pTexDesc->MipLevels, multisampled);
			m_FirstSRVHandle = pRenderEnv->m_pShaderInvisibleSRVHeap->Allocate();
			pD3DDevice->CreateShaderResourceView(GetD3DObject(), &viewDesc, m_FirstSRVHandle);
		}
	}
}

Buffer::Buffer(RenderEnv* pRenderEnv, const D3D12_HEAP_PROPERTIES* pHeapProps,
	const ConstantBufferDesc* pBufferDesc, D3D12_RESOURCE_STATES initialState, LPCWSTR pName)
	: GraphicsResource(pBufferDesc)
{
	CreateCommittedResource(pRenderEnv, pHeapProps, pBufferDesc, initialState, pName);
	CreateBufferView(pRenderEnv, pBufferDesc);
}

Buffer::Buffer(RenderEnv* pRenderEnv, const D3D12_HEAP_PROPERTIES* pHeapProps,
	const VertexBufferDesc* pBufferDesc, D3D12_RESOURCE_STATES initialState, LPCWSTR pName)
	: GraphicsResource(pBufferDesc)
{
	CreateCommittedResource(pRenderEnv, pHeapProps, pBufferDesc, initialState, pName);
	CreateBufferView(pRenderEnv, pBufferDesc);
}

Buffer::Buffer(RenderEnv* pRenderEnv, const D3D12_HEAP_PROPERTIES* pHeapProps,
	const IndexBufferDesc* pBufferDesc, D3D12_RESOURCE_STATES initialState, LPCWSTR pName)
	: GraphicsResource(pBufferDesc)
{
	CreateCommittedResource(pRenderEnv, pHeapProps, pBufferDesc, initialState, pName);
	CreateBufferView(pRenderEnv, pBufferDesc);
}

Buffer::Buffer(RenderEnv* pRenderEnv, const D3D12_HEAP_PROPERTIES* pHeapProps,
	const StructuredBufferDesc* pBufferDesc, D3D12_RESOURCE_STATES initialState, LPCWSTR pName)
	: GraphicsResource(pBufferDesc)
{
	CreateCommittedResource(pRenderEnv, pHeapProps, pBufferDesc, initialState, pName);
	CreateBufferViews(pRenderEnv, pBufferDesc);
}

Buffer::Buffer(RenderEnv* pRenderEnv, const D3D12_HEAP_PROPERTIES* pHeapProps,
	const FormattedBufferDesc* pBufferDesc, D3D12_RESOURCE_STATES initialState, LPCWSTR pName)
	: GraphicsResource(pBufferDesc)
{
	CreateCommittedResource(pRenderEnv, pHeapProps, pBufferDesc, initialState, pName);
	CreateBufferViews(pRenderEnv, pBufferDesc);
}

Buffer::~Buffer()
{
	SafeDelete(m_pVBView);
	SafeDelete(m_pIBView);
}

DescriptorHandle Buffer::GetSRVHandle()
{
	assert(m_SRVHandle.IsValid());
	return m_SRVHandle;
}

DescriptorHandle Buffer::GetUAVHandle()
{
	assert(m_UAVHandle.IsValid());
	return m_UAVHandle;
}

DescriptorHandle Buffer::GetCBVHandle()
{
	assert(m_CBVHandle.IsValid());
	return m_CBVHandle;
}

VertexBufferView* Buffer::GetVBView()
{
	assert(m_pVBView != nullptr);
	return m_pVBView;
}

IndexBufferView* Buffer::GetIBView()
{
	assert(m_pIBView != nullptr);
	return m_pIBView;
}

void Buffer::CreateCommittedResource(RenderEnv* pRenderEnv, const D3D12_HEAP_PROPERTIES* pHeapProps,
	const D3D12_RESOURCE_DESC* pBufferDesc, D3D12_RESOURCE_STATES initialState, LPCWSTR pName)
{
	ID3D12Device* pD3DDevice = pRenderEnv->m_pDevice->GetD3DObject();

	D3D12_HEAP_FLAGS heapFlags = D3D12_HEAP_FLAG_NONE;
	const D3D12_CLEAR_VALUE* pOptimizedClearValue = nullptr;

	VerifyD3DResult(pD3DDevice->CreateCommittedResource(pHeapProps, heapFlags, pBufferDesc,
		initialState, pOptimizedClearValue, IID_PPV_ARGS(&m_D3DResource)));

	m_pVBView = nullptr;
	m_pIBView = nullptr;

#ifdef ENABLE_GRAPHICS_DEBUGGING
	VerifyD3DResult(m_D3DResource->SetName(pName));
#endif // ENABLE_GRAPHICS_DEBUGGING
}

void Buffer::CreateBufferView(RenderEnv* pRenderEnv, const ConstantBufferDesc* pBufferDesc)
{
	ID3D12Device* pD3DDevice = pRenderEnv->m_pDevice->GetD3DObject();
	m_CBVHandle = pRenderEnv->m_pShaderInvisibleSRVHeap->Allocate();
	
	ConstantBufferViewDesc viewDesc(GetD3DObject()->GetGPUVirtualAddress(), (UINT)pBufferDesc->Width);
	pD3DDevice->CreateConstantBufferView(&viewDesc, m_CBVHandle);
}

void Buffer::CreateBufferView(RenderEnv* pRenderEnv, const VertexBufferDesc* pBufferDesc)
{
	ID3D12Device* pD3DDevice = pRenderEnv->m_pDevice->GetD3DObject();
	D3D12_GPU_VIRTUAL_ADDRESS gpuVirtualAddress = GetD3DObject()->GetGPUVirtualAddress();

	m_pVBView = new VertexBufferView(gpuVirtualAddress, (UINT)pBufferDesc->Width, pBufferDesc->StrideInBytes);
}

void Buffer::CreateBufferView(RenderEnv* pRenderEnv, const IndexBufferDesc* pBufferDesc)
{
	ID3D12Device* pD3DDevice = pRenderEnv->m_pDevice->GetD3DObject();
	D3D12_GPU_VIRTUAL_ADDRESS gpuVirtualAddress = GetD3DObject()->GetGPUVirtualAddress();

	m_pIBView = new IndexBufferView(gpuVirtualAddress, (UINT)pBufferDesc->Width, pBufferDesc->StrideInBytes);
}

void Buffer::CreateBufferViews(RenderEnv* pRenderEnv, const StructuredBufferDesc* pBufferDesc)
{
	ID3D12Device* pD3DDevice = pRenderEnv->m_pDevice->GetD3DObject();

	if ((pBufferDesc->Flags & D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE) == 0)
	{
		assert(pRenderEnv->m_pShaderInvisibleSRVHeap != nullptr);
		m_SRVHandle = pRenderEnv->m_pShaderInvisibleSRVHeap->Allocate();

		StructuredBufferSRVDesc viewDesc(0, pBufferDesc->NumElements, pBufferDesc->StructureByteStride);
		pD3DDevice->CreateShaderResourceView(GetD3DObject(), &viewDesc, m_SRVHandle);
	}
	if ((pBufferDesc->Flags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS) != 0)
	{
		assert(pRenderEnv->m_pShaderInvisibleSRVHeap != nullptr);
		m_UAVHandle = pRenderEnv->m_pShaderInvisibleSRVHeap->Allocate();

		StructuredBufferUAVDesc viewDesc(0, pBufferDesc->NumElements, pBufferDesc->StructureByteStride);
		pD3DDevice->CreateUnorderedAccessView(GetD3DObject(), nullptr, &viewDesc, m_UAVHandle);
	}
}

void Buffer::CreateBufferViews(RenderEnv* pRenderEnv, const FormattedBufferDesc* pBufferDesc)
{
	ID3D12Device* pD3DDevice = pRenderEnv->m_pDevice->GetD3DObject();

	if ((pBufferDesc->Flags & D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE) == 0)
	{
		assert(pRenderEnv->m_pShaderInvisibleSRVHeap != nullptr);
		m_SRVHandle = pRenderEnv->m_pShaderInvisibleSRVHeap->Allocate();

		FormattedBufferSRVDesc viewDesc(0, pBufferDesc->NumElements, pBufferDesc->SRVFormat);
		pD3DDevice->CreateShaderResourceView(GetD3DObject(), &viewDesc, m_SRVHandle);
	}
	if ((pBufferDesc->Flags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS) != 0)
	{
		assert(pRenderEnv->m_pShaderInvisibleSRVHeap != nullptr);
		m_UAVHandle = pRenderEnv->m_pShaderInvisibleSRVHeap->Allocate();

		FormattedBufferUAVDesc viewDesc(0, pBufferDesc->NumElements, pBufferDesc->UAVFormat);
		pD3DDevice->CreateUnorderedAccessView(GetD3DObject(), nullptr, &viewDesc, m_UAVHandle);
	}
}

Sampler::Sampler(RenderEnv* pRenderEnv, const D3D12_SAMPLER_DESC* pDesc)
{
	assert(pRenderEnv->m_pShaderInvisibleSamplerHeap != nullptr);
	m_Handle = pRenderEnv->m_pShaderInvisibleSamplerHeap->Allocate();

	ID3D12Device* pD3DDevice = pRenderEnv->m_pDevice->GetD3DObject();
	pD3DDevice->CreateSampler(pDesc, m_Handle);
}

TextureCopyLocation::TextureCopyLocation(GraphicsResource* pGraphicsResource, UINT subresourceIndex)
{
	pResource = pGraphicsResource->GetD3DObject();
	Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
	SubresourceIndex = subresourceIndex;
}

TextureCopyLocation::TextureCopyLocation(GraphicsResource* pGraphicsResource, const D3D12_PLACED_SUBRESOURCE_FOOTPRINT& footprint)
{
	pResource = pGraphicsResource->GetD3DObject();
	Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
	PlacedFootprint = footprint;
}

RayTracingTrianglesGeometryDesc::RayTracingTrianglesGeometryDesc(DXGI_FORMAT vertexFormat, Buffer* pVertexBuffer, Buffer* pIndexBuffer,
	D3D12_RAYTRACING_GEOMETRY_FLAGS flags, Buffer* pTransformBuffer)
{
	Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
	Flags = flags;

	Triangles.Transform3x4 = (pTransformBuffer != nullptr) ? pTransformBuffer->GetD3DObject()->GetGPUVirtualAddress() : 0;
	
	assert(pVertexBuffer != nullptr);
	const VertexBufferView* pVBView = pVertexBuffer->GetVBView();
	
	Triangles.VertexFormat = vertexFormat;
	Triangles.VertexCount = pVBView->SizeInBytes / pVBView->StrideInBytes;
	Triangles.VertexBuffer.StartAddress = pVBView->BufferLocation;
	Triangles.VertexBuffer.StrideInBytes = pVBView->StrideInBytes;

	if (pIndexBuffer != nullptr)
	{
		const IndexBufferView* pIBView = pIndexBuffer->GetIBView();

		Triangles.IndexFormat = pIBView->Format;
		Triangles.IndexCount = (pIBView->Format == DXGI_FORMAT_R16_UINT) ? (pIBView->SizeInBytes / sizeof(u16)) : (pIBView->SizeInBytes / sizeof(u32));
		Triangles.IndexBuffer = pIBView->BufferLocation;
	}
	else
	{
		Triangles.IndexFormat = DXGI_FORMAT_UNKNOWN;
		Triangles.IndexCount = 0;
		Triangles.IndexBuffer = 0;
	}
}
