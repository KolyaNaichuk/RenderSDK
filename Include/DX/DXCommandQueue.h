#pragma once

#include "DXObject.h"

class DXCommandList;
class DXCommandListPool;
class DXDevice;
class DXFence;

struct DXCommandQueueDesc : public D3D12_COMMAND_QUEUE_DESC
{
	DXCommandQueueDesc(D3D12_COMMAND_LIST_TYPE type);
};

class DXCommandQueue : public DXObject<ID3D12CommandQueue>
{
public:
	DXCommandQueue(DXDevice* pDevice, DXCommandListPool* pCommandListPool, const DXCommandQueueDesc* pDesc, LPCWSTR pName);
	
	void ExecuteCommandLists(UINT numCommandLists, DXCommandList** ppCommandLists);
	void Signal(DXFence* pFence, UINT64 value);

private:
	DXCommandListPool* m_pCommandListPool;
};
