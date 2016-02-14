#pragma once

#include "Common/Common.h"

class Radian;
struct Vector3f;

f32 SHEvaluate(i32 l, i32 m, const Radian& theta, const Radian& phi);

i32 SHGetBasisFuncIndex(i32 l, i32 m);
i32 SHGetNumBasisFuncs(i32 numBands);
void SHEvaluateBasis(f32* pBasisFuncValues, i32 numBands, const Vector3f& dir);

void SHProjectClampedCosineFunc(f32* pCoeffs, i32 numBands, const Vector3f& direction);