#pragma once

#include "D3DWrapper/Common.h"

class CommandList;
class GraphicsDevice;
class Fence;
struct RenderEnv;

struct CommandQueueDesc : public D3D12_COMMAND_QUEUE_DESC
{
	CommandQueueDesc(D3D12_COMMAND_LIST_TYPE type);
};

class CommandQueue
{
public:
	CommandQueue(GraphicsDevice* pDevice, const CommandQueueDesc* pDesc, LPCWSTR pName);
	ID3D12CommandQueue* GetD3DObject() { return m_D3DCommandQueue.Get(); }
	
	void ExecuteCommandLists(RenderEnv* pRenderEnv, UINT numCommandLists, CommandList** ppCommandLists, Fence* pCompletionFence, UINT64 completionFenceValue);
	void Signal(Fence* pFence, UINT64 fenceValue);

private:
	ComPtr<ID3D12CommandQueue> m_D3DCommandQueue;
};
