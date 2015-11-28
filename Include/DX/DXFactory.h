#pragma once

#include "DXObject.h"

class DXFactory : public DXObject<IDXGIFactory4>
{
public:
	DXFactory();
};
