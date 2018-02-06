#pragma once

#include "D3DWrapper/Common.h"

class CommandListPool;
class CommandQueue;
class GraphicsDevice;
class DescriptorHeap;
class GPUProfiler;
class Fence;

struct HeapProperties;

struct RenderEnv
{
	GraphicsDevice* m_pDevice = nullptr;
	CommandQueue* m_pCommandQueue = nullptr;
	CommandListPool* m_pCommandListPool = nullptr;
	GPUProfiler* m_pGPUProfiler = nullptr;
	Fence* m_pFence = nullptr;
	UINT64 m_LastSubmissionFenceValue = 0;

	HeapProperties* m_pUploadHeapProps = nullptr;
	HeapProperties* m_pDefaultHeapProps = nullptr;
	HeapProperties* m_pReadbackHeapProps = nullptr;

	DescriptorHeap* m_pShaderInvisibleRTVHeap = nullptr;
	DescriptorHeap* m_pShaderInvisibleDSVHeap = nullptr;
	DescriptorHeap* m_pShaderInvisibleSRVHeap = nullptr;
	DescriptorHeap* m_pShaderInvisibleSamplerHeap = nullptr;

	DescriptorHeap* m_pShaderVisibleSRVHeap = nullptr;
	DescriptorHeap* m_pShaderVisibleSamplerHeap = nullptr;
};
