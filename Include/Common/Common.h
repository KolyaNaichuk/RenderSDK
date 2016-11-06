#pragma once

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <windows.h>

#include <string>
#include <vector>
#include <queue>
#include <map>
#include <unordered_map>
#include <algorithm>
#include <cassert>
#include <memory>
#include <fstream>
#include <sstream>

#include "Common/BasicTypes.h"

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
