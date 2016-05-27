#pragma once

#include "DX/DXDescriptorHeap.h"

class DXDevice;
class DXCommandListPool;
class DXCommandAllocatorPool;
struct DXHeapProperties;

struct DXRenderEnvironment
{
	DXRenderEnvironment()
		: m_pDevice(nullptr)
		, m_pCommandListPool(nullptr)
		, m_pCommandAllocatorPool(nullptr)
		, m_pUploadHeapProps(nullptr)
		, m_pDefaultHeapProps(nullptr)
		, m_pShaderInvisibleRTVHeap(nullptr)
		, m_pShaderInvisibleDSVHeap(nullptr)
		, m_pShaderInvisibleSRVHeap(nullptr)
		, m_pShaderInvisibleSamplerHeap(nullptr)
		, m_pShaderVisibleSRVHeap(nullptr)
		, m_pShaderVisibleSamplerHeap(nullptr)
	{}

	DXDevice* m_pDevice;

	DXCommandListPool* m_pCommandListPool;
	DXCommandAllocatorPool* m_pCommandAllocatorPool;

	DXHeapProperties* m_pUploadHeapProps;
	DXHeapProperties* m_pDefaultHeapProps;

	DXDescriptorHeap* m_pShaderInvisibleRTVHeap;
	DXDescriptorHeap* m_pShaderInvisibleDSVHeap;
	DXDescriptorHeap* m_pShaderInvisibleSRVHeap;
	DXDescriptorHeap* m_pShaderInvisibleSamplerHeap;

	DXDescriptorHeap* m_pShaderVisibleSRVHeap;
	DXDescriptorHeap* m_pShaderVisibleSamplerHeap;
};
