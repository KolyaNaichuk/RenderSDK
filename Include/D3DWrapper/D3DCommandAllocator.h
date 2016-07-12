#pragma once

#include "D3DWrapper/D3DFence.h"

class D3DDevice;

class D3DCommandAllocator : public DXObject<ID3D12CommandAllocator>
{
public:
	D3DCommandAllocator(D3DDevice* pDevice, D3D12_COMMAND_LIST_TYPE type, LPCWSTR pName);
	~D3DCommandAllocator();

	void Reset();
	D3D12_COMMAND_LIST_TYPE GetType() const;
		
	D3DSyncPoint* GetSyncPoint();
	void SetSyncPoint(D3DSyncPoint* pSyncPoint);

private:
	const D3D12_COMMAND_LIST_TYPE m_Type;
	D3DSyncPoint* m_pSyncPoint;
};

class D3DCommandAllocatorPool
{
public:
	D3DCommandAllocatorPool(D3DDevice* pDevice, D3D12_COMMAND_LIST_TYPE type);
	~D3DCommandAllocatorPool();

	D3DCommandAllocator* Create(LPCWSTR pName);
	void Release(D3DCommandAllocator* pCommandAllocator);

private:
	D3DDevice* m_pDevice;
	const D3D12_COMMAND_LIST_TYPE m_Type;
	std::queue<D3DCommandAllocator*> m_CommandAllocatorQueue;
};