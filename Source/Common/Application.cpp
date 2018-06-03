#include "Common/Application.h"
#include "Common/Window.h"

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_DESTROY:
		{
			PostQuitMessage(0);
			return 0;
		}
	}
	return DefWindowProc(hWnd, message, wParam, lParam);
}

Application::Application(HINSTANCE hApp, LPCWSTR pWindowName, LONG windowX, LONG windowY, LONG windowWidth, LONG windowHeight)
	: m_pWindow(new Window(hApp, WndProc, L"DXWindow", pWindowName, windowX, windowY, windowWidth, windowHeight))
{
}

Application::~Application()
{
	SafeDelete(m_pWindow);
}

int Application::Run(int showCommand)
{
	OnInit();
	m_pWindow->Show(showCommand);

	MSG msg;
	ZeroMemory(&msg, sizeof(msg));

	while (true)
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);

			if (msg.message == WM_QUIT)
				break;
		}
		OnUpdate();
		OnRender();
	}

	OnDestroy();
	return msg.wParam;
}
