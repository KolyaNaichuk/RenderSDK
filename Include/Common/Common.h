#pragma once

#define WIN32_LEAN_AND_MEAN  // Exclude rarely-used stuff from Windows headers

#include <windows.h>

#include <d3d12.h>
#include <dxgi1_4.h>
#include <D3Dcompiler.h>
#include <wrl.h>

#include <string>
#include <vector>
#include <algorithm>
#include <cassert>
#include <memory>

#include "Common/BasicTypes.h"

using namespace Microsoft::WRL;

static void DXVerify(HRESULT result)
{
	assert(SUCCEEDED(result));
}

template <typename T>
static void SafeDelete(T*& pObject)
{
	delete pObject;
	pObject = nullptr;
}

template <typename T>
static void SafeArrayDelete(T*& pArrayObject)
{
	delete[] pArrayObject;
	pArrayObject = nullptr;
}

#define MS_ALIGN(n) __declspec(align(n))