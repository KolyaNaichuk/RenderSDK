#include "DX/DXCommandQueue.h"
#include "DX/DXDevice.h"
#include "DX/DXFence.h"

DXCommandQueueDesc::DXCommandQueueDesc(D3D12_COMMAND_LIST_TYPE type)
{
	Type = type;
	Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	NodeMask = 0;
}

DXCommandQueue::DXCommandQueue(DXDevice* pDevice, const DXCommandQueueDesc* pDesc, LPCWSTR pName)
{
	DXVerify(pDevice->GetDXObject()->CreateCommandQueue(pDesc, IID_PPV_ARGS(GetDXObjectAddress())));

#ifdef _DEBUG
	SetName(pName);
#endif
}

void DXCommandQueue::ExecuteCommandLists(UINT numCommandLists, ID3D12CommandList** ppDXCommandLists)
{
	GetDXObject()->ExecuteCommandLists(numCommandLists, ppDXCommandLists);
}

void DXCommandQueue::Signal(DXFence* pFence, UINT64 value)
{
	DXVerify(GetDXObject()->Signal(pFence->GetDXObject(), value));
}
