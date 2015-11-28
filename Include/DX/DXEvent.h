#pragma once

#include "Common/Common.h"

class DXEvent
{
public:
	DXEvent();
	~DXEvent();

	void Wait();
	HANDLE GetHandle() { return m_hEvent; }

private:
	HANDLE m_hEvent;
};

