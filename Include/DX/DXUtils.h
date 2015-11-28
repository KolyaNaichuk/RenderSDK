#pragma once

#include "Common/Common.h"

struct DXViewport : public D3D12_VIEWPORT
{
	DXViewport(FLOAT topLeftX, FLOAT topLeftY, FLOAT width, FLOAT height,
		FLOAT minDepth = 0.0f, FLOAT maxDepth = 1.0f);
};

struct DXRect : public D3D12_RECT
{
	DXRect(LONG upperLeftX, LONG upperLeftY, LONG lowerRightX, LONG lowerRightY);
};