#include "DX/DXEvent.h"

DXEvent::DXEvent()
	: m_hEvent(CreateEventEx(nullptr, FALSE, FALSE, EVENT_ALL_ACCESS))
{
	assert(m_hEvent != nullptr);
}

DXEvent::~DXEvent()
{
	CloseHandle(m_hEvent);
}

void DXEvent::Wait()
{
	WaitForSingleObjectEx(m_hEvent, INFINITE, FALSE);
}
