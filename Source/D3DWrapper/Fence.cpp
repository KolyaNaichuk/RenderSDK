#include "D3DWrapper/Fence.h"
#include "D3DWrapper/GraphicsDevice.h"

Fence::Fence(GraphicsDevice* pDevice, UINT64 initialValue, LPCWSTR pName)
	: m_hCompletionEvent(INVALID_HANDLE_VALUE)
	, m_CompletedValue(initialValue)
{
	D3D12_FENCE_FLAGS flags = D3D12_FENCE_FLAG_NONE;
	VerifyD3DResult(pDevice->GetD3DObject()->CreateFence(initialValue, flags, IID_PPV_ARGS(&m_D3DFence)));

#ifdef ENABLE_GRAPHICS_DEBUGGING
	VerifyD3DResult(m_D3DFence->SetName(pName));
#endif // ENABLE_GRAPHICS_DEBUGGING
	
	m_hCompletionEvent = ::CreateEventEx(nullptr, FALSE, FALSE, EVENT_ALL_ACCESS);
	assert(m_hCompletionEvent != INVALID_HANDLE_VALUE);
}

Fence::~Fence()
{
	::CloseHandle(m_hCompletionEvent);
}

void Fence::Clear(UINT64 value)
{
	VerifyD3DResult(m_D3DFence->Signal(value));
	m_CompletedValue = value;
}

void Fence::WaitForSignalOnCPU(UINT64 value)
{
	if (ReceivedSignal(value))
		return;

	VerifyD3DResult(m_D3DFence->SetEventOnCompletion(value, m_hCompletionEvent));
	::WaitForSingleObject(m_hCompletionEvent, INFINITE);
}

bool Fence::ReceivedSignal(UINT64 value)
{
	if (value > m_CompletedValue)
		m_CompletedValue = m_D3DFence->GetCompletedValue();

	return (m_CompletedValue >= value);
}
