#pragma once

#include <d3d12.h>
#include <dxgidebug.h>
#include <dxgi1_3.h>
#include <dxgi1_4.h>
#include <D3Dcompiler.h>
#include <wrl.h>

#include "Common/Common.h"

using namespace Microsoft::WRL;

static void VerifyD3DResult(HRESULT result)
{
	assert(SUCCEEDED(result));
}
