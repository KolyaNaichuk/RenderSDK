#pragma once

#include "Common/Common.h"

struct Vector3f;

enum SHProjectinFlags
{
	SHProjectionFlags_DistanceAttenuation = 1 << 0,
	SHProjectionFlags_Normalization = 1 << 1
};


f32 SHEvaluate(i32 l, i32 m, f32 thetaInRadians, f32 phiInRadians);

constexpr i32 SHGetBasisFuncIndex(i32 l, i32 m) { return (l * (l + 1) + m); }
constexpr i32 SHGetNumBasisFuncs(i32 numBands) { return numBands * numBands; }

void SHEvaluateBasis(f32* pOutBasisFuncValues, i32 numBands, const Vector3f& dir);
void SHAdd(f32* pOutCoeffs, const f32* pCoeffs1, const f32* pCoeffs2, i32 numBands);
void SHScale(f32* pOutCoeffs, const f32* pCoeffs, f32 scale, i32 numBands);
f32 SHProduct(const f32* pCoeffs1, const f32* pCoeffs2, i32 numBands);

void SHProjectPointLight(f32* pOutRCoeffs, f32* pOutGCoeffs, f32* pOutBCoeffs,
	i32 numBands, const Vector3f& surfacePoint, const Vector3f& lightPos, Vector3f lightRadiance, u8 flags);

void SHProjectDirectionalLight(f32* pOutRCoeffs, f32* pOutGCoeffs, f32* pOutBCoeffs,
	i32 numBands, const Vector3f& dir, Vector3f lightRadiance, u8 flags);

void ZHProjectCosineLobeOrientedAlongZAxis(f32* pOutCoeffs, i32 numBands);
void SHProjectCosineLobe(f32* pOutCoeffs, i32 numBands, const Vector3f& dir);
