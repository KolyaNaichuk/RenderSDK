#include "DX/DXFactory.h"
#include "DX/DXDebug.h"

DXFactory::DXFactory()
{
#ifdef _DEBUG
	DXDebug debugLayer;
	debugLayer.EnableDebugLayer();
#endif
	DXVerify(CreateDXGIFactory1(IID_PPV_ARGS(GetDXObjectAddress())));
}

