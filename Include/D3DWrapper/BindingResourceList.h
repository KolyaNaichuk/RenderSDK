#pragma once

#include "D3DWrapper/DescriptorHeap.h"

class GraphicsResource;

struct RequiredResourceState
{
	RequiredResourceState(GraphicsResource* pResource, D3D12_RESOURCE_STATES requiredState)
		: m_pResource(pResource)
		, m_RequiredState(requiredState)
	{}
	GraphicsResource* m_pResource;
	D3D12_RESOURCE_STATES m_RequiredState;
};

typedef std::vector<RequiredResourceState> RequiredResourceStateList;

struct BindingResourceList
{
	RequiredResourceStateList m_RequiredResourceStates;
	DescriptorHandle m_RTVHeapStart;
	DescriptorHandle m_DSVHeapStart;
	DescriptorHandle m_SRVHeapStart;
	DescriptorHandle m_SamplerHeapStart;
};