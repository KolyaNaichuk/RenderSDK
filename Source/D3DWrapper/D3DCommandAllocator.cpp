#include "D3DWrapper/D3DDevice.h"
#include "D3DWrapper/D3DCommandAllocator.h"

D3DCommandAllocator::D3DCommandAllocator(D3DDevice* pDevice, D3D12_COMMAND_LIST_TYPE type, LPCWSTR pName)
	: m_Type(type)
	, m_pSyncPoint(nullptr)
{
	DXVerify(pDevice->GetDXObject()->CreateCommandAllocator(type, IID_PPV_ARGS(GetDXObjectAddress())));

#ifdef _DEBUG
	SetName(pName);
#endif
}

D3DCommandAllocator::~D3DCommandAllocator()
{
	assert((m_pSyncPoint == nullptr) || m_pSyncPoint->IsComplete());
	SafeDelete(m_pSyncPoint);
}

void D3DCommandAllocator::Reset()
{
	DXVerify(GetDXObject()->Reset());
}

D3D12_COMMAND_LIST_TYPE D3DCommandAllocator::GetType() const
{
	return m_Type;
}

D3DSyncPoint* D3DCommandAllocator::GetSyncPoint()
{
	return m_pSyncPoint;
}

void D3DCommandAllocator::SetSyncPoint(D3DSyncPoint* pSyncPoint)
{
	if (m_pSyncPoint != pSyncPoint)
	{
		SafeDelete(m_pSyncPoint);
		m_pSyncPoint = pSyncPoint;
	}
}

D3DCommandAllocatorPool::D3DCommandAllocatorPool(D3DDevice* pDevice, D3D12_COMMAND_LIST_TYPE type)
	: m_pDevice(pDevice)
	, m_Type(type)
{
}

D3DCommandAllocatorPool::~D3DCommandAllocatorPool()
{
	while (!m_CommandAllocatorQueue.empty())
	{
		D3DCommandAllocator* pCommandAllocator = m_CommandAllocatorQueue.front();
		
		D3DSyncPoint* pSyncPoint = pCommandAllocator->GetSyncPoint();
		if (pSyncPoint != nullptr)
			pSyncPoint->WaitForCompletion();

		m_CommandAllocatorQueue.pop();
		SafeDelete(pCommandAllocator);
	}
}

D3DCommandAllocator* D3DCommandAllocatorPool::Create(LPCWSTR pName)
{
	if (!m_CommandAllocatorQueue.empty())
	{
		D3DCommandAllocator* pCommandAllocator = m_CommandAllocatorQueue.front();
		
		D3DSyncPoint* pSyncPoint = pCommandAllocator->GetSyncPoint();
		if ((pSyncPoint == nullptr) || pSyncPoint->IsComplete())
		{
			m_CommandAllocatorQueue.pop();

			pCommandAllocator->Reset();
			pCommandAllocator->SetName(pName);

			return pCommandAllocator;
		}
	}
	return new D3DCommandAllocator(m_pDevice, m_Type, pName);
}

void D3DCommandAllocatorPool::Release(D3DCommandAllocator* pCommandAllocator)
{
	m_CommandAllocatorQueue.push(pCommandAllocator);
}
