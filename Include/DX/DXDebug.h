#pragma once

#include "DXObject.h"

class DXDebug : public DXObject<ID3D12Debug>
{
public:
	DXDebug();
	void EnableDebugLayer();
};
