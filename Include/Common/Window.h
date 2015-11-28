#pragma once

#include "Common.h"

class Window
{
public:
	Window(HINSTANCE hApp, WNDPROC pfnWndProc,
		LPCWSTR pWndClassName, LPCWSTR pWndName,
		LONG x = CW_USEDEFAULT, LONG y = CW_USEDEFAULT,
		LONG width = CW_USEDEFAULT, LONG height = CW_USEDEFAULT,
		DWORD style = WS_OVERLAPPEDWINDOW);

	~Window();

	HWND GetHWND();
	void Show(int command);
	RECT GetClientRect() const;

private:
	HINSTANCE m_hApp;
	std::wstring m_WndClassName;
	HWND m_hWnd;
};
