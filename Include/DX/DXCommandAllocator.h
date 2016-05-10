#pragma once

#include "DX/DXFence.h"

class DXDevice;

class DXCommandAllocator : public DXObject<ID3D12CommandAllocator>
{
public:
	DXCommandAllocator(DXDevice* pDevice, D3D12_COMMAND_LIST_TYPE type, LPCWSTR pName);
	~DXCommandAllocator();

	void Reset();
	D3D12_COMMAND_LIST_TYPE GetType() const;
		
	DXSyncPoint* GetSyncPoint();
	void SetSyncPoint(DXSyncPoint* pSyncPoint);

private:
	const D3D12_COMMAND_LIST_TYPE m_Type;
	DXSyncPoint* m_pSyncPoint;
};

class DXCommandAllocatorPool
{
public:
	DXCommandAllocatorPool(DXDevice* pDevice, D3D12_COMMAND_LIST_TYPE type);
	~DXCommandAllocatorPool();

	DXCommandAllocator* Create(LPCWSTR pName);
	void Release(DXCommandAllocator* pCommandAllocator);

private:
	DXDevice* m_pDevice;
	const D3D12_COMMAND_LIST_TYPE m_Type;
	std::queue<DXCommandAllocator*> m_CommandAllocatorQueue;
};