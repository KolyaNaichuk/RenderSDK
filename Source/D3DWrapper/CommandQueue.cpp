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
#ifdef _DEBUG
	VerifyD3DResult(m_D3DCommandQueue->SetName(pName));
#endif
}

void CommandQueue::ExecuteCommandLists(RenderEnv* pRenderEnv, UINT numCommandLists, CommandList** ppCommandLists, Fence* pCompletionFence, UINT64 completionFenceValue)
{
	std::vector<ID3D12CommandList*> d3dCommandLists;
	d3dCommandLists.reserve(numCommandLists);
		
	for (UINT listIndex = 0; listIndex < numCommandLists; ++listIndex)
	{
		CommandList* pCommandList = ppCommandLists[listIndex];
		pCommandList->SetCompletionFence(pCompletionFence, completionFenceValue);

		RequiredResourceStateList* pRequiredResourceStates = pCommandList->GetRequiredResourceStates();
		if (pRequiredResourceStates != nullptr)
		{
			std::vector<ResourceTransitionBarrier> resourceBarriers;
			resourceBarriers.reserve(pRequiredResourceStates->size());

			for (std::size_t stateIndex = 0; stateIndex < pRequiredResourceStates->size(); ++stateIndex)
			{
				RequiredResourceState& requiredResourceState = (*pRequiredResourceStates)[stateIndex];
				GraphicsResource* pResource = requiredResourceState.m_pResource;

				D3D12_RESOURCE_STATES stateBefore = pResource->GetState();
				D3D12_RESOURCE_STATES stateAfter = requiredResourceState.m_RequiredState;

				if (stateBefore != stateAfter)
				{
					resourceBarriers.emplace_back(pResource, stateBefore, stateAfter);
					pResource->SetState(stateAfter);
				}
			}

			if (!resourceBarriers.empty())
			{
				CommandList* pBarrierCommandList = pRenderEnv->m_pCommandListPool->Create(L"pBarrierCommandList");
				pBarrierCommandList->Begin();
				pBarrierCommandList->ResourceBarrier(resourceBarriers.size(), &resourceBarriers[0]);
				pBarrierCommandList->End();
				pBarrierCommandList->SetCompletionFence(pCompletionFence, completionFenceValue);
				
				d3dCommandLists.emplace_back(pBarrierCommandList->GetD3DObject());
			}
		}
		d3dCommandLists.emplace_back(pCommandList->GetD3DObject());
	}
	
	m_D3DCommandQueue->ExecuteCommandLists(d3dCommandLists.size(), &d3dCommandLists[0]);
	Signal(pCompletionFence, completionFenceValue);
}

void CommandQueue::Signal(Fence* pFence, UINT64 fenceValue)
{
	VerifyD3DResult(m_D3DCommandQueue->Signal(pFence->GetD3DObject(), fenceValue));
}
