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
		, m_pRTVDescriptorHeap(nullptr)
		, m_pDSVDescritoprHeap(nullptr)
		, m_pSRVDescriptorHeap(nullptr)
		, m_pUAVDescriptorHeap(nullptr)
		, m_pSamplerDescriptorHeap(nullptr)
	{}

	DXDevice* m_pDevice;
	DXHeapProperties* m_pUploadHeapProps;
	DXHeapProperties* m_pDefaultHeapProps;
	DXDescriptorHeap* m_pRTVDescriptorHeap;
	DXDescriptorHeap* m_pDSVDescritoprHeap;
	DXDescriptorHeap* m_pSRVDescriptorHeap;
	DXDescriptorHeap* m_pUAVDescriptorHeap;
	DXDescriptorHeap* m_pSamplerDescriptorHeap;
};