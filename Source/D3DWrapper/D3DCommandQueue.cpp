#include "D3DWrapper/D3DCommandQueue.h"
#include "D3DWrapper/D3DCommandList.h"
#include "D3DWrapper/D3DCommandAllocator.h"
#include "D3DWrapper/D3DRenderEnv.h"
#include "D3DWrapper/D3DResource.h"
#include "D3DWrapper/D3DDevice.h"
#include "D3DWrapper/D3DFence.h"

D3DCommandQueueDesc::D3DCommandQueueDesc(D3D12_COMMAND_LIST_TYPE type)
{
	Type = type;
	Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	NodeMask = 0;
}

D3DCommandQueue::D3DCommandQueue(D3DDevice* pDevice, const D3DCommandQueueDesc* pDesc, LPCWSTR pName)
	: m_pFence(new D3DFence(pDevice, 0))
	, m_NextFenceValue(1)
{
	DXVerify(pDevice->GetDXObject()->CreateCommandQueue(pDesc, IID_PPV_ARGS(GetDXObjectAddress())));

#ifdef _DEBUG
	SetName(pName);
#endif
}

D3DCommandQueue::~D3DCommandQueue()
{
	SafeDelete(m_pFence);
}

void D3DCommandQueue::ExecuteCommandLists(D3DRenderEnv* pRenderEnv, UINT numCommandLists, D3DCommandList** ppCommandLists, D3DCommandAllocator* pBarrierCommandAllocator)
{
	D3DCommandListPool* pCommandListPool = pRenderEnv->m_pCommandListPool;

	std::vector<ID3D12CommandList*> commandLists;
	commandLists.reserve(numCommandLists);

	std::vector<D3DCommandList*> barrierCommandLists;
	barrierCommandLists.reserve(numCommandLists);

	for (UINT listIndex = 0; listIndex < numCommandLists; ++listIndex)
	{
		D3DCommandList* pCommandList = ppCommandLists[listIndex];
		D3DRequiredResourceStateList* pResourceTransitions = pCommandList->GetResourceTransitions();
		if (pResourceTransitions != nullptr)
		{
			std::vector<D3DResourceTransitionBarrier> resourceBarriers;
			resourceBarriers.reserve(pResourceTransitions->size());

			for (std::size_t resourceIndex = 0; resourceIndex < pResourceTransitions->size(); ++resourceIndex)
			{
				D3DRequiredResourceState& resourceTransition = (*pResourceTransitions)[resourceIndex];
				D3DResource* pResource = resourceTransition.m_pResource;

				D3D12_RESOURCE_STATES stateBefore = pResource->GetState();
				D3D12_RESOURCE_STATES stateAfter = resourceTransition.m_RequiredState;
				
				if (stateBefore != stateAfter)
				{
					resourceBarriers.emplace_back(pResource, stateBefore, stateAfter);
					pResource->SetState(stateAfter);
				}
			}

			if (!resourceBarriers.empty())
			{
				D3DCommandList* pBarrierCommandList = pCommandListPool->Create(pBarrierCommandAllocator, nullptr, L"pBarrierCommandList");
				pBarrierCommandList->ResourceBarrier(resourceBarriers.size(), &resourceBarriers[0]);
				pBarrierCommandList->Close();

				barrierCommandLists.emplace_back(pBarrierCommandList);
				commandLists.emplace_back(pBarrierCommandList->GetDXObject());
			}
		}
		commandLists.emplace_back(pCommandList->GetDXObject());
	}
	
	GetDXObject()->ExecuteCommandLists(commandLists.size(), &commandLists[0]);

	for (std::size_t listIndex = 0; listIndex < barrierCommandLists.size(); ++listIndex)
		pCommandListPool->Release(barrierCommandLists[listIndex]);
}

void D3DCommandQueue::Signal(D3DFence* pFence, UINT64 value)
{
	DXVerify(GetDXObject()->Signal(pFence->GetDXObject(), value));
}
