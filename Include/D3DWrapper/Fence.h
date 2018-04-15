#pragma once

#include "D3DWrapper/Common.h"

class GraphicsDevice;

class Fence
{
public:
	Fence(GraphicsDevice* pDevice, UINT64 initialValue, LPCWSTR pName);
	~Fence();

	ID3D12Fence* GetD3DObject() { return m_D3DFence.Get(); }

	void Clear(UINT64 value);
	void WaitForSignalOnCPU(UINT64 value);
	bool ReceivedSignal(UINT64 value);
	
private:
	ComPtr<ID3D12Fence> m_D3DFence;
	HANDLE m_hCompletionEvent;
	UINT64 m_CompletedValue;
};
