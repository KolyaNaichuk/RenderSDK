#pragma once

#include "DXObject.h"

class D3DFactory : public DXObject<IDXGIFactory4>
{
public:
	D3DFactory();
};
