#include "DX/DXFence.h"
#include "DX/DXDevice.h"

DXFence::DXFence(DXDevice* pDevice, UINT64 initialValue, D3D12_FENCE_FLAGS flags)
	: m_hCompletionEvent(INVALID_HANDLE_VALUE)
{
	DXVerify(pDevice->GetDXObject()->CreateFence(initialValue, flags, IID_PPV_ARGS(GetDXObjectAddress())));

	m_hCompletionEvent = ::CreateEventEx(nullptr, FALSE, FALSE, EVENT_ALL_ACCESS);
	assert(m_hCompletionEvent != INVALID_HANDLE_VALUE);
}

DXFence::~DXFence()
{
	::CloseHandle(m_hCompletionEvent);
}

void DXFence::Clear(UINT64 value)
{
	DXVerify(GetDXObject()->Signal(value));
}

void DXFence::WaitForSignal(UINT64 value)
{
	if (HasBeenSignaled(value))
		return;

	DXVerify(GetDXObject()->SetEventOnCompletion(value, m_hCompletionEvent));
	::WaitForSingleObject(m_hCompletionEvent, INFINITE);
}

bool DXFence::HasBeenSignaled(UINT64 value)
{
	return (GetDXObject()->GetCompletedValue() >= value);
}
