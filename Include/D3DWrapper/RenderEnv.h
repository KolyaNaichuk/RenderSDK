#pragma once

#include "D3DWrapper/DescriptorHeap.h"

class GraphicsDevice;
class CommandListPool;
struct HeapProperties;

struct RenderEnv
{
	RenderEnv()
		: m_pDevice(nullptr)
		, m_pCommandListPool(nullptr)
		, m_pUploadHeapProps(nullptr)
		, m_pDefaultHeapProps(nullptr)
		, m_pShaderInvisibleRTVHeap(nullptr)
		, m_pShaderInvisibleDSVHeap(nullptr)
		, m_pShaderInvisibleSRVHeap(nullptr)
		, m_pShaderInvisibleSamplerHeap(nullptr)
		, m_pShaderVisibleSRVHeap(nullptr)
		, m_pShaderVisibleSamplerHeap(nullptr)
	{}

	GraphicsDevice* m_pDevice;
	CommandListPool* m_pCommandListPool;

	HeapProperties* m_pUploadHeapProps;
	HeapProperties* m_pDefaultHeapProps;

	DescriptorHeap* m_pShaderInvisibleRTVHeap;
	DescriptorHeap* m_pShaderInvisibleDSVHeap;
	DescriptorHeap* m_pShaderInvisibleSRVHeap;
	DescriptorHeap* m_pShaderInvisibleSamplerHeap;

	DescriptorHeap* m_pShaderVisibleSRVHeap;
	DescriptorHeap* m_pShaderVisibleSamplerHeap;
};
