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
			return DXGI_FORMAT_R32_FLOAT;
		case DXGI_FORMAT_R24G8_TYPELESS:
			return DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
		case DXGI_FORMAT_R10G10B10A2_UNORM:
			return DXGI_FORMAT_R10G10B10A2_UNORM;
		case DXGI_FORMAT_R8G8B8A8_SNORM:
			return DXGI_FORMAT_R8G8B8A8_SNORM;
		case DXGI_FORMAT_R8G8B8A8_UNORM:
			return DXGI_FORMAT_R8G8B8A8_UNORM;
		case DXGI_FORMAT_R16G16B16A16_FLOAT:
			return DXGI_FORMAT_R16G16B16A16_FLOAT;
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
	}
	assert(false);
	return resourceFormat;
}

ResourceBarrier::ResourceBarrier(GraphicsResource* pResource, D3D12_RESOURCE_STATES stateBefore, D3D12_RESOURCE_STATES stateAfter, UINT subresource, D3D12_RESOURCE_BARRIER_FLAGS flags)
{
	Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	Flags = flags;
	Transition.pResource = pResource->GetD3DObject();
	Transition.StateBefore = stateBefore;
	Transition.StateAfter = stateAfter;
	Transition.Subresource = subresource;
}

VertexBufferView::VertexBufferView(D3D12_GPU_VIRTUAL_ADDRESS bufferLocation, UINT sizeInBytes, UINT strideInBytes)
{
	BufferLocation = bufferLocation;
	SizeInBytes = sizeInBytes;
	StrideInBytes = strideInBytes;
}

IndexBufferView::IndexBufferView(D3D12_GPU_VIRTUAL_ADDRESS bufferLocation, UINT sizeInBytes, UINT strideInBytes)
{
	assert((strideInBytes == sizeof(u16)) || (strideInBytes == sizeof(u32)));
	
	BufferLocation = bufferLocation;
	SizeInBytes = sizeInBytes;
	Format = (strideInBytes == sizeof(u16)) ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;
}

ConstantBufferViewDesc::ConstantBufferViewDesc(D3D12_GPU_VIRTUAL_ADDRESS bufferLocation, UINT sizeInBytes)
{
	BufferLocation = bufferLocation;
	SizeInBytes = sizeInBytes;
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
	UINT16 arraySize, UINT16 mipLevels, D3D12_TEXTURE_LAYOUT layout, UINT64 alignment)
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

	Flags = D3D12_RESOURCE_FLAG_NONE;
	if (createRTV)
		Flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
	if (!createSRV)
		Flags |= D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
	if (createUAV)
		Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
}

ColorTexture2DDesc::ColorTexture2DDesc(DXGI_FORMAT format, UINT64 width, UINT height, bool createRTV, bool createSRV, bool createUAV,
	UINT16 arraySize, UINT16 mipLevels, UINT sampleCount, UINT sampleQuality, D3D12_TEXTURE_LAYOUT layout, UINT64 alignment)
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
	UINT16 arraySize, UINT16 mipLevels, D3D12_TEXTURE_LAYOUT layout, UINT64 alignment)
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
	
	Flags = D3D12_RESOURCE_FLAG_NONE;
	if (createDSV)
		Flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
	if (!createSRV)
		Flags |= D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
}

DepthTexture2DDesc::DepthTexture2DDesc(DXGI_FORMAT format, UINT64 width, UINT height, bool createDSV, bool createSRV,
	UINT16 arraySize, UINT16 mipLevels, UINT sampleCount, UINT sampleQuality, D3D12_TEXTURE_LAYOUT layout, UINT64 alignment)
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
	
	Flags = D3D12_RESOURCE_FLAG_NONE;
	if (createDSV)
		Flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
	if (!createSRV)
		Flags |= D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
}

DepthTexture3DDesc::DepthTexture3DDesc(DXGI_FORMAT format, UINT64 width, UINT height, UINT16 depth,
	bool createDSV, bool createSRV, UINT16 mipLevels, D3D12_TEXTURE_LAYOUT layout, UINT64 alignment)
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
	if (createDSV)
		Flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
	if (!createSRV)
		Flags |= D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
}

Tex1DRenderTargetViewDesc::Tex1DRenderTargetViewDesc(UINT mipSlice, UINT arraySize, DXGI_FORMAT format)
{
	Format = format;
	if (arraySize > 1)
	{
		ViewDimension = D3D12_RTV_DIMENSION_TEXTURE1DARRAY;
		Texture1DArray.MipSlice = mipSlice;
		Texture1DArray.FirstArraySlice = 0;
		Texture1DArray.ArraySize = arraySize;
	}
	else
	{
		ViewDimension = D3D12_RTV_DIMENSION_TEXTURE1D;
		Texture1D.MipSlice = mipSlice;
	}
}

Tex2DRenderTargetViewDesc::Tex2DRenderTargetViewDesc(UINT mipSlice, UINT arraySize, bool multisampled, DXGI_FORMAT format, UINT planeSlice)
{
	Format = format;
	if (arraySize > 1)
	{
		if (multisampled)
		{
			ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DMSARRAY;
			Texture2DMSArray.FirstArraySlice = 0;
			Texture2DMSArray.ArraySize = arraySize;
		}
		else
		{
			ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
			Texture2DArray.MipSlice = mipSlice;
			Texture2DArray.FirstArraySlice = 0;
			Texture2DArray.ArraySize = arraySize;
			Texture2DArray.PlaneSlice = planeSlice;
		}
	}
	else
	{
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
}

Tex3DRenderTargetViewDesc::Tex3DRenderTargetViewDesc(UINT mipSlice, UINT firstDepthSlice, UINT depthSliceCount, DXGI_FORMAT format)
{
	ViewDimension = D3D12_RTV_DIMENSION_TEXTURE3D;
	Format = format;
	Texture3D.MipSlice = mipSlice;
	Texture3D.FirstWSlice = firstDepthSlice;
	Texture3D.WSize = depthSliceCount;
}

Tex1DDepthStencilViewDesc::Tex1DDepthStencilViewDesc(UINT mipSlice, UINT arraySize, DXGI_FORMAT format, D3D12_DSV_FLAGS flags)
{
	Format = format;
	Flags = flags;

	if (arraySize > 1)
	{
		ViewDimension = D3D12_DSV_DIMENSION_TEXTURE1DARRAY;
		Texture1DArray.MipSlice = mipSlice;
		Texture1DArray.FirstArraySlice = 0;
		Texture1DArray.ArraySize = arraySize;
	}
	else
	{
		ViewDimension = D3D12_DSV_DIMENSION_TEXTURE1D;
		Texture1D.MipSlice = mipSlice;
	}
}

Tex2DDepthStencilViewDesc::Tex2DDepthStencilViewDesc(DXGI_FORMAT format, UINT mipSlice, UINT arraySize, bool multisampled, D3D12_DSV_FLAGS flags)
{
	Format = format;
	Flags = flags;

	if (arraySize > 1)
	{
		if (multisampled)
		{
			ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DMSARRAY;
			Texture2DMSArray.FirstArraySlice = 0;
			Texture2DMSArray.ArraySize = arraySize;
		}
		else
		{
			ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
			Texture2DArray.MipSlice = mipSlice;
			Texture2DArray.FirstArraySlice = 0;
			Texture2DArray.ArraySize = arraySize;
		}
	}
	else
	{
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
}

Tex1DShaderResourceViewDesc::Tex1DShaderResourceViewDesc(DXGI_FORMAT format, UINT mostDetailedMip, UINT mipLevels, UINT arraySize,
	FLOAT minLODClamp, UINT shader4ComponentMapping)
{
	Format = format;
	Shader4ComponentMapping = shader4ComponentMapping;

	if (arraySize > 1)
	{
		ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1DARRAY;
		Texture1DArray.MostDetailedMip = mostDetailedMip;
		Texture1DArray.MipLevels = mipLevels;
		Texture1DArray.FirstArraySlice = 0;
		Texture1DArray.ArraySize = arraySize;
		Texture1DArray.ResourceMinLODClamp = minLODClamp;
	}
	else
	{
		ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1D;
		Texture1D.MostDetailedMip = mostDetailedMip;
		Texture1D.MipLevels = mipLevels;
		Texture1D.ResourceMinLODClamp = minLODClamp;
	}
}

Tex2DShaderResourceViewDesc::Tex2DShaderResourceViewDesc(DXGI_FORMAT format, UINT mostDetailedMip, UINT mipLevels, UINT arraySize, bool multisampled,
	FLOAT minLODClamp, UINT shader4ComponentMapping, UINT planeSlice)
{
	Format = format;
	Shader4ComponentMapping = shader4ComponentMapping;

	if (arraySize > 1)
	{
		if (multisampled)
		{
			ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMSARRAY;
			Texture2DMSArray.FirstArraySlice = 0;
			Texture2DMSArray.ArraySize = arraySize;
		}
		else
		{
			ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
			Texture2DArray.MostDetailedMip = mostDetailedMip;
			Texture2DArray.MipLevels = mipLevels;
			Texture2DArray.FirstArraySlice = 0;
			Texture2DArray.ArraySize = arraySize;
			Texture2DArray.PlaneSlice = planeSlice;
			Texture2DArray.ResourceMinLODClamp = minLODClamp;
		}
	}
	else
	{
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

TexCubeShaderResourceViewDesc::TexCubeShaderResourceViewDesc(DXGI_FORMAT format, UINT mostDetailedMip, UINT mipLevels, UINT numCubes,
	FLOAT minLODClamp, UINT shader4ComponentMapping)
{
	Format = format;
	Shader4ComponentMapping = shader4ComponentMapping;

	if (numCubes > 1)
	{
		ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBEARRAY;
		TextureCubeArray.MostDetailedMip = mostDetailedMip;
		TextureCubeArray.MipLevels = mipLevels;
		TextureCubeArray.First2DArrayFace = 0;
		TextureCubeArray.NumCubes = numCubes;
		TextureCubeArray.ResourceMinLODClamp = minLODClamp;
	}
	else
	{
		ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
		TextureCube.MostDetailedMip = mostDetailedMip;
		TextureCube.MipLevels = mipLevels;
		TextureCube.ResourceMinLODClamp = minLODClamp;
	}
}

Tex1DUnorderedAccessViewDesc::Tex1DUnorderedAccessViewDesc(DXGI_FORMAT format, UINT mipSlice)
{
	Format = format;
	ViewDimension = D3D12_UAV_DIMENSION_TEXTURE1D;
	Texture1D.MipSlice = mipSlice;
}

Tex2DUnorderedAccessViewDesc::Tex2DUnorderedAccessViewDesc(DXGI_FORMAT format, UINT mipSlice, UINT planeSlice)
{
	Format = format;
	ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
	Texture2D.MipSlice = mipSlice;
	Texture2D.PlaneSlice = planeSlice;
}

Tex3DUnorderedAccessViewDesc::Tex3DUnorderedAccessViewDesc(DXGI_FORMAT format, UINT mipSlice, UINT firstDepthSlice, UINT depthSliceCount)
{
	Format = format;
	ViewDimension = D3D12_UAV_DIMENSION_TEXTURE3D;
	Texture3D.MipSlice = mipSlice;
	Texture3D.FirstWSlice = firstDepthSlice;
	Texture3D.WSize = depthSliceCount;
}

GraphicsResource::GraphicsResource(ComPtr<ID3D12Resource> d3dResource, D3D12_RESOURCE_STATES initialState)
	: m_D3DResource(d3dResource)
	, m_Desc(d3dResource->GetDesc())
	, m_State(initialState)
	, m_ReadState(D3D12_RESOURCE_STATE_COMMON)
	, m_WriteState(D3D12_RESOURCE_STATE_COMMON)
{
}

GraphicsResource::GraphicsResource(const D3D12_RESOURCE_DESC* pDesc, D3D12_RESOURCE_STATES initialState)
	: m_Desc(*pDesc)
	, m_State(initialState)
	, m_ReadState(D3D12_RESOURCE_STATE_COMMON)
	, m_WriteState(D3D12_RESOURCE_STATE_COMMON)
{
}

void GraphicsResource::Write(const void* pInputData, SIZE_T numBytes)
{
	const UINT subresource = 0;
	void* pResourceData = nullptr;

	MemoryRange readRange(0, 0);
	VerifyD3DResult(GetD3DObject()->Map(subresource, &readRange, reinterpret_cast<void**>(&pResourceData)));

	std::memcpy(pResourceData, pInputData, numBytes);

	MemoryRange writtenRange(0, numBytes);
	GetD3DObject()->Unmap(subresource, &writtenRange);
}

void GraphicsResource::Read(void* pOutputData, SIZE_T numBytes)
{
	const UINT subresource = 0;
	void* pResourceData = nullptr;

	MemoryRange readRange(0, numBytes);
	VerifyD3DResult(GetD3DObject()->Map(subresource, &readRange, reinterpret_cast<void**>(&pResourceData)));

	std::memcpy(pOutputData, pResourceData, numBytes);

	MemoryRange writtenRange(0, 0);
	GetD3DObject()->Unmap(subresource, &writtenRange);
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

ColorTexture::ColorTexture(RenderEnv* pRenderEnv, const D3D12_HEAP_PROPERTIES* pHeapProps,
	const ColorTexture1DDesc* pTexDesc, D3D12_RESOURCE_STATES initialState,
	const FLOAT optimizedClearColor[4], LPCWSTR pName)
	: GraphicsResource(pTexDesc, initialState)
{
	CreateCommittedResource(pRenderEnv, pHeapProps, pTexDesc, initialState, optimizedClearColor, pName);
	CreateTex1DViews(pRenderEnv, pTexDesc);
	DetermineResourceStates(pTexDesc);
}

ColorTexture::ColorTexture(RenderEnv* pRenderEnv, const D3D12_HEAP_PROPERTIES* pHeapProps,
	const ColorTexture2DDesc* pTexDesc, D3D12_RESOURCE_STATES initialState,
	const FLOAT optimizedClearColor[4], LPCWSTR pName)
	: GraphicsResource(pTexDesc, initialState)
{
	CreateCommittedResource(pRenderEnv, pHeapProps, pTexDesc, initialState, optimizedClearColor, pName);
	CreateTex2DViews(pRenderEnv, pTexDesc);
	DetermineResourceStates(pTexDesc);
}

ColorTexture::ColorTexture(RenderEnv* pRenderEnv, const D3D12_HEAP_PROPERTIES* pHeapProps,
	const ColorTexture3DDesc* pTexDesc, D3D12_RESOURCE_STATES initialState,
	const FLOAT optimizedClearColor[4], LPCWSTR pName)
	: GraphicsResource(pTexDesc, initialState)
{
	CreateCommittedResource(pRenderEnv, pHeapProps, pTexDesc, initialState, optimizedClearColor, pName);
	CreateTex3DViews(pRenderEnv, pTexDesc);
	DetermineResourceStates(pTexDesc);
}

ColorTexture::ColorTexture(RenderEnv* pRenderEnv, ComPtr<ID3D12Resource> d3dResource, D3D12_RESOURCE_STATES initialState, LPCWSTR pName)
	: GraphicsResource(d3dResource, initialState)
{
	if (m_Desc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE1D)
		CreateTex1DViews(pRenderEnv, &m_Desc);
	if (m_Desc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE2D)
		CreateTex2DViews(pRenderEnv, &m_Desc);
	if (m_Desc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE3D)
		CreateTex3DViews(pRenderEnv, &m_Desc);

	DetermineResourceStates(&m_Desc);
}

DescriptorHandle ColorTexture::GetRTVHandle()
{
	assert(m_RTVHandle.IsValid());
	return m_RTVHandle;
}

DescriptorHandle ColorTexture::GetSRVHandle()
{
	assert(m_SRVHandle.IsValid());
	return m_SRVHandle;
}

DescriptorHandle ColorTexture::GetUAVHandle()
{
	assert(m_UAVHandle.IsValid());
	return m_UAVHandle;
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

#ifdef _DEBUG
	VerifyD3DResult(m_D3DResource->SetName(pName));
#endif
}

void ColorTexture::DetermineResourceStates(const D3D12_RESOURCE_DESC* pTexDesc)
{
	m_WriteState = D3D12_RESOURCE_STATE_COMMON;
	
	if ((pTexDesc->Flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET) != 0)
		m_WriteState |= D3D12_RESOURCE_STATE_RENDER_TARGET;

	if ((pTexDesc->Flags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS) != 0)
		m_WriteState |= D3D12_RESOURCE_STATE_UNORDERED_ACCESS;

	m_ReadState = D3D12_RESOURCE_STATE_COMMON;
	if ((pTexDesc->Flags & D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE) == 0)
	{
		m_ReadState |= D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
		m_ReadState |= D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	}
}

void ColorTexture::CreateTex1DViews(RenderEnv* pRenderEnv, const D3D12_RESOURCE_DESC* pTexDesc)
{
	ID3D12Device* pD3DDevice = pRenderEnv->m_pDevice->GetD3DObject();

	// Kolya: Add missing impl
	assert(pTexDesc->DepthOrArraySize == 1);
	assert(pTexDesc->MipLevels == 1);

	if ((pTexDesc->Flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET) != 0)
	{
		assert(pRenderEnv->m_pShaderInvisibleRTVHeap != nullptr);
		m_RTVHandle = pRenderEnv->m_pShaderInvisibleRTVHeap->Allocate();

		Tex1DRenderTargetViewDesc viewDesc;
		pD3DDevice->CreateRenderTargetView(GetD3DObject(), &viewDesc, m_RTVHandle);
	}
	if ((pTexDesc->Flags & D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE) == 0)
	{
		assert(pRenderEnv->m_pShaderInvisibleSRVHeap != nullptr);
		m_SRVHandle = pRenderEnv->m_pShaderInvisibleSRVHeap->Allocate();

		Tex1DShaderResourceViewDesc viewDesc(GetShaderResourceViewFormat(pTexDesc->Format));
		pD3DDevice->CreateShaderResourceView(GetD3DObject(), &viewDesc, m_SRVHandle);
	}
	if ((pTexDesc->Flags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS) != 0)
	{
		assert(pRenderEnv->m_pShaderInvisibleSRVHeap != nullptr);
		m_UAVHandle = pRenderEnv->m_pShaderInvisibleSRVHeap->Allocate();

		Tex1DUnorderedAccessViewDesc viewDesc(GetUnorderedAccessViewFormat(pTexDesc->Format));
		pD3DDevice->CreateUnorderedAccessView(GetD3DObject(), nullptr, &viewDesc, m_UAVHandle);
	}
}

void ColorTexture::CreateTex2DViews(RenderEnv* pRenderEnv, const D3D12_RESOURCE_DESC* pTexDesc)
{
	ID3D12Device* pD3DDevice = pRenderEnv->m_pDevice->GetD3DObject();

	assert(pTexDesc->DepthOrArraySize == 1);
	assert(pTexDesc->MipLevels == 1);
	assert(pTexDesc->SampleDesc.Count == 1);
	assert(pTexDesc->SampleDesc.Quality == 0);

	if ((pTexDesc->Flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET) != 0)
	{
		assert(pRenderEnv->m_pShaderInvisibleRTVHeap != nullptr);
		m_RTVHandle = pRenderEnv->m_pShaderInvisibleRTVHeap->Allocate();

		Tex2DRenderTargetViewDesc viewDesc;
		pD3DDevice->CreateRenderTargetView(GetD3DObject(), &viewDesc, m_RTVHandle);
	}
	if ((pTexDesc->Flags & D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE) == 0)
	{
		assert(pRenderEnv->m_pShaderInvisibleSRVHeap != nullptr);
		m_SRVHandle = pRenderEnv->m_pShaderInvisibleSRVHeap->Allocate();

		Tex2DShaderResourceViewDesc viewDesc(GetShaderResourceViewFormat(pTexDesc->Format));
		pD3DDevice->CreateShaderResourceView(GetD3DObject(), &viewDesc, m_SRVHandle);
	}
	if ((pTexDesc->Flags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS) != 0)
	{
		assert(pRenderEnv->m_pShaderInvisibleSRVHeap != nullptr);
		m_UAVHandle = pRenderEnv->m_pShaderInvisibleSRVHeap->Allocate();

		Tex2DUnorderedAccessViewDesc viewDesc(GetUnorderedAccessViewFormat(pTexDesc->Format));
		pD3DDevice->CreateUnorderedAccessView(GetD3DObject(), nullptr, &viewDesc, m_UAVHandle);
	}
}

void ColorTexture::CreateTex3DViews(RenderEnv* pRenderEnv, const D3D12_RESOURCE_DESC* pTexDesc)
{
	ID3D12Device* pD3DDevice = pRenderEnv->m_pDevice->GetD3DObject();

	assert(pTexDesc->MipLevels == 1);
	assert(pTexDesc->SampleDesc.Count == 1);
	assert(pTexDesc->SampleDesc.Quality == 0);

	if ((pTexDesc->Flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET) != 0)
	{
		assert(pRenderEnv->m_pShaderInvisibleRTVHeap != nullptr);
		m_RTVHandle = pRenderEnv->m_pShaderInvisibleRTVHeap->Allocate();

		Tex3DRenderTargetViewDesc viewDesc;
		pD3DDevice->CreateRenderTargetView(GetD3DObject(), &viewDesc, m_RTVHandle);
	}
	if ((pTexDesc->Flags & D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE) == 0)
	{
		assert(pRenderEnv->m_pShaderInvisibleSRVHeap != nullptr);
		m_SRVHandle = pRenderEnv->m_pShaderInvisibleSRVHeap->Allocate();

		Tex3DShaderResourceViewDesc viewDesc(GetShaderResourceViewFormat(pTexDesc->Format));
		pD3DDevice->CreateShaderResourceView(GetD3DObject(), &viewDesc, m_SRVHandle);
	}
	if ((pTexDesc->Flags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS) != 0)
	{
		assert(pRenderEnv->m_pShaderInvisibleSRVHeap != nullptr);
		m_UAVHandle = pRenderEnv->m_pShaderInvisibleSRVHeap->Allocate();

		Tex3DUnorderedAccessViewDesc viewDesc(GetUnorderedAccessViewFormat(pTexDesc->Format));
		pD3DDevice->CreateUnorderedAccessView(GetD3DObject(), nullptr, &viewDesc, m_UAVHandle);
	}
}

DepthTexture::DepthTexture(RenderEnv* pRenderEnv, const D3D12_HEAP_PROPERTIES* pHeapProps,
	const DepthTexture1DDesc* pTexDesc, D3D12_RESOURCE_STATES initialState,
	const DepthStencilValue* pOptimizedClearDepth, LPCWSTR pName)
	: GraphicsResource(pTexDesc, initialState)
{
	CreateCommittedResource(pRenderEnv, pHeapProps, pTexDesc, initialState, pOptimizedClearDepth, pName);
	CreateTex1DViews(pRenderEnv, pTexDesc);
	DetermineResourceStates(pTexDesc);
}

DepthTexture::DepthTexture(RenderEnv* pRenderEnv, const D3D12_HEAP_PROPERTIES* pHeapProps,
	const DepthTexture2DDesc* pTexDesc, D3D12_RESOURCE_STATES initialState,
	const DepthStencilValue* pOptimizedClearDepth, LPCWSTR pName)
	: GraphicsResource(pTexDesc, initialState)
{
	CreateCommittedResource(pRenderEnv, pHeapProps, pTexDesc, initialState, pOptimizedClearDepth, pName);
	CreateTex2DViews(pRenderEnv, pTexDesc);
	DetermineResourceStates(pTexDesc);
}

DepthTexture::DepthTexture(RenderEnv* pRenderEnv, const D3D12_HEAP_PROPERTIES* pHeapProps,
	const DepthTexture3DDesc* pTexDesc, D3D12_RESOURCE_STATES initialState,
	const DepthStencilValue* pOptimizedClearDepth, LPCWSTR pName)
	: GraphicsResource(pTexDesc, initialState)
{
	CreateCommittedResource(pRenderEnv, pHeapProps, pTexDesc, initialState, pOptimizedClearDepth, pName);
	CreateTex3DViews(pRenderEnv, pTexDesc);
	DetermineResourceStates(pTexDesc);
}

DescriptorHandle DepthTexture::GetDSVHandle()
{
	assert(m_DSVHandle.IsValid());
	return m_DSVHandle;
}

DescriptorHandle DepthTexture::GetSRVHandle()
{
	assert(m_SRVHandle.IsValid());
	return m_SRVHandle;
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

#ifdef _DEBUG
	VerifyD3DResult(m_D3DResource->SetName(pName));
#endif
}

void DepthTexture::CreateTex1DViews(RenderEnv* pRenderEnv, const D3D12_RESOURCE_DESC* pTexDesc)
{
	assert(false && "Kolya: Needs impl");
	if ((pTexDesc->Flags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL) != 0)
	{
		assert(pRenderEnv->m_pShaderInvisibleDSVHeap != nullptr);
	}
	if ((pTexDesc->Flags & D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE) == 0)
	{
		assert(pRenderEnv->m_pShaderInvisibleSRVHeap != nullptr);
	}
}

void DepthTexture::CreateTex2DViews(RenderEnv* pRenderEnv, const D3D12_RESOURCE_DESC* pTexDesc)
{
	ID3D12Device* pD3DDevice = pRenderEnv->m_pDevice->GetD3DObject();

	assert(pTexDesc->DepthOrArraySize == 1);
	assert(pTexDesc->MipLevels == 1);
	assert(pTexDesc->SampleDesc.Count == 1);
	assert(pTexDesc->SampleDesc.Quality == 0);

	if ((pTexDesc->Flags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL) != 0)
	{
		assert(pRenderEnv->m_pShaderInvisibleDSVHeap != nullptr);
		m_DSVHandle = pRenderEnv->m_pShaderInvisibleDSVHeap->Allocate();

		Tex2DDepthStencilViewDesc viewDesc(GetDepthStencilViewFormat(pTexDesc->Format));
		pD3DDevice->CreateDepthStencilView(GetD3DObject(), &viewDesc, m_DSVHandle);
	}
	if ((pTexDesc->Flags & D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE) == 0)
	{
		assert(pRenderEnv->m_pShaderInvisibleSRVHeap != nullptr);
		m_SRVHandle = pRenderEnv->m_pShaderInvisibleSRVHeap->Allocate();

		Tex2DShaderResourceViewDesc viewDesc(GetShaderResourceViewFormat(pTexDesc->Format));
		pD3DDevice->CreateShaderResourceView(GetD3DObject(), &viewDesc, m_SRVHandle);
	}
}

void DepthTexture::CreateTex3DViews(RenderEnv* pRenderEnv, const D3D12_RESOURCE_DESC* pTexDesc)
{
	assert(false && "Kolya: Needs impl");
	if ((pTexDesc->Flags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL) != 0)
	{
		assert(pRenderEnv->m_pShaderInvisibleDSVHeap != nullptr);
	}
	if ((pTexDesc->Flags & D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE) == 0)
	{
		assert(pRenderEnv->m_pShaderInvisibleSRVHeap != nullptr);
	}
}

void DepthTexture::DetermineResourceStates(const D3D12_RESOURCE_DESC* pTexDesc)
{
	m_WriteState = D3D12_RESOURCE_STATE_COMMON;
	if ((pTexDesc->Flags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL) != 0)
		m_WriteState |= D3D12_RESOURCE_STATE_DEPTH_WRITE;
	
	m_ReadState = D3D12_RESOURCE_STATE_COMMON;
	if ((pTexDesc->Flags & D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE) == 0)
	{
		m_ReadState |= D3D12_RESOURCE_STATE_DEPTH_READ;
		m_ReadState |= D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
		m_ReadState |= D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	}
}

Buffer::Buffer(RenderEnv* pRenderEnv, const D3D12_HEAP_PROPERTIES* pHeapProps,
	const ConstantBufferDesc* pBufferDesc, D3D12_RESOURCE_STATES initialState, LPCWSTR pName)
	: GraphicsResource(pBufferDesc, initialState)
{
	CreateCommittedResource(pRenderEnv, pHeapProps, pBufferDesc, initialState, pName);
	CreateConstantBufferView(pRenderEnv, pBufferDesc);

	m_WriteState = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
	m_ReadState = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
}

Buffer::Buffer(RenderEnv* pRenderEnv, const D3D12_HEAP_PROPERTIES* pHeapProps,
	const VertexBufferDesc* pBufferDesc, D3D12_RESOURCE_STATES initialState, LPCWSTR pName)
	: GraphicsResource(pBufferDesc, initialState)
{
	CreateCommittedResource(pRenderEnv, pHeapProps, pBufferDesc, initialState, pName);
	CreateVertexBufferView(pRenderEnv, pBufferDesc);

	m_WriteState = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
	m_ReadState = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
}

Buffer::Buffer(RenderEnv* pRenderEnv, const D3D12_HEAP_PROPERTIES* pHeapProps,
	const IndexBufferDesc* pBufferDesc, D3D12_RESOURCE_STATES initialState, LPCWSTR pName)
	: GraphicsResource(pBufferDesc, initialState)
{
	CreateCommittedResource(pRenderEnv, pHeapProps, pBufferDesc, initialState, pName);
	CreateIndexBufferView(pRenderEnv, pBufferDesc);

	m_WriteState = D3D12_RESOURCE_STATE_INDEX_BUFFER;
	m_ReadState = D3D12_RESOURCE_STATE_INDEX_BUFFER;
}

Buffer::Buffer(RenderEnv* pRenderEnv, const D3D12_HEAP_PROPERTIES* pHeapProps,
	const StructuredBufferDesc* pBufferDesc, D3D12_RESOURCE_STATES initialState, LPCWSTR pName)
	: GraphicsResource(pBufferDesc, initialState)
{
	CreateCommittedResource(pRenderEnv, pHeapProps, pBufferDesc, initialState, pName);
	CreateStructuredBufferViews(pRenderEnv, pBufferDesc);

	m_WriteState = D3D12_RESOURCE_STATE_COMMON;
	if ((pBufferDesc->Flags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS) != 0)
		m_WriteState |= D3D12_RESOURCE_STATE_UNORDERED_ACCESS;

	m_ReadState = D3D12_RESOURCE_STATE_COMMON;
	if ((pBufferDesc->Flags & D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE) == 0)
	{
		m_ReadState |= D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
		m_ReadState |= D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	}
}

Buffer::Buffer(RenderEnv* pRenderEnv, const D3D12_HEAP_PROPERTIES* pHeapProps,
	const FormattedBufferDesc* pBufferDesc, D3D12_RESOURCE_STATES initialState, LPCWSTR pName)
	: GraphicsResource(pBufferDesc, initialState)
{
	CreateCommittedResource(pRenderEnv, pHeapProps, pBufferDesc, initialState, pName);
	CreateFormattedBufferViews(pRenderEnv, pBufferDesc);

	m_WriteState = D3D12_RESOURCE_STATE_COMMON;
	if ((pBufferDesc->Flags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS) != 0)
		m_WriteState |= D3D12_RESOURCE_STATE_UNORDERED_ACCESS;

	m_ReadState = D3D12_RESOURCE_STATE_COMMON;
	if ((pBufferDesc->Flags & D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE) == 0)
	{
		m_ReadState |= D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
		m_ReadState |= D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	}
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

#ifdef _DEBUG
	VerifyD3DResult(m_D3DResource->SetName(pName));
#endif
}

void Buffer::CreateConstantBufferView(RenderEnv* pRenderEnv, const ConstantBufferDesc* pBufferDesc)
{
	ID3D12Device* pD3DDevice = pRenderEnv->m_pDevice->GetD3DObject();
	m_CBVHandle = pRenderEnv->m_pShaderInvisibleSRVHeap->Allocate();
	
	ConstantBufferViewDesc viewDesc(GetD3DObject()->GetGPUVirtualAddress(), (UINT)pBufferDesc->Width);
	pD3DDevice->CreateConstantBufferView(&viewDesc, m_CBVHandle);
}

void Buffer::CreateVertexBufferView(RenderEnv* pRenderEnv, const VertexBufferDesc* pBufferDesc)
{
	ID3D12Device* pD3DDevice = pRenderEnv->m_pDevice->GetD3DObject();
	D3D12_GPU_VIRTUAL_ADDRESS gpuVirtualAddress = GetD3DObject()->GetGPUVirtualAddress();

	m_pVBView = new VertexBufferView(gpuVirtualAddress, (UINT)pBufferDesc->Width, pBufferDesc->StrideInBytes);
}

void Buffer::CreateIndexBufferView(RenderEnv* pRenderEnv, const IndexBufferDesc* pBufferDesc)
{
	ID3D12Device* pD3DDevice = pRenderEnv->m_pDevice->GetD3DObject();
	D3D12_GPU_VIRTUAL_ADDRESS gpuVirtualAddress = GetD3DObject()->GetGPUVirtualAddress();

	m_pIBView = new IndexBufferView(gpuVirtualAddress, (UINT)pBufferDesc->Width, pBufferDesc->StrideInBytes);
}

void Buffer::CreateStructuredBufferViews(RenderEnv* pRenderEnv, const StructuredBufferDesc* pBufferDesc)
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

void Buffer::CreateFormattedBufferViews(RenderEnv* pRenderEnv, const FormattedBufferDesc* pBufferDesc)
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
