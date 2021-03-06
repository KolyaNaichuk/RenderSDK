#pragma once

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <windows.h>

#include <string>
#include <cstring>
#include <cassert>
#include <cstdlib>
#include <cwchar>
#include <vector>
#include <array>
#include <queue>
#include <map>
#include <unordered_map>
#include <algorithm>
#include <memory>
#include <fstream>
#include <functional>
#include <sstream>
#include <filesystem>
#include <iostream>

#include "BasicTypes.h"

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

static void VerifyWinAPIResult(BOOL result)
{
	assert(result != 0);
}

enum CubeMapFaces
{
	kCubeMapFacePositiveX = 0,
	kCubeMapFaceNegativeX,
	kCubeMapFacePositiveY,
	kCubeMapFaceNegativeY,
	kCubeMapFacePositiveZ,
	kCubeMapFaceNegativeZ,
	kNumCubeMapFaces
};