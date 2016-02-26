#pragma once

#include "Common/Common.h"

class Radian;
struct Vector3f;

f32 SHEvaluate(i32 l, i32 m, const Radian& theta, const Radian& phi);

i32 SHGetBasisFuncIndex(i32 l, i32 m);
i32 SHGetNumBasisFuncs(i32 numBands);
void SHEvaluateBasis(f32* pOutBasisFuncValues, i32 numBands, const Vector3f& dir);
void SHAdd(f32* pOutCoeffs, const f32* pCoeffs1, const f32* pCoeffs2, i32 numBands);
void SHScale(f32* pOutCoeffs, const f32* pCoeffs, f32 scale, i32 numBands);
f32 SHDot(const f32* pCoeffs1, const f32* pCoeffs2, i32 numBands);
void ZHProjectClampedCosineFunc(f32* pOutCoeffs, i32 numBands);
