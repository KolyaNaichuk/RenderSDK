#include "DX/DXUtils.h"

DXViewport::DXViewport(FLOAT topLeftX, FLOAT topLeftY, FLOAT width, FLOAT height, FLOAT minDepth, FLOAT maxDepth)
{
	TopLeftX = topLeftX;
	TopLeftY = topLeftY;
	Width = width;
	Height = height;
	MinDepth = minDepth;
	MaxDepth = maxDepth;
}

DXRect::DXRect(LONG upperLeftX, LONG upperLeftY, LONG lowerRightX, LONG lowerRightY)
{
	left = upperLeftX;
	top = upperLeftY;
	right = lowerRightX;
	bottom = lowerRightY;
}
