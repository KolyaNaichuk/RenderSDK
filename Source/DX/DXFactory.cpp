#include "DX/DXFactory.h"

DXFactory::DXFactory()
{
	DXVerify(CreateDXGIFactory1(IID_PPV_ARGS(GetDXObjectAddress())));
}

