#include "D3DWrapper/CommandQueue.h"
#include "D3DWrapper/CommandList.h"
#include "D3DWrapper/CommandAllocator.h"
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

void CommandQueue::ExecuteCommandLists(RenderEnv* pRenderEnv, UINT numCommandLists, CommandList** ppCommandLists, CommandAllocator* pBarrierCommandAllocator)
{
	CommandListPool* pCommandListPool = pRenderEnv->m_pCommandListPool;

	std::vector<ID3D12CommandList*> commandLists;
	commandLists.reserve(numCommandLists);

	std::vector<CommandList*> barrierCommandLists;
	barrierCommandLists.reserve(numCommandLists);

	for (UINT listIndex = 0; listIndex < numCommandLists; ++listIndex)
	{
		CommandList* pCommandList = ppCommandLists[listIndex];
		RequiredResourceStateList* pRequiredResourceStates = pCommandList->GetRequiredResourceStates();
		if (pRequiredResourceStates != nullptr)
		{
			std::vector<ResourceTransitionBarrier> resourceBarriers;
			resourceBarriers.reserve(pRequiredResourceStates->size());

			for (std::size_t resourceIndex = 0; resourceIndex < pRequiredResourceStates->size(); ++resourceIndex)
			{
				RequiredResourceState& requiredResourceState = (*pRequiredResourceStates)[resourceIndex];
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
				CommandList* pBarrierCommandList = pCommandListPool->Create(pBarrierCommandAllocator, nullptr, L"pBarrierCommandList");
				pBarrierCommandList->ResourceBarrier(resourceBarriers.size(), &resourceBarriers[0]);
				pBarrierCommandList->Close();

				barrierCommandLists.emplace_back(pBarrierCommandList);
				commandLists.emplace_back(pBarrierCommandList->GetD3DObject());
			}
		}
		commandLists.emplace_back(pCommandList->GetD3DObject());
	}
	
	GetD3DObject()->ExecuteCommandLists(commandLists.size(), &commandLists[0]);

	for (std::size_t listIndex = 0; listIndex < barrierCommandLists.size(); ++listIndex)
		pCommandListPool->Release(barrierCommandLists[listIndex]);
}

void CommandQueue::Signal(Fence* pFence, UINT64 value)
{
	VerifyD3DResult(GetD3DObject()->Signal(pFence->GetD3DObject(), value));
}
