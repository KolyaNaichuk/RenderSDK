#include "DX/DXResource.h"
#include "DX/DXDevice.h"
#include "DX/DXRenderEnvironment.h"
#include "DX/DXUtils.h"

DXDepthStencilValue::DXDepthStencilValue(FLOAT depth, UINT8 stencil)
{
	Depth = depth;
	Stencil = stencil;
}

DXClearValue::DXClearValue(DXGI_FORMAT format, const FLOAT color[4])
{
	Format = format;
	Color[0] = color[0];
	Color[1] = color[1];
	Color[2] = color[2];
	Color[3] = color[3];
}

DXClearValue::DXClearValue(DXGI_FORMAT format, const DXDepthStencilValue* pDepthStencilValue)
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
	}
	assert(false);
	return resourceFormat;
}

DXResourceTransitionBarrier::DXResourceTransitionBarrier(DXResource* pResource, D3D12_RESOURCE_STATES stateBefore, D3D12_RESOURCE_STATES stateAfter, UINT subresource, D3D12_RESOURCE_BARRIER_FLAGS flags)
{
	Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	Flags = flags;
	Transition.pResource = pResource->GetDXObject();
	Transition.StateBefore = stateBefore;
	Transition.StateAfter = stateAfter;
	Transition.Subresource = subresource;
}

DXVertexBufferView::DXVertexBufferView(D3D12_GPU_VIRTUAL_ADDRESS bufferLocation, UINT sizeInBytes, UINT strideInBytes)
{
	BufferLocation = bufferLocation;
	SizeInBytes = sizeInBytes;
	StrideInBytes = strideInBytes;
}

DXIndexBufferView::DXIndexBufferView(D3D12_GPU_VIRTUAL_ADDRESS bufferLocation, UINT sizeInBytes, UINT strideInBytes)
{
	assert((strideInBytes == sizeof(u16)) || (strideInBytes == sizeof(u32)));
	
	BufferLocation = bufferLocation;
	SizeInBytes = sizeInBytes;
	Format = (strideInBytes == sizeof(u16)) ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;
}

DXConstantBufferViewDesc::DXConstantBufferViewDesc(D3D12_GPU_VIRTUAL_ADDRESS bufferLocation, UINT sizeInBytes)
{
	BufferLocation = bufferLocation;
	SizeInBytes = sizeInBytes;
}

DXRange::DXRange(SIZE_T begin, SIZE_T end)
{
	Begin = begin;
	End = end;
}

DXConstantBufferDesc::DXConstantBufferDesc(UINT64 sizeInBytes, UINT64 alignment)
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

DXVertexBufferDesc::DXVertexBufferDesc(UINT numVertices, UINT strideInBytes, UINT64 alignment)
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

DXIndexBufferDesc::DXIndexBufferDesc(UINT numIndices, UINT strideInBytes, UINT64 alignment)
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

DXStructuredBufferDesc::DXStructuredBufferDesc(UINT numElements, UINT structureByteStride, bool createSRV, bool createUAV, UINT64 alignment)
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

DXStructuredBufferSRVDesc::DXStructuredBufferSRVDesc(UINT64 firstElement, UINT numElements,
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

DXStructuredBufferUAVDesc::DXStructuredBufferUAVDesc(UINT64 firstElement, UINT numElements, UINT structureByteStride)
{
	Format = DXGI_FORMAT_UNKNOWN;
	ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
	Buffer.FirstElement = firstElement;
	Buffer.NumElements = numElements;
	Buffer.StructureByteStride = structureByteStride;
	Buffer.CounterOffsetInBytes = 0;
	Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;
}

DXRawBufferShaderResourceViewDesc::DXRawBufferShaderResourceViewDesc(UINT64 firstElement, UINT numElements, UINT shader4ComponentMapping)
{
	Format = DXGI_FORMAT_R32_TYPELESS;
	ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	Shader4ComponentMapping = shader4ComponentMapping;
	Buffer.FirstElement = firstElement;
	Buffer.NumElements = numElements;
	Buffer.StructureByteStride = 0;
	Buffer.Flags = D3D12_BUFFER_SRV_FLAG_RAW;
}

DXBufferUnorderedAccessViewDesc::DXBufferUnorderedAccessViewDesc(UINT64 firstElement, UINT numElements,
	UINT structureByteStride, DXGI_FORMAT format)
{
	Format = format;
	ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
	Buffer.FirstElement = firstElement;
	Buffer.NumElements = numElements;
	Buffer.StructureByteStride = structureByteStride;
	Buffer.CounterOffsetInBytes = 0;
	Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;
}

DXCounterBufferUnorderedAccessViewDesc::DXCounterBufferUnorderedAccessViewDesc(UINT64 firstElement, UINT numElements,
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

DXRawBufferUnorderedAccessViewDesc::DXRawBufferUnorderedAccessViewDesc(UINT64 firstElement, UINT numElements)
{
	Format = DXGI_FORMAT_R32_TYPELESS;
	ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
	Buffer.FirstElement = firstElement;
	Buffer.NumElements = numElements;
	Buffer.StructureByteStride = 0;
	Buffer.CounterOffsetInBytes = 0;
	Buffer.Flags = D3D12_BUFFER_UAV_FLAG_RAW;
}

DXColorTexture1DDesc::DXColorTexture1DDesc(DXGI_FORMAT format, UINT64 width, bool createRTV, bool createSRV, bool createUAV,
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

DXColorTexture2DDesc::DXColorTexture2DDesc(DXGI_FORMAT format, UINT64 width, UINT height, bool createRTV, bool createSRV, bool createUAV,
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

DXColorTexture3DDesc::DXColorTexture3DDesc(DXGI_FORMAT format, UINT64 width, UINT height, UINT16 depth,
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

DXDepthTexture1DDesc::DXDepthTexture1DDesc(DXGI_FORMAT format, UINT64 width, bool createDSV, bool createSRV,
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

DXDepthTexture2DDesc::DXDepthTexture2DDesc(DXGI_FORMAT format, UINT64 width, UINT height, bool createDSV, bool createSRV,
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

DXDepthTexture3DDesc::DXDepthTexture3DDesc(DXGI_FORMAT format, UINT64 width, UINT height, UINT16 depth,
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

DXTex1DRenderTargetViewDesc::DXTex1DRenderTargetViewDesc(UINT mipSlice, UINT arraySize, DXGI_FORMAT format)
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

DXTex2DRenderTargetViewDesc::DXTex2DRenderTargetViewDesc(UINT mipSlice, UINT arraySize, bool multisampled, DXGI_FORMAT format, UINT planeSlice)
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

DXTex3DRenderTargetViewDesc::DXTex3DRenderTargetViewDesc(UINT mipSlice, UINT firstDepthSlice, UINT depthSliceCount, DXGI_FORMAT format)
{
	ViewDimension = D3D12_RTV_DIMENSION_TEXTURE3D;
	Format = format;
	Texture3D.MipSlice = mipSlice;
	Texture3D.FirstWSlice = firstDepthSlice;
	Texture3D.WSize = depthSliceCount;
}

DXTex1DDepthStencilViewDesc::DXTex1DDepthStencilViewDesc(UINT mipSlice, UINT arraySize, DXGI_FORMAT format, D3D12_DSV_FLAGS flags)
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

DXTex2DDepthStencilViewDesc::DXTex2DDepthStencilViewDesc(DXGI_FORMAT format, UINT mipSlice, UINT arraySize, bool multisampled, D3D12_DSV_FLAGS flags)
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

DXTex1DShaderResourceViewDesc::DXTex1DShaderResourceViewDesc(DXGI_FORMAT format, UINT mostDetailedMip, UINT mipLevels, UINT arraySize,
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

DXTex2DShaderResourceViewDesc::DXTex2DShaderResourceViewDesc(DXGI_FORMAT format, UINT mostDetailedMip, UINT mipLevels, UINT arraySize, bool multisampled,
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

DXTex3DShaderResourceViewDesc::DXTex3DShaderResourceViewDesc(DXGI_FORMAT format, UINT mostDetailedMip, UINT mipLevels, FLOAT minLODClamp, UINT shader4ComponentMapping)
{
	Format = format;
	Shader4ComponentMapping = shader4ComponentMapping;
	ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
	Texture3D.MostDetailedMip = mostDetailedMip;
	Texture3D.MipLevels = mipLevels;
	Texture3D.ResourceMinLODClamp = minLODClamp;
}

DXTexCubeShaderResourceViewDesc::DXTexCubeShaderResourceViewDesc(DXGI_FORMAT format, UINT mostDetailedMip, UINT mipLevels, UINT numCubes,
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

DXTex1DUnorderedAccessViewDesc::DXTex1DUnorderedAccessViewDesc(DXGI_FORMAT format, UINT mipSlice)
{
	Format = format;
	ViewDimension = D3D12_UAV_DIMENSION_TEXTURE1D;
	Texture1D.MipSlice = mipSlice;
}

DXTex2DUnorderedAccessViewDesc::DXTex2DUnorderedAccessViewDesc(DXGI_FORMAT format, UINT mipSlice, UINT planeSlice)
{
	Format = format;
	ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
	Texture2D.MipSlice = mipSlice;
	Texture2D.PlaneSlice = planeSlice;
}

DXResource::DXResource(ID3D12Resource* pDXObject, D3D12_RESOURCE_STATES initialState)
	: DXObject<ID3D12Resource>(pDXObject)
	, m_Desc(pDXObject->GetDesc())
	, m_State(initialState)
	, m_ReadState(D3D12_RESOURCE_STATE_COMMON)
	, m_WriteState(D3D12_RESOURCE_STATE_COMMON)
{
}

DXResource::DXResource(const D3D12_RESOURCE_DESC* pDesc, D3D12_RESOURCE_STATES initialState)
	: m_Desc(*pDesc)
	, m_State(initialState)
	, m_ReadState(D3D12_RESOURCE_STATE_COMMON)
	, m_WriteState(D3D12_RESOURCE_STATE_COMMON)
{
}

DXHeapProperties::DXHeapProperties(D3D12_HEAP_TYPE type)
{
	Type = type;
	CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	CreationNodeMask = 1;
	VisibleNodeMask = 1;
}

DXHeapProperties::DXHeapProperties(D3D12_CPU_PAGE_PROPERTY cpuPageProperty, D3D12_MEMORY_POOL memoryPoolPreference)
{
	Type = D3D12_HEAP_TYPE_CUSTOM;
	CPUPageProperty = cpuPageProperty;
	MemoryPoolPreference = memoryPoolPreference;
	CreationNodeMask = 1;
	VisibleNodeMask = 1;
}

DXColorTexture::DXColorTexture(DXRenderEnvironment* pEnv, const D3D12_HEAP_PROPERTIES* pHeapProps,
	const DXColorTexture1DDesc* pTexDesc, D3D12_RESOURCE_STATES initialState,
	const FLOAT optimizedClearColor[4], LPCWSTR pName)
	: DXResource(pTexDesc, initialState)
{
	CreateCommittedResource(pEnv, pHeapProps, pTexDesc, initialState, optimizedClearColor, pName);
	CreateTex1DViews(pEnv, pTexDesc);
	DetermineResourceStates(pTexDesc);
}

DXColorTexture::DXColorTexture(DXRenderEnvironment* pEnv, const D3D12_HEAP_PROPERTIES* pHeapProps,
	const DXColorTexture2DDesc* pTexDesc, D3D12_RESOURCE_STATES initialState,
	const FLOAT optimizedClearColor[4], LPCWSTR pName)
	: DXResource(pTexDesc, initialState)
{
	CreateCommittedResource(pEnv, pHeapProps, pTexDesc, initialState, optimizedClearColor, pName);
	CreateTex2DViews(pEnv, pTexDesc);
	DetermineResourceStates(pTexDesc);
}

DXColorTexture::DXColorTexture(DXRenderEnvironment* pEnv, const D3D12_HEAP_PROPERTIES* pHeapProps,
	const DXColorTexture3DDesc* pTexDesc, D3D12_RESOURCE_STATES initialState,
	const FLOAT optimizedClearColor[4], LPCWSTR pName)
	: DXResource(pTexDesc, initialState)
{
	CreateCommittedResource(pEnv, pHeapProps, pTexDesc, initialState, optimizedClearColor, pName);
	CreateTex3DViews(pEnv, pTexDesc);
	DetermineResourceStates(pTexDesc);
}

DXColorTexture::DXColorTexture(DXRenderEnvironment* pEnv, ID3D12Resource* pDXObject, D3D12_RESOURCE_STATES initialState, LPCWSTR pName)
	: DXResource(pDXObject, initialState)
{
	if (m_Desc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE1D)
		CreateTex1DViews(pEnv, &m_Desc);
	if (m_Desc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE2D)
		CreateTex2DViews(pEnv, &m_Desc);
	if (m_Desc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE3D)
		CreateTex3DViews(pEnv, &m_Desc);

	DetermineResourceStates(&m_Desc);
}

DXDescriptorHandle DXColorTexture::GetRTVHandle()
{
	assert(m_RTVHandle.IsValid());
	return m_RTVHandle;
}

DXDescriptorHandle DXColorTexture::GetSRVHandle()
{
	assert(m_SRVHandle.IsValid());
	return m_SRVHandle;
}

DXDescriptorHandle DXColorTexture::GetUAVHandle()
{
	assert(m_UAVHandle.IsValid());
	return m_UAVHandle;
}

void DXColorTexture::CreateCommittedResource(DXRenderEnvironment* pEnv, const D3D12_HEAP_PROPERTIES* pHeapProps,
	const D3D12_RESOURCE_DESC* pTexDesc, D3D12_RESOURCE_STATES initialState,
	const FLOAT optimizedClearColor[4], LPCWSTR pName)
{
	ID3D12Device* pDXDevice = pEnv->m_pDevice->GetDXObject();

	D3D12_HEAP_FLAGS heapFlags = D3D12_HEAP_FLAG_NONE;
	if ((pTexDesc->Flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET) != 0)
	{
		DXGI_FORMAT rtvFormat = GetRenderTargetViewFormat(pTexDesc->Format);
		DXClearValue optimizedClearValue(rtvFormat, optimizedClearColor);

		DXVerify(pDXDevice->CreateCommittedResource(pHeapProps, heapFlags, pTexDesc,
			initialState, &optimizedClearValue, IID_PPV_ARGS(GetDXObjectAddress())));
	}
	else
	{
		DXVerify(pDXDevice->CreateCommittedResource(pHeapProps, heapFlags, pTexDesc,
			initialState, nullptr, IID_PPV_ARGS(GetDXObjectAddress())));
	}

#ifdef _DEBUG
	SetName(pName);
#endif
}

void DXColorTexture::DetermineResourceStates(const D3D12_RESOURCE_DESC* pTexDesc)
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

void DXColorTexture::CreateTex1DViews(DXRenderEnvironment* pEnv, const D3D12_RESOURCE_DESC* pTexDesc)
{
	ID3D12Device* pDXDevice = pEnv->m_pDevice->GetDXObject();

	// Kolya: Add missing impl
	assert(pTexDesc->DepthOrArraySize == 1);
	assert(pTexDesc->MipLevels == 1);

	if ((pTexDesc->Flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET) != 0)
	{
		assert(pEnv->m_pShaderInvisibleRTVHeap != nullptr);
		m_RTVHandle = pEnv->m_pShaderInvisibleRTVHeap->Allocate();

		DXTex1DRenderTargetViewDesc viewDesc;
		pDXDevice->CreateRenderTargetView(GetDXObject(), &viewDesc, m_RTVHandle);
	}
	if ((pTexDesc->Flags & D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE) == 0)
	{
		assert(pEnv->m_pShaderInvisibleSRVHeap != nullptr);
		m_SRVHandle = pEnv->m_pShaderInvisibleSRVHeap->Allocate();

		DXTex1DShaderResourceViewDesc viewDesc(GetShaderResourceViewFormat(pTexDesc->Format));
		pDXDevice->CreateShaderResourceView(GetDXObject(), &viewDesc, m_SRVHandle);
	}
	if ((pTexDesc->Flags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS) != 0)
	{
		assert(pEnv->m_pShaderInvisibleSRVHeap != nullptr);
		m_UAVHandle = pEnv->m_pShaderInvisibleSRVHeap->Allocate();

		DXTex1DUnorderedAccessViewDesc viewDesc(GetUnorderedAccessViewFormat(pTexDesc->Format));
		pDXDevice->CreateUnorderedAccessView(GetDXObject(), nullptr, &viewDesc, m_UAVHandle);
	}
}

void DXColorTexture::CreateTex2DViews(DXRenderEnvironment* pEnv, const D3D12_RESOURCE_DESC* pTexDesc)
{
	ID3D12Device* pDXDevice = pEnv->m_pDevice->GetDXObject();

	// Kolya: Add missing impl
	assert(pTexDesc->DepthOrArraySize == 1);
	assert(pTexDesc->MipLevels == 1);
	assert(pTexDesc->SampleDesc.Count == 1);
	assert(pTexDesc->SampleDesc.Quality == 0);

	if ((pTexDesc->Flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET) != 0)
	{
		assert(pEnv->m_pShaderInvisibleRTVHeap != nullptr);
		m_RTVHandle = pEnv->m_pShaderInvisibleRTVHeap->Allocate();

		DXTex2DRenderTargetViewDesc viewDesc;
		pDXDevice->CreateRenderTargetView(GetDXObject(), &viewDesc, m_RTVHandle);
	}
	if ((pTexDesc->Flags & D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE) == 0)
	{
		assert(pEnv->m_pShaderInvisibleSRVHeap != nullptr);
		m_SRVHandle = pEnv->m_pShaderInvisibleSRVHeap->Allocate();

		DXTex2DShaderResourceViewDesc viewDesc(GetShaderResourceViewFormat(pTexDesc->Format));
		pDXDevice->CreateShaderResourceView(GetDXObject(), &viewDesc, m_SRVHandle);
	}
	if ((pTexDesc->Flags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS) != 0)
	{
		assert(pEnv->m_pShaderInvisibleSRVHeap != nullptr);
		m_UAVHandle = pEnv->m_pShaderInvisibleSRVHeap->Allocate();

		DXTex2DUnorderedAccessViewDesc viewDesc(GetUnorderedAccessViewFormat(pTexDesc->Format));
		pDXDevice->CreateUnorderedAccessView(GetDXObject(), nullptr, &viewDesc, m_UAVHandle);
	}
}

void DXColorTexture::CreateTex3DViews(DXRenderEnvironment* pEnv, const D3D12_RESOURCE_DESC* pTexDesc)
{
	assert(false && "Kolya: Needs impl");
	if ((pTexDesc->Flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET) != 0)
	{
		assert(pEnv->m_pShaderInvisibleRTVHeap != nullptr);
	}
	if ((pTexDesc->Flags & D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE) == 0)
	{
		assert(pEnv->m_pShaderInvisibleSRVHeap != nullptr);
	}
	if ((pTexDesc->Flags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS) != 0)
	{
		assert(pEnv->m_pShaderInvisibleSRVHeap != nullptr);
	}
}

DXDepthTexture::DXDepthTexture(DXRenderEnvironment* pEnv, const D3D12_HEAP_PROPERTIES* pHeapProps,
	const DXDepthTexture1DDesc* pTexDesc, D3D12_RESOURCE_STATES initialState,
	const DXDepthStencilValue* pOptimizedClearDepth, LPCWSTR pName)
	: DXResource(pTexDesc, initialState)
{
	CreateCommittedResource(pEnv, pHeapProps, pTexDesc, initialState, pOptimizedClearDepth, pName);
	CreateTex1DViews(pEnv, pTexDesc);
	DetermineResourceStates(pTexDesc);
}

DXDepthTexture::DXDepthTexture(DXRenderEnvironment* pEnv, const D3D12_HEAP_PROPERTIES* pHeapProps,
	const DXDepthTexture2DDesc* pTexDesc, D3D12_RESOURCE_STATES initialState,
	const DXDepthStencilValue* pOptimizedClearDepth, LPCWSTR pName)
	: DXResource(pTexDesc, initialState)
{
	CreateCommittedResource(pEnv, pHeapProps, pTexDesc, initialState, pOptimizedClearDepth, pName);
	CreateTex2DViews(pEnv, pTexDesc);
	DetermineResourceStates(pTexDesc);
}

DXDepthTexture::DXDepthTexture(DXRenderEnvironment* pEnv, const D3D12_HEAP_PROPERTIES* pHeapProps,
	const DXDepthTexture3DDesc* pTexDesc, D3D12_RESOURCE_STATES initialState,
	const DXDepthStencilValue* pOptimizedClearDepth, LPCWSTR pName)
	: DXResource(pTexDesc, initialState)
{
	CreateCommittedResource(pEnv, pHeapProps, pTexDesc, initialState, pOptimizedClearDepth, pName);
	CreateTex3DViews(pEnv, pTexDesc);
	DetermineResourceStates(pTexDesc);
}

DXDescriptorHandle DXDepthTexture::GetDSVHandle()
{
	assert(m_DSVHandle.IsValid());
	return m_DSVHandle;
}

DXDescriptorHandle DXDepthTexture::GetSRVHandle()
{
	assert(m_SRVHandle.IsValid());
	return m_SRVHandle;
}

void DXDepthTexture::CreateCommittedResource(DXRenderEnvironment* pEnv, const D3D12_HEAP_PROPERTIES* pHeapProps,
	const D3D12_RESOURCE_DESC* pTexDesc, D3D12_RESOURCE_STATES initialState,
	const DXDepthStencilValue* pOptimizedClearDepth, LPCWSTR pName)
{
	ID3D12Device* pDXDevice = pEnv->m_pDevice->GetDXObject();
	
	D3D12_HEAP_FLAGS heapFlags = D3D12_HEAP_FLAG_NONE;
	DXGI_FORMAT dsvFormat = GetDepthStencilViewFormat(pTexDesc->Format);
	DXClearValue optimizedClearValue(dsvFormat, pOptimizedClearDepth);

	DXVerify(pDXDevice->CreateCommittedResource(pHeapProps, heapFlags, pTexDesc,
		initialState, &optimizedClearValue, IID_PPV_ARGS(GetDXObjectAddress())));

#ifdef _DEBUG
	SetName(pName);
#endif
}

void DXDepthTexture::CreateTex1DViews(DXRenderEnvironment* pEnv, const D3D12_RESOURCE_DESC* pTexDesc)
{
	assert(false && "Kolya: Needs impl");
	if ((pTexDesc->Flags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL) != 0)
	{
		assert(pEnv->m_pShaderInvisibleDSVHeap != nullptr);
	}
	if ((pTexDesc->Flags & D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE) == 0)
	{
		assert(pEnv->m_pShaderInvisibleSRVHeap != nullptr);
	}
}

void DXDepthTexture::CreateTex2DViews(DXRenderEnvironment* pEnv, const D3D12_RESOURCE_DESC* pTexDesc)
{
	ID3D12Device* pDXDevice = pEnv->m_pDevice->GetDXObject();

	// Kolya: Add missing impl
	assert(pTexDesc->DepthOrArraySize == 1);
	assert(pTexDesc->MipLevels == 1);
	assert(pTexDesc->SampleDesc.Count == 1);
	assert(pTexDesc->SampleDesc.Quality == 0);

	if ((pTexDesc->Flags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL) != 0)
	{
		assert(pEnv->m_pShaderInvisibleDSVHeap != nullptr);
		m_DSVHandle = pEnv->m_pShaderInvisibleDSVHeap->Allocate();

		DXTex2DDepthStencilViewDesc viewDesc(GetDepthStencilViewFormat(pTexDesc->Format));
		pDXDevice->CreateDepthStencilView(GetDXObject(), &viewDesc, m_DSVHandle);
	}
	if ((pTexDesc->Flags & D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE) == 0)
	{
		assert(pEnv->m_pShaderInvisibleSRVHeap != nullptr);
		m_SRVHandle = pEnv->m_pShaderInvisibleSRVHeap->Allocate();

		DXTex2DShaderResourceViewDesc viewDesc(GetShaderResourceViewFormat(pTexDesc->Format));
		pDXDevice->CreateShaderResourceView(GetDXObject(), &viewDesc, m_SRVHandle);
	}
}

void DXDepthTexture::CreateTex3DViews(DXRenderEnvironment* pEnv, const D3D12_RESOURCE_DESC* pTexDesc)
{
	assert(false && "Kolya: Needs impl");
	if ((pTexDesc->Flags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL) != 0)
	{
		assert(pEnv->m_pShaderInvisibleDSVHeap != nullptr);
	}
	if ((pTexDesc->Flags & D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE) == 0)
	{
		assert(pEnv->m_pShaderInvisibleSRVHeap != nullptr);
	}
}

void DXDepthTexture::DetermineResourceStates(const D3D12_RESOURCE_DESC* pTexDesc)
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

DXBuffer::DXBuffer(DXRenderEnvironment* pEnv, const D3D12_HEAP_PROPERTIES* pHeapProps,
	const DXConstantBufferDesc* pBufferDesc, D3D12_RESOURCE_STATES initialState, LPCWSTR pName)
	: DXResource(pBufferDesc, initialState)
{
	CreateCommittedResource(pEnv, pHeapProps, pBufferDesc, initialState, pName);
	CreateConstantBufferView(pEnv, pBufferDesc);

	m_WriteState = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
	m_ReadState = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
}

DXBuffer::DXBuffer(DXRenderEnvironment* pEnv, const D3D12_HEAP_PROPERTIES* pHeapProps,
	const DXVertexBufferDesc* pBufferDesc, D3D12_RESOURCE_STATES initialState, LPCWSTR pName)
	: DXResource(pBufferDesc, initialState)
{
	CreateCommittedResource(pEnv, pHeapProps, pBufferDesc, initialState, pName);
	CreateVertexBufferView(pEnv, pBufferDesc);

	m_WriteState = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
	m_ReadState = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
}

DXBuffer::DXBuffer(DXRenderEnvironment* pEnv, const D3D12_HEAP_PROPERTIES* pHeapProps,
	const DXIndexBufferDesc* pBufferDesc, D3D12_RESOURCE_STATES initialState, LPCWSTR pName)
	: DXResource(pBufferDesc, initialState)
{
	CreateCommittedResource(pEnv, pHeapProps, pBufferDesc, initialState, pName);
	CreateIndexBufferView(pEnv, pBufferDesc);

	m_WriteState = D3D12_RESOURCE_STATE_INDEX_BUFFER;
	m_ReadState = D3D12_RESOURCE_STATE_INDEX_BUFFER;
}

DXBuffer::DXBuffer(DXRenderEnvironment* pEnv, const D3D12_HEAP_PROPERTIES* pHeapProps,
	const DXStructuredBufferDesc* pBufferDesc, D3D12_RESOURCE_STATES initialState, LPCWSTR pName)
	: DXResource(pBufferDesc, initialState)
{
	CreateCommittedResource(pEnv, pHeapProps, pBufferDesc, initialState, pName);
	CreateStructuredBufferViews(pEnv, pBufferDesc);

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

DXBuffer::~DXBuffer()
{
	SafeDelete(m_pVBView);
	SafeDelete(m_pIBView);
}

DXDescriptorHandle DXBuffer::GetSRVHandle()
{
	assert(m_SRVHandle.IsValid());
	return m_SRVHandle;
}

DXDescriptorHandle DXBuffer::GetUAVHandle()
{
	assert(m_UAVHandle.IsValid());
	return m_UAVHandle;
}

DXDescriptorHandle DXBuffer::GetCBVHandle()
{
	assert(m_CBVHandle.IsValid());
	return m_CBVHandle;
}

DXVertexBufferView* DXBuffer::GetVBView()
{
	assert(m_pVBView != nullptr);
	return m_pVBView;
}

DXIndexBufferView* DXBuffer::GetIBView()
{
	assert(m_pIBView != nullptr);
	return m_pIBView;
}

void DXBuffer::CreateCommittedResource(DXRenderEnvironment* pEnv, const D3D12_HEAP_PROPERTIES* pHeapProps,
	const D3D12_RESOURCE_DESC* pBufferDesc, D3D12_RESOURCE_STATES initialState, LPCWSTR pName)
{
	ID3D12Device* pDXDevice = pEnv->m_pDevice->GetDXObject();

	D3D12_HEAP_FLAGS heapFlags = D3D12_HEAP_FLAG_NONE;
	const D3D12_CLEAR_VALUE* pOptimizedClearValue = nullptr;

	DXVerify(pDXDevice->CreateCommittedResource(pHeapProps, heapFlags, pBufferDesc,
		initialState, pOptimizedClearValue, IID_PPV_ARGS(GetDXObjectAddress())));

	m_pVBView = nullptr;
	m_pIBView = nullptr;

#ifdef _DEBUG
	SetName(pName);
#endif
}

void DXBuffer::CreateConstantBufferView(DXRenderEnvironment* pEnv, const DXConstantBufferDesc* pBufferDesc)
{
	ID3D12Device* pDXDevice = pEnv->m_pDevice->GetDXObject();
	m_CBVHandle = pEnv->m_pShaderInvisibleSRVHeap->Allocate();
	
	DXConstantBufferViewDesc viewDesc(GetDXObject()->GetGPUVirtualAddress(), (UINT)pBufferDesc->Width);
	pDXDevice->CreateConstantBufferView(&viewDesc, m_CBVHandle);
}

void DXBuffer::CreateVertexBufferView(DXRenderEnvironment* pEnv, const DXVertexBufferDesc* pBufferDesc)
{
	ID3D12Device* pDXDevice = pEnv->m_pDevice->GetDXObject();
	D3D12_GPU_VIRTUAL_ADDRESS gpuVirtualAddress = GetDXObject()->GetGPUVirtualAddress();

	m_pVBView = new DXVertexBufferView(gpuVirtualAddress, (UINT)pBufferDesc->Width, pBufferDesc->StrideInBytes);
}

void DXBuffer::CreateIndexBufferView(DXRenderEnvironment* pEnv, const DXIndexBufferDesc* pBufferDesc)
{
	ID3D12Device* pDXDevice = pEnv->m_pDevice->GetDXObject();
	D3D12_GPU_VIRTUAL_ADDRESS gpuVirtualAddress = GetDXObject()->GetGPUVirtualAddress();

	m_pIBView = new DXIndexBufferView(gpuVirtualAddress, (UINT)pBufferDesc->Width, pBufferDesc->StrideInBytes);
}

void DXBuffer::CreateStructuredBufferViews(DXRenderEnvironment* pEnv, const DXStructuredBufferDesc* pBufferDesc)
{
	ID3D12Device* pDXDevice = pEnv->m_pDevice->GetDXObject();

	if ((pBufferDesc->Flags & D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE) == 0)
	{
		assert(pEnv->m_pShaderInvisibleSRVHeap != nullptr);
		m_SRVHandle = pEnv->m_pShaderInvisibleSRVHeap->Allocate();

		DXStructuredBufferSRVDesc viewDesc(0, pBufferDesc->NumElements, pBufferDesc->StructureByteStride);
		pDXDevice->CreateShaderResourceView(GetDXObject(), &viewDesc, m_SRVHandle);
	}
	if ((pBufferDesc->Flags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS) != 0)
	{
		assert(pEnv->m_pShaderInvisibleSRVHeap != nullptr);
		m_UAVHandle = pEnv->m_pShaderInvisibleSRVHeap->Allocate();

		DXStructuredBufferUAVDesc viewDesc(0, pBufferDesc->NumElements, pBufferDesc->StructureByteStride);
		pDXDevice->CreateUnorderedAccessView(GetDXObject(), nullptr, &viewDesc, m_UAVHandle);
	}
}

DXSampler::DXSampler(DXRenderEnvironment* pEnv, const D3D12_SAMPLER_DESC* pDesc)
{
	assert(pEnv->m_pShaderInvisibleSamplerHeap != nullptr);
	m_Handle = pEnv->m_pShaderInvisibleSamplerHeap->Allocate();

	ID3D12Device* pDXDevice = pEnv->m_pDevice->GetDXObject();
	pDXDevice->CreateSampler(pDesc, m_Handle);
}
