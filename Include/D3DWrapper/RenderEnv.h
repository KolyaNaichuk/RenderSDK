#pragma once

#include "D3DWrapper/DescriptorHeap.h"

class GraphicsDevice;
class CommandListPool;
class CommandQueue;
class Fence;

struct HeapProperties;

struct RenderEnv
{
	RenderEnv()
		: m_pDevice(nullptr)
		, m_pCommandQueue(nullptr)
		, m_pCommandListPool(nullptr)
		, m_pFence(nullptr)
		, m_LastSubmissionFenceValue(0)
		, m_pUploadHeapProps(nullptr)
		, m_pDefaultHeapProps(nullptr)
		, m_pReadbackHeapProps(nullptr)
		, m_pShaderInvisibleRTVHeap(nullptr)
		, m_pShaderInvisibleDSVHeap(nullptr)
		, m_pShaderInvisibleSRVHeap(nullptr)
		, m_pShaderInvisibleSamplerHeap(nullptr)
		, m_pShaderVisibleSRVHeap(nullptr)
		, m_pShaderVisibleSamplerHeap(nullptr)
	{}

	GraphicsDevice* m_pDevice;
	CommandQueue* m_pCommandQueue;
	CommandListPool* m_pCommandListPool;
	Fence* m_pFence;
	UINT64 m_LastSubmissionFenceValue;

	HeapProperties* m_pUploadHeapProps;
	HeapProperties* m_pDefaultHeapProps;
	HeapProperties* m_pReadbackHeapProps;

	DescriptorHeap* m_pShaderInvisibleRTVHeap;
	DescriptorHeap* m_pShaderInvisibleDSVHeap;
	DescriptorHeap* m_pShaderInvisibleSRVHeap;
	DescriptorHeap* m_pShaderInvisibleSamplerHeap;

	DescriptorHeap* m_pShaderVisibleSRVHeap;
	DescriptorHeap* m_pShaderVisibleSamplerHeap;
};
