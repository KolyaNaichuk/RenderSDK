#pragma once

#include "DX/DXObject.h"

class DXDevice;
class DXFence;

class DXSyncPoint
{
public:
	DXSyncPoint(DXFence* pFence, UINT64 fenceValue);

	bool IsComplete();
	void WaitForCompletion();

private:
	DXFence* m_pFence;
	UINT64 m_FenceValue;
};

class DXFence : public DXObject<ID3D12Fence>
{
public:
	DXFence(DXDevice* pDevice, UINT64 initialValue, D3D12_FENCE_FLAGS flags = D3D12_FENCE_FLAG_NONE);
	~DXFence();

	void Clear(UINT64 value);
	void WaitForSignal(UINT64 value);
	bool HasBeenSignaled(UINT64 value);
	
private:
	HANDLE m_hCompletionEvent;
};
