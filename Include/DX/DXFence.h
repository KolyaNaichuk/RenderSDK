#pragma once

#include "DXObject.h"

class DXDevice;
class DXEvent;

class DXFence : public DXObject<ID3D12Fence>
{
public:
	DXFence(DXDevice* pDevice, UINT64 initialValue, D3D12_FENCE_FLAGS flags = D3D12_FENCE_FLAG_NONE);

	UINT64 GetCompletedValue();
	void Signal(UINT64 value);
	void SetEventOnCompletion(UINT64 value, DXEvent* pEvent);
};
