#include "Common/Common.h"

void DXVerify(HRESULT result)
{
	assert(SUCCEEDED(result));
}