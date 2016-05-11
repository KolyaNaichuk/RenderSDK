#include "DX/DXCommandQueue.h"
#include "DX/DXCommandList.h"
#include "DX/DXCommandAllocator.h"
#include "DX/DXRenderEnvironment.h"
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

DXCommandQueue::DXCommandQueue(DXDevice* pDevice, const DXCommandQueueDesc* pDesc, LPCWSTR pName)
	: m_pFence(new DXFence(pDevice, 0))
	, m_NextFenceValue(1)
{
	DXVerify(pDevice->GetDXObject()->CreateCommandQueue(pDesc, IID_PPV_ARGS(GetDXObjectAddress())));

#ifdef _DEBUG
	SetName(pName);
#endif
}

DXCommandQueue::~DXCommandQueue()
{
	SafeDelete(m_pFence);
}

void DXCommandQueue::ExecuteCommandLists(DXRenderEnvironment* pEnv, UINT numCommandLists, DXCommandList** ppCommandLists, DXCommandAllocator* pBarrierCommandAllocator)
{
	DXCommandListPool* pCommandListPool = pEnv->m_pCommandListPool;

	std::vector<ID3D12CommandList*> commandLists;
	commandLists.reserve(numCommandLists);

	std::vector<DXCommandList*> barrierCommandLists;
	barrierCommandLists.reserve(numCommandLists);

	for (UINT listIndex = 0; listIndex < numCommandLists; ++listIndex)
	{
		DXCommandList* pCommandList = ppCommandLists[listIndex];
		DXResourceTransitionList* pResourceTransitions = pCommandList->GetResourceTransitions();
		if (pResourceTransitions != nullptr)
		{
			std::vector<DXResourceTransitionBarrier> resourceBarriers;
			resourceBarriers.reserve(pResourceTransitions->size());

			for (std::size_t resourceIndex = 0; resourceIndex < pResourceTransitions->size(); ++resourceIndex)
			{
				DXResourceTransition& resourceTransition = (*pResourceTransitions)[resourceIndex];
				DXResource* pResource = resourceTransition.m_pResource;

				D3D12_RESOURCE_STATES stateBefore = pResource->GetState();
				D3D12_RESOURCE_STATES stateAfter = resourceTransition.m_ExpectedState;
				
				if (stateBefore != stateAfter)
				{
					resourceBarriers.emplace_back(pResource, stateBefore, stateAfter);
					pResource->SetState(stateAfter);
				}
			}

			DXCommandList* pBarrierCommandList = pCommandListPool->Create(pBarrierCommandAllocator, nullptr, L"pBarrierCommandList");			
			pBarrierCommandList->ResourceBarrier(resourceBarriers.size(), &resourceBarriers[0]);
			pBarrierCommandList->Close();

			barrierCommandLists.emplace_back(pBarrierCommandList);
			commandLists.emplace_back(pBarrierCommandList->GetDXObject());
		}
		commandLists.emplace_back(pCommandList->GetDXObject());
	}
	
	GetDXObject()->ExecuteCommandLists(commandLists.size(), &commandLists[0]);

	for (std::size_t listIndex = 0; listIndex < barrierCommandLists.size(); ++listIndex)
		pCommandListPool->Release(barrierCommandLists[listIndex]);
}

void DXCommandQueue::Signal(DXFence* pFence, UINT64 value)
{
	DXVerify(GetDXObject()->Signal(pFence->GetDXObject(), value));
}
