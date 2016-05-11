#pragma once

#include "DX/DXObject.h"

class DXCommandList;
class DXCommandAllocator;
class DXDevice;
class DXFence;
struct DXRenderEnvironment;

struct DXCommandQueueDesc : public D3D12_COMMAND_QUEUE_DESC
{
	DXCommandQueueDesc(D3D12_COMMAND_LIST_TYPE type);
};

class DXCommandQueue : public DXObject<ID3D12CommandQueue>
{
public:
	DXCommandQueue(DXDevice* pDevice, const DXCommandQueueDesc* pDesc, LPCWSTR pName);
	~DXCommandQueue();

	void ExecuteCommandLists(DXRenderEnvironment* pEnv, UINT numCommandLists, DXCommandList** ppCommandLists, DXCommandAllocator* pBarrierCommandAllocator);
	void Signal(DXFence* pFence, UINT64 value);

private:
	DXFence* m_pFence;
	UINT64 m_NextFenceValue;
};
