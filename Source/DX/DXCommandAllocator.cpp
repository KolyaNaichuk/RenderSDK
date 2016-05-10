#include "DX/DXDevice.h"
#include "DX/DXCommandAllocator.h"

DXCommandAllocator::DXCommandAllocator(DXDevice* pDevice, D3D12_COMMAND_LIST_TYPE type, LPCWSTR pName)
	: m_Type(type)
	, m_pSyncPoint(nullptr)
{
	DXVerify(pDevice->GetDXObject()->CreateCommandAllocator(type, IID_PPV_ARGS(GetDXObjectAddress())));

#ifdef _DEBUG
	SetName(pName);
#endif
}

DXCommandAllocator::~DXCommandAllocator()
{
	assert((m_pSyncPoint == nullptr) || m_pSyncPoint->IsComplete());
	SafeDelete(m_pSyncPoint);
}

void DXCommandAllocator::Reset()
{
	DXVerify(GetDXObject()->Reset());
}

D3D12_COMMAND_LIST_TYPE DXCommandAllocator::GetType() const
{
	return m_Type;
}

DXSyncPoint* DXCommandAllocator::GetSyncPoint()
{
	return m_pSyncPoint;
}

void DXCommandAllocator::SetSyncPoint(DXSyncPoint* pSyncPoint)
{
	if (m_pSyncPoint != pSyncPoint)
	{
		SafeDelete(m_pSyncPoint);
		m_pSyncPoint = pSyncPoint;
	}
}

DXCommandAllocatorPool::DXCommandAllocatorPool(DXDevice* pDevice, D3D12_COMMAND_LIST_TYPE type)
	: m_pDevice(pDevice)
	, m_Type(type)
{
}

DXCommandAllocatorPool::~DXCommandAllocatorPool()
{
	while (!m_CommandAllocatorQueue.empty())
	{
		DXCommandAllocator* pCommandAllocator = m_CommandAllocatorQueue.front();
		
		DXSyncPoint* pSyncPoint = pCommandAllocator->GetSyncPoint();
		if (pSyncPoint != nullptr)
			pSyncPoint->WaitForCompletion();

		m_CommandAllocatorQueue.pop();
		SafeDelete(pCommandAllocator);
	}
}

DXCommandAllocator* DXCommandAllocatorPool::Create(LPCWSTR pName)
{
	if (!m_CommandAllocatorQueue.empty())
	{
		DXCommandAllocator* pCommandAllocator = m_CommandAllocatorQueue.front();
		
		DXSyncPoint* pSyncPoint = pCommandAllocator->GetSyncPoint();
		if ((pSyncPoint == nullptr) || pSyncPoint->IsComplete())
		{
			m_CommandAllocatorQueue.pop();

			pCommandAllocator->Reset();
			pCommandAllocator->SetName(pName);

			return pCommandAllocator;
		}
	}
	return new DXCommandAllocator(m_pDevice, m_Type, pName);
}

void DXCommandAllocatorPool::Release(DXCommandAllocator* pCommandAllocator)
{
	m_CommandAllocatorQueue.push(pCommandAllocator);
}
