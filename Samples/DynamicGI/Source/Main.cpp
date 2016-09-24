#include "DXApplication.h"

int APIENTRY wWinMain(HINSTANCE hApp, HINSTANCE hPrevApp, LPWSTR pCmdLine, int showCmd)
{
	UNREFERENCED_PARAMETER(hPrevApp);
	UNREFERENCED_PARAMETER(pCmdLine);

	DXApplication app(hApp);
	return app.Run(showCmd);
}