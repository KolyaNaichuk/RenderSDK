#include "Common/Window.h"

Window::Window(HINSTANCE hApp, WNDPROC pfnWndProc, LPCWSTR pWndClassName, LPCWSTR pWndName,
			   LONG x, LONG y, LONG width, LONG height, DWORD style)
	: m_hApp(hApp)
	, m_WndClassName(pWndClassName)
	, m_hWnd(nullptr)
{
	WNDCLASSEX wcex;
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = pfnWndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hApp;
	wcex.hIcon = nullptr;
	wcex.hCursor = nullptr;
	wcex.hbrBackground = nullptr;
	wcex.lpszMenuName = nullptr;
	wcex.lpszClassName = pWndClassName;
	wcex.hIconSm = nullptr;

	if (RegisterClassEx(&wcex))
	{
		RECT rect = {0, 0, width, height};
		AdjustWindowRect(&rect, style, FALSE);

		m_hWnd = CreateWindow(pWndClassName, pWndName, style, x, y,
			rect.right - rect.left, rect.bottom - rect.top,
			nullptr, nullptr, m_hApp, nullptr);
	}
}

Window::~Window()
{
	UnregisterClass(m_WndClassName.c_str(), m_hApp);
}

HWND Window::GetHWND()
{
	return m_hWnd;
}

void Window::Show(int command)
{
	ShowWindow(m_hWnd, command);
	UpdateWindow(m_hWnd);
}

RECT Window::GetClientRect() const
{
	RECT rect;
	::GetClientRect(m_hWnd, &rect);
	return rect;
}

void Window::SetWindowText(LPCWSTR pText)
{
	::SetWindowText(m_hWnd, pText);
}
