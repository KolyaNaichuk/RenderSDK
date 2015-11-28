#include "DX/DXFence.h"
#include "DX/DXDevice.h"
#include "DX/DXEvent.h"

DXFence::DXFence(DXDevice* pDevice, UINT64 initialValue, D3D12_FENCE_FLAGS flags)
{
	DXVerify(pDevice->GetDXObject()->CreateFence(initialValue, flags, IID_PPV_ARGS(GetDXObjectAddress())));
}

UINT64 DXFence::GetCompletedValue()
{
	return GetDXObject()->GetCompletedValue();
}

void DXFence::Signal(UINT64 value)
{
	DXVerify(GetDXObject()->Signal(value));
}

void DXFence::SetEventOnCompletion(UINT64 value, DXEvent* pEvent)
{
	DXVerify(GetDXObject()->SetEventOnCompletion(value, pEvent->GetHandle()));
}
