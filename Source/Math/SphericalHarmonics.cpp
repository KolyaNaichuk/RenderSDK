#include "Math/SphericalHarmonics.h"
#include "Math/Vector3.h"

namespace
{
	const u32 MIN_NUM_BANDS = 1;
	const u32 MAX_NUM_BANDS = 5;
}

void SHEvaluateBasisFunctions(f32* pOutBasisFuncValues, u32 numBands, const Vector3f& normDir)
{
	// Based on Appendix A2 Polynomial Forms of SH Basis
	// from Stupid Spherical Harmonics Tricks by Peter-Pike Sloan 

	assert(pOutBasisFuncValues != nullptr);
	assert(IsInRange(MIN_NUM_BANDS, MAX_NUM_BANDS, numBands));
	assert(IsNormalized(normDir));
	
	pOutBasisFuncValues[0] = 0.282095f;
	if (numBands == 1)
		return;

	pOutBasisFuncValues[1] = -0.488603f * normDir.m_Y;
	pOutBasisFuncValues[2] =  0.488603f * normDir.m_Z;
	pOutBasisFuncValues[3] = -0.488603f * normDir.m_X;
	if (numBands == 2)
		return;

	const Vector3f squaredNormDir = Sqr(normDir);

	pOutBasisFuncValues[4] =  1.092548f * normDir.m_X * normDir.m_Y;
	pOutBasisFuncValues[5] = -1.092548f * normDir.m_Y * normDir.m_Z;
	pOutBasisFuncValues[6] =  0.315392f * (3.0f * squaredNormDir.m_Z - 1.0f);
	pOutBasisFuncValues[7] = -1.092548f * normDir.m_X * normDir.m_Z;
	pOutBasisFuncValues[8] =  0.546274f * (squaredNormDir.m_X - squaredNormDir.m_Y);
	if (numBands == 3)
		return;

	pOutBasisFuncValues[ 9] = -0.590044f * normDir.m_Y * (3.0f * squaredNormDir.m_X - squaredNormDir.m_Y);
	pOutBasisFuncValues[10] =  2.890611f * normDir.m_X * normDir.m_Y * normDir.m_Z;
	pOutBasisFuncValues[11] = -0.457046f * normDir.m_Y * (5.0f * squaredNormDir.m_Z - 1.0f);
	pOutBasisFuncValues[12] =  0.373176f * normDir.m_Z * (5.0f * squaredNormDir.m_Z - 3.0f);
	pOutBasisFuncValues[13] = -0.457046f * normDir.m_X * (5.0f * squaredNormDir.m_Z - 1.0f);
	pOutBasisFuncValues[14] =  1.445306f * normDir.m_Z * (squaredNormDir.m_X - squaredNormDir.m_Y);
	pOutBasisFuncValues[15] = -0.590044f * normDir.m_X * (squaredNormDir.m_X - 3.0f * squaredNormDir.m_Y);
	if (numBands == 4)
		return;
	
	pOutBasisFuncValues[16] =  2.503343f * normDir.m_X * normDir.m_Y * (squaredNormDir.m_X - squaredNormDir.m_Y);
	pOutBasisFuncValues[17] = -1.770131f * normDir.m_Y * normDir.m_Z * (3.0f * squaredNormDir.m_X - squaredNormDir.m_Y);
	pOutBasisFuncValues[18] =  0.946175f * normDir.m_X * normDir.m_Y * (7.0f * squaredNormDir.m_Z - 1.0f);
	pOutBasisFuncValues[19] = -0.669047f * normDir.m_Y * normDir.m_Z * (7.0f * squaredNormDir.m_Z - 3.0f);
	pOutBasisFuncValues[20] =  0.105786f * (35.0f * Sqr(squaredNormDir.m_Z) - 30.0f * squaredNormDir.m_Z + 3.0f);
	pOutBasisFuncValues[21] = -0.669047f * normDir.m_X * normDir.m_Z * (7.0f * squaredNormDir.m_Z - 3.0f);
	pOutBasisFuncValues[22] =  0.473087f * (squaredNormDir.m_X - squaredNormDir.m_Y) * (7.0f * squaredNormDir.m_Z - 1.0f);
	pOutBasisFuncValues[23] = -1.770131f * normDir.m_X * normDir.m_Z * (squaredNormDir.m_X - 3.0f * squaredNormDir.m_Y);
	pOutBasisFuncValues[24] =  0.625836f * (Sqr(squaredNormDir.m_X) - 6.0f * squaredNormDir.m_X * squaredNormDir.m_Y + Sqr(squaredNormDir.m_Y));
	if (numBands == 5)
		return;
}

void SHAdd(f32* pOutCoeffs, const f32* pCoeffs1, const f32* pCoeffs2, u32 numCoeffs)
{
	assert((pOutCoeffs != nullptr) && (pCoeffs1 != nullptr) && (pCoeffs2 != nullptr));
	
	for (u32 i = 0; i < numCoeffs; ++i)
		pOutCoeffs[i] = pCoeffs1[i] + pCoeffs2[i];
}

void SHScale(f32* pOutCoeffs, const f32* pCoeffs, f32 scale, u32 numCoeffs)
{
	assert((pOutCoeffs != nullptr) && (pCoeffs != nullptr));

	for (u32 i = 0; i < numCoeffs; ++i)
		pOutCoeffs[i] = scale * pCoeffs[i];
}

f32 SHProduct(const f32* pCoeffs1, const f32* pCoeffs2, u32 numCoeffs)
{
	assert((pCoeffs1 != nullptr) && (pCoeffs2 != nullptr));
	
	f32 product = 0.0f;
	for (u32 i = 0; i < numCoeffs; ++i)
		product += pCoeffs1[i] * pCoeffs2[i];
	
	return product;
}

void ZHProjectCosineLobeOrientedAlongZAxis(f32* pOutCoeffs, u32 numBands)
{
	// Based on Properties of the Convolution Kernel
	// from Lambertian Reflectance and Linear Subspaces by Ronen Basri and Dawid W. Jacobs
	
	assert(pOutCoeffs != nullptr);
	assert(IsInRange(MIN_NUM_BANDS, MAX_NUM_BANDS, numBands));
	
	pOutCoeffs[0] = 0.886227f;
	if (numBands == 1)
		return;

	pOutCoeffs[1] = 1.023327f;
	if (numBands == 2)
		return;

	pOutCoeffs[2] = 0.495416f;
	if (numBands == 3)
		return;

	pOutCoeffs[3] = 0.0f;
	if (numBands == 4)
		return;

	pOutCoeffs[4] = -0.110778f;
	if (numBands == 5)
		return;
}

void SHProjectCosineLobe(f32* pOutCoeffs, u32 numBands, const Vector3f& normDir)
{
	// Based on Zonal Harmonics section from Stupid Spherical Harmonics Tricks by Peter-Pike Sloan
	// and Efficient Representation for Irradiance Environment Maps by Ramamoorthi and Hanrahan.
	// We compute zonal harmonics coefficients for cosine lobe oriented along Z axis.
	// Then, we rotate them so that they are aligned with actual direction of the cosine lobe.
	
	assert(pOutCoeffs != nullptr);
	assert(IsInRange(MIN_NUM_BANDS, MAX_NUM_BANDS, numBands));

	f32 basisFuncValues[SHGetNumBasisFunctions(MAX_NUM_BANDS)];
	SHEvaluateBasisFunctions(basisFuncValues, numBands, normDir);

	f32 zonalCosineLobeCoeffs[MAX_NUM_BANDS];
	ZHProjectCosineLobeOrientedAlongZAxis(zonalCosineLobeCoeffs, numBands);
	
	zonalCosineLobeCoeffs[0] *= Sqrt(4.0f * PI); // A0 = 3.141593f
	pOutCoeffs[0] = zonalCosineLobeCoeffs[0] * basisFuncValues[0];
	if (numBands == 1)
		return;

	zonalCosineLobeCoeffs[1] *= Sqrt(4.0f * PI / 3.0f); // A1 = 2.094395f
	pOutCoeffs[1] = zonalCosineLobeCoeffs[1] * basisFuncValues[1];
	pOutCoeffs[2] = zonalCosineLobeCoeffs[1] * basisFuncValues[2];
	pOutCoeffs[3] = zonalCosineLobeCoeffs[1] * basisFuncValues[3];
	if (numBands == 2)
		return;

	zonalCosineLobeCoeffs[2] *= Sqrt(4.0f * PI / 5.0f); // A2 = 0.785398f
	pOutCoeffs[4] = zonalCosineLobeCoeffs[2] * basisFuncValues[4];
	pOutCoeffs[5] = zonalCosineLobeCoeffs[2] * basisFuncValues[5];
	pOutCoeffs[6] = zonalCosineLobeCoeffs[2] * basisFuncValues[6];
	pOutCoeffs[7] = zonalCosineLobeCoeffs[2] * basisFuncValues[7];
	pOutCoeffs[8] = zonalCosineLobeCoeffs[2] * basisFuncValues[8];
	if (numBands == 3)
		return;

	// zonalCosineLobeCoeffs[3] = 0.0f, A3 = 0.0f
	pOutCoeffs[ 9] = 0.0f;
	pOutCoeffs[10] = 0.0f;
	pOutCoeffs[11] = 0.0f;
	pOutCoeffs[12] = 0.0f;
	pOutCoeffs[13] = 0.0f;
	pOutCoeffs[14] = 0.0f;
	pOutCoeffs[15] = 0.0f;
	if (numBands == 4)
		return;
	
	zonalCosineLobeCoeffs[4] *= Sqrt(4.0f * PI / 9.0f); // A4 = -0.130900
	pOutCoeffs[16] = zonalCosineLobeCoeffs[4] * basisFuncValues[16];
	pOutCoeffs[17] = zonalCosineLobeCoeffs[4] * basisFuncValues[17];
	pOutCoeffs[18] = zonalCosineLobeCoeffs[4] * basisFuncValues[18];
	pOutCoeffs[19] = zonalCosineLobeCoeffs[4] * basisFuncValues[19];
	pOutCoeffs[20] = zonalCosineLobeCoeffs[4] * basisFuncValues[20];
	pOutCoeffs[21] = zonalCosineLobeCoeffs[4] * basisFuncValues[21];
	pOutCoeffs[22] = zonalCosineLobeCoeffs[4] * basisFuncValues[22];
	pOutCoeffs[23] = zonalCosineLobeCoeffs[4] * basisFuncValues[23];
	pOutCoeffs[24] = zonalCosineLobeCoeffs[4] * basisFuncValues[24];
	if (numBands == 5)
		return;
}

f32 SHReconstruct(const f32* pCoeffs, u32 numBands, const Vector3f& normDir)
{
	assert(pCoeffs != nullptr);

	f32 basisFuncValues[SHGetNumBasisFunctions(MAX_NUM_BANDS)];
	SHEvaluateBasisFunctions(basisFuncValues, numBands, normDir);

	f32 value = 0.0f;
	
	const u32 numBasisFunctions = SHGetNumBasisFunctions(numBands);
	for (u32 i = 0; i < numBasisFunctions; ++i)
		value += pCoeffs[i] * basisFuncValues[i];

	return value;
}