#pragma once

#include "D3DWrapper/D3DDescriptorHeap.h"

class D3DDevice;
class D3DCommandListPool;
class D3DCommandAllocatorPool;
struct D3DHeapProperties;

struct D3DRenderEnv
{
	D3DRenderEnv()
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

	D3DDevice* m_pDevice;

	D3DCommandListPool* m_pCommandListPool;
	D3DCommandAllocatorPool* m_pCommandAllocatorPool;

	D3DHeapProperties* m_pUploadHeapProps;
	D3DHeapProperties* m_pDefaultHeapProps;

	D3DDescriptorHeap* m_pShaderInvisibleRTVHeap;
	D3DDescriptorHeap* m_pShaderInvisibleDSVHeap;
	D3DDescriptorHeap* m_pShaderInvisibleSRVHeap;
	D3DDescriptorHeap* m_pShaderInvisibleSamplerHeap;

	D3DDescriptorHeap* m_pShaderVisibleSRVHeap;
	D3DDescriptorHeap* m_pShaderVisibleSamplerHeap;
};
