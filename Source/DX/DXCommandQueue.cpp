#include "DX/DXCommandQueue.h"
#include "DX/DXCommandList.h"
#include "DX/DXResource.h"
#include "DX/DXDevice.h"
#include "DX/DXFence.h"

DXCommandQueueDesc::DXCommandQueueDesc(D3D12_COMMAND_LIST_TYPE type)
{
	Type = type;
	Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	NodeMask = 0;
}

DXCommandQueue::DXCommandQueue(DXDevice* pDevice, DXCommandListPool* pCommandListPool, const DXCommandQueueDesc* pDesc, LPCWSTR pName)
	: m_pCommandListPool(pCommandListPool)
{
	DXVerify(pDevice->GetDXObject()->CreateCommandQueue(pDesc, IID_PPV_ARGS(GetDXObjectAddress())));

#ifdef _DEBUG
	SetName(pName);
#endif
}

void DXCommandQueue::ExecuteCommandLists(UINT numCommandLists, DXCommandList** ppCommandLists)
{
	std::vector<ID3D12CommandList*> commandLists;
	commandLists.reserve(numCommandLists);

	for (UINT commandListIndex = 0; commandListIndex < numCommandLists; ++commandListIndex)
	{
		DXCommandList* pCommandList = ppCommandLists[commandListIndex];
		DXResourceTransitionList* pResourceTransitions = pCommandList->GetResourceTransitions();
		if (pResourceTransitions != nullptr)
		{
			std::vector<DXResourceTransitionBarrier> barriers;
			barriers.reserve(pResourceTransitions->size());

			for (std::size_t resourceIndex = 0; resourceIndex < pResourceTransitions->size(); ++resourceIndex)
			{
				DXResourceTransition& resourceTransition = (*pResourceTransitions)[resourceIndex];
				DXResource* pResource = resourceTransition.m_pResource;

				D3D12_RESOURCE_STATES stateBefore = pResource->GetState();
				D3D12_RESOURCE_STATES stateAfter = resourceTransition.m_ExpectedState;
				
				if (stateBefore != stateAfter)
				{
					barriers.emplace_back(pResource, stateBefore, stateAfter);
					pResource->SetState(stateAfter);
				}
			}

			DXCommandList* pBarrierCommandList = m_pCommandListPool->CreateCommandList();
			DXCommandAllocator* pBarrierCommandAllocator = m_pCommandListPool->CreateCommandAllocator();

			pBarrierCommandList->Reset(pBarrierCommandAllocator, nullptr);
			pBarrierCommandList->ResourceBarrier(barriers.size(), &barriers[0]);
			pBarrierCommandList->Close();

			commandLists.emplace_back(pBarrierCommandList->GetDXObject());
		}
		commandLists.emplace_back(pCommandList->GetDXObject());
	}
	
	GetDXObject()->ExecuteCommandLists(commandLists.size(), &commandLists[0]);
}

void DXCommandQueue::Signal(DXFence* pFence, UINT64 value)
{
	DXVerify(GetDXObject()->Signal(pFence->GetDXObject(), value));
}
