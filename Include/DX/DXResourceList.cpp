#include "DX/DXResourceList.h"

DXResourceTransition::DXResourceTransition(DXResource* pResource, D3D12_RESOURCE_STATES expectedState)
	: m_pResource(pResource)
	, m_ExpectedState(expectedState)
{
}
