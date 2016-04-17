#pragma once

class DXDevice;
class DXDescriptorHeap;

struct DXRenderEnvironment
{
	DXRenderEnvironment()
		: m_pDevice(nullptr)
		, m_pRTVDescriptorHeap(nullptr)
		, m_pDSVDescritoprHeap(nullptr)
		, m_pSRVDescriptorHeap(nullptr)
		, m_pUAVDescriptorHeap(nullptr)
		, m_pSamplerDescriptorHeap(nullptr)
	{}

	DXDevice* m_pDevice;
	DXDescriptorHeap* m_pRTVDescriptorHeap;
	DXDescriptorHeap* m_pDSVDescritoprHeap;
	DXDescriptorHeap* m_pSRVDescriptorHeap;
	DXDescriptorHeap* m_pUAVDescriptorHeap;
	DXDescriptorHeap* m_pSamplerDescriptorHeap;
};