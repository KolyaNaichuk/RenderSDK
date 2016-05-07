#pragma once

#include "DX/DXDescriptorHeap.h"

class DXResource;

struct ResourceTransitionList
{
	ResourceTransitionList(u32 numResources);
	~ResourceTransitionList();

	u32 m_NumResources;
	DXResource** m_ppResources;
	D3D12_RESOURCE_STATES* m_pExpectedStates;
};

struct BindingResourceList
{
	BindingResourceList(u32 numResourceTransitions);

	ResourceTransitionList m_TransitionList;
	DXDescriptorHandle m_RTVHeapStart;
	DXDescriptorHandle m_DSVHeapStart;
	DXDescriptorHandle m_SRVHeapStart;
	DXDescriptorHandle m_SamplerHeapStart;
};