#pragma once

#include "Common/Common.h"

struct D3DViewport : public D3D12_VIEWPORT
{
	D3DViewport(FLOAT topLeftX, FLOAT topLeftY, FLOAT width, FLOAT height,
		FLOAT minDepth = 0.0f, FLOAT maxDepth = 1.0f);
};

struct D3DRect : public D3D12_RECT
{
	D3DRect(LONG upperLeftX, LONG upperLeftY, LONG lowerRightX, LONG lowerRightY);
};

const D3DRect ExtractRect(const D3DViewport* viewport);

UINT GetSizeInBytes(DXGI_FORMAT format);