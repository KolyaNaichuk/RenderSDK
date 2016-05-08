#pragma once

#include "DX/DXDescriptorHeap.h"

class DXResource;

struct DXResourceTransition
{
	DXResourceTransition(DXResource* pResource, D3D12_RESOURCE_STATES expectedState);

	DXResource* m_pResource;
	D3D12_RESOURCE_STATES m_ExpectedState;
};

typedef std::vector<DXResourceTransition> DXResourceTransitionList;

struct DXBindingResourceList
{
	DXResourceTransitionList m_ResourceTransitions;
	DXDescriptorHandle m_RTVHeapStart;
	DXDescriptorHandle m_DSVHeapStart;
	DXDescriptorHandle m_SRVHeapStart;
	DXDescriptorHandle m_SamplerHeapStart;
};