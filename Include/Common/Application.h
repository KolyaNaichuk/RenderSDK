#pragma once

#include "Common/Window.h"

class Application
{
public:
	Application(HINSTANCE hApp, LPCWSTR pWndowName,
		LONG windowX = CW_USEDEFAULT, LONG windowY = CW_USEDEFAULT,
		LONG windowWidth = CW_USEDEFAULT, LONG windowHeight = CW_USEDEFAULT);
	virtual ~Application();

	int Run(int showCommand);
	
private:
	virtual void OnInit() = 0;
	virtual void OnUpdate(float deltaTimeInMS) = 0;
	virtual void OnRender() = 0;
	virtual void OnDestroy() = 0;

protected:
	Window* m_pWindow;
};
