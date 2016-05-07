#include "DX/DXResourceList.h"

ResourceTransitionList::ResourceTransitionList(u32 numResources)
	: m_NumResources(numResources)
	, m_ppResources(new DXResource* [numResources])
	, m_pExpectedStates(new D3D12_RESOURCE_STATES[numResources])
{
}

ResourceTransitionList::~ResourceTransitionList()
{
	SafeArrayDelete(m_ppResources);
	SafeArrayDelete(m_pExpectedStates);
}

BindingResourceList::BindingResourceList(u32 numResourceTransitions)
	: m_TransitionList(numResourceTransitions)
{
}
