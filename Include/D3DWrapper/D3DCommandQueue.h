#pragma once

#include "D3DWrapper/DXObject.h"

class D3DCommandList;
class D3DCommandAllocator;
class D3DDevice;
class D3DFence;
struct D3DRenderEnv;

struct D3DCommandQueueDesc : public D3D12_COMMAND_QUEUE_DESC
{
	D3DCommandQueueDesc(D3D12_COMMAND_LIST_TYPE type);
};

class D3DCommandQueue : public DXObject<ID3D12CommandQueue>
{
public:
	D3DCommandQueue(D3DDevice* pDevice, const D3DCommandQueueDesc* pDesc, LPCWSTR pName);
	~D3DCommandQueue();

	void ExecuteCommandLists(D3DRenderEnv* pRenderEnv, UINT numCommandLists, D3DCommandList** ppCommandLists, D3DCommandAllocator* pBarrierCommandAllocator);
	void Signal(D3DFence* pFence, UINT64 value);

private:
	D3DFence* m_pFence;
	UINT64 m_NextFenceValue;
};
