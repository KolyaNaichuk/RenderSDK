#include "D3DWrapper/D3DFactory.h"

D3DFactory::D3DFactory()
{
	DXVerify(CreateDXGIFactory1(IID_PPV_ARGS(GetDXObjectAddress())));
}

