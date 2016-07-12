#include "D3DWrapper/D3DFence.h"
#include "D3DWrapper/D3DDevice.h"

D3DFence::D3DFence(D3DDevice* pDevice, UINT64 initialValue, D3D12_FENCE_FLAGS flags)
	: m_hCompletionEvent(INVALID_HANDLE_VALUE)
{
	DXVerify(pDevice->GetDXObject()->CreateFence(initialValue, flags, IID_PPV_ARGS(GetDXObjectAddress())));

	m_hCompletionEvent = ::CreateEventEx(nullptr, FALSE, FALSE, EVENT_ALL_ACCESS);
	assert(m_hCompletionEvent != INVALID_HANDLE_VALUE);
}

D3DFence::~D3DFence()
{
	::CloseHandle(m_hCompletionEvent);
}

void D3DFence::Clear(UINT64 value)
{
	DXVerify(GetDXObject()->Signal(value));
}

void D3DFence::WaitForSignal(UINT64 value)
{
	if (HasBeenSignaled(value))
		return;

	DXVerify(GetDXObject()->SetEventOnCompletion(value, m_hCompletionEvent));
	::WaitForSingleObject(m_hCompletionEvent, INFINITE);
}

bool D3DFence::HasBeenSignaled(UINT64 value)
{
	return (GetDXObject()->GetCompletedValue() >= value);
}

D3DSyncPoint::D3DSyncPoint(D3DFence* pFence, UINT64 fenceValue)
	: m_pFence(pFence)
	, m_FenceValue(fenceValue)
{
	assert(m_pFence != nullptr);
}

bool D3DSyncPoint::IsComplete()
{
	return m_pFence->HasBeenSignaled(m_FenceValue);
}

void D3DSyncPoint::WaitForCompletion()
{
	m_pFence->WaitForSignal(m_FenceValue);
}
