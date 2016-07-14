#pragma once

#include "D3DWrapper/Common.h"

struct Viewport : public D3D12_VIEWPORT
{
	Viewport(FLOAT topLeftX, FLOAT topLeftY, FLOAT width, FLOAT height,
		FLOAT minDepth = 0.0f, FLOAT maxDepth = 1.0f);
};

struct Rect : public D3D12_RECT
{
	Rect(LONG upperLeftX, LONG upperLeftY, LONG lowerRightX, LONG lowerRightY);
};

const Rect ExtractRect(const Viewport* viewport);

UINT GetSizeInBytes(DXGI_FORMAT format);