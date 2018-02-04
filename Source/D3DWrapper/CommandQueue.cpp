#include "D3DWrapper/CommandQueue.h"
#include "D3DWrapper/CommandList.h"
#include "D3DWrapper/RenderEnv.h"
#include "D3DWrapper/GraphicsResource.h"
#include "D3DWrapper/GraphicsDevice.h"
#include "D3DWrapper/Fence.h"

CommandQueueDesc::CommandQueueDesc(D3D12_COMMAND_LIST_TYPE type)
{
	Type = type;
	Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	NodeMask = 0;
}

CommandQueue::CommandQueue(GraphicsDevice* pDevice, const CommandQueueDesc* pDesc, LPCWSTR pName)
{
	VerifyD3DResult(pDevice->GetD3DObject()->CreateCommandQueue(pDesc, IID_PPV_ARGS(&m_D3DCommandQueue)));
#ifdef ENABLE_GRAPHICS_DEBUGGING
	VerifyD3DResult(m_D3DCommandQueue->SetName(pName));
#endif // ENABLE_GRAPHICS_DEBUGGING
}

void CommandQueue::ExecuteCommandLists(UINT numCommandLists, CommandList** ppCommandLists, Fence* pCompletionFence, UINT64 completionFenceValue)
{
	ID3D12CommandList* d3dCommandLists[MAX_NUM_COMMAND_LISTS_IN_BATCH];

	assert(numCommandLists < MAX_NUM_COMMAND_LISTS_IN_BATCH);
	for (UINT listIndex = 0; listIndex < numCommandLists; ++listIndex)
	{
		CommandList* pCommandList = ppCommandLists[listIndex];

		pCommandList->SetCompletionFence(pCompletionFence, completionFenceValue);
		d3dCommandLists[listIndex] = pCommandList->GetD3DObject();
	}

	m_D3DCommandQueue->ExecuteCommandLists(numCommandLists, d3dCommandLists);
	Signal(pCompletionFence, completionFenceValue);
}

void CommandQueue::Signal(Fence* pFence, UINT64 fenceValue)
{
	VerifyD3DResult(m_D3DCommandQueue->Signal(pFence->GetD3DObject(), fenceValue));
}

UINT64 CommandQueue::GetTimestampFrequency()
{
	UINT64 timestampFrequency = 0;
	VerifyD3DResult(m_D3DCommandQueue->GetTimestampFrequency(&timestampFrequency));
	return timestampFrequency;
}
