#pragma once

#include "D3DWrapper/D3DDescriptorHeap.h"

class D3DResource;

struct D3DRequiredResourceState
{
	D3DRequiredResourceState(D3DResource* pResource, D3D12_RESOURCE_STATES requiredState)
		: m_pResource(pResource)
		, m_RequiredState(requiredState)
	{}
	D3DResource* m_pResource;
	D3D12_RESOURCE_STATES m_RequiredState;
};

typedef std::vector<D3DRequiredResourceState> D3DRequiredResourceStateList;

struct D3DResourceList
{
	D3DRequiredResourceStateList m_RequiredResourceStates;
	D3DDescriptorHandle m_RTVHeapStart;
	D3DDescriptorHandle m_DSVHeapStart;
	D3DDescriptorHandle m_SRVHeapStart;
	D3DDescriptorHandle m_SamplerHeapStart;
};