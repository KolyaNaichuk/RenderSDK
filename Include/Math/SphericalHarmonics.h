#pragma once

#include "Common/Common.h"

struct Vector3f;

constexpr u32 SHGetBasisFunctionIndex(u32 l, u32 m) { return (l * (l + 1) + m); }
constexpr u32 SHGetNumBasisFunctions(u32 numBands) { return numBands * numBands; }

void SHEvaluateBasisFunctions(f32* pOutBasisFuncValues, u32 numBands, const Vector3f& normDir);
void SHAdd(f32* pOutCoeffs, const f32* pCoeffs1, const f32* pCoeffs2, u32 numCoeffs);
void SHScale(f32* pOutCoeffs, const f32* pCoeffs, f32 scale, u32 numCoeffs);
f32 SHProduct(const f32* pCoeffs1, const f32* pCoeffs2, u32 numCoeffs);

void ZHProjectCosineLobeOrientedAlongZAxis(f32* pOutCoeffs, u32 numBands);
void SHProjectCosineLobe(f32* pOutCoeffs, u32 numBands, const Vector3f& normDir);

f32 SHReconstruct(const f32* pCoeffs, u32 numBands, const Vector3f& normDir);