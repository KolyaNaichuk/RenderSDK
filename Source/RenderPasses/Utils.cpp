#include "RenderPasses/Utils.h"

DXGI_FORMAT DetectOptimalFormat(u32 numElements, const u32* pFirstElement)
{
	const u32 u8Max = ~u8(0);
	const u32 u16Max = ~u16(0);

	bool u8Suffice = true;
	bool u16Suffice = true;

	for (u32 it = 0; it < numElements; ++it, ++pFirstElement)
	{
		u8Suffice = u8Suffice && (*pFirstElement <= u8Max);
		u16Suffice = u16Suffice && (*pFirstElement <= u16Max);
	}
	
	if (u8Suffice)
		return DXGI_FORMAT_R8_UINT;
	
	if (u16Suffice)
		return DXGI_FORMAT_R16_UINT;

	return DXGI_FORMAT_R32_UINT;
}