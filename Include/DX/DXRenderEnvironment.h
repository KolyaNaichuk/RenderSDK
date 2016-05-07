#pragma once

class DXDevice;
class DXDescriptorHeap;
struct DXHeapProperties;

struct DXRenderEnvironment
{
	DXRenderEnvironment()
		: m_pDevice(nullptr)
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

	DXHeapProperties* m_pUploadHeapProps;
	DXHeapProperties* m_pDefaultHeapProps;

	DXDescriptorHeap* m_pShaderInvisibleRTVHeap;
	DXDescriptorHeap* m_pShaderInvisibleDSVHeap;
	DXDescriptorHeap* m_pShaderInvisibleSRVHeap;
	DXDescriptorHeap* m_pShaderInvisibleSamplerHeap;

	DXDescriptorHeap* m_pShaderVisibleSRVHeap;
	DXDescriptorHeap* m_pShaderVisibleSamplerHeap;
};
