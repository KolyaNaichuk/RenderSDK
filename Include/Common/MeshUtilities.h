#pragma once

#include "Common/Common.h"

class Mesh;
struct Vector3f;

enum FaceNormalWeight
{
	FaceNormalWeight_Equal,
	FaceNormalWeight_ByArea,
	FaceNormalWeight_ByAngle
};

void ComputeNormals(u32 numVertices, const Vector3f* pPositions, u32 numIndices, const u16* pIndices, Vector3f* pNormals, FaceNormalWeight faceNormalWeight = FaceNormalWeight_Equal);

enum ConvertionFlags
{
	ConvertionFlag_FlipZCoord = 1 << 0,
	ConvertionFlag_FlipWindingOrder = 1 << 1,
	ConvertionFlag_FlipTexCoords = 1 << 2,
	ConvertionFlag_LeftHandedCoordSystem = ConvertionFlag_FlipZCoord | ConvertionFlag_FlipWindingOrder | ConvertionFlag_FlipTexCoords
};

void ConvertMesh(Mesh* pMesh, u8 convertionFlags);
