#pragma once

#include "D3DWrapper/DXObject.h"

class D3DDevice;
class D3DFence;

class D3DSyncPoint
{
public:
	D3DSyncPoint(D3DFence* pFence, UINT64 fenceValue);

	bool IsComplete();
	void WaitForCompletion();

private:
	D3DFence* m_pFence;
	UINT64 m_FenceValue;
};

class D3DFence : public DXObject<ID3D12Fence>
{
public:
	D3DFence(D3DDevice* pDevice, UINT64 initialValue, D3D12_FENCE_FLAGS flags = D3D12_FENCE_FLAG_NONE);
	~D3DFence();

	void Clear(UINT64 value);
	void WaitForSignal(UINT64 value);
	bool HasBeenSignaled(UINT64 value);
	
private:
	HANDLE m_hCompletionEvent;
};
