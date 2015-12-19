#pragma once

#include "Common/Common.h"

class MeshData;

enum ConvertionFlags
{
	ConvertionFlag_FlipZCoord = 1,
	ConvertionFlag_FlipWindingOrder = 2,
	ConvertionFlag_FlipTexCoords = 4,
	ConvertionFlag_LeftHandedCoordSystem = ConvertionFlag_FlipZCoord | ConvertionFlag_FlipWindingOrder | ConvertionFlag_FlipTexCoords
};

void ConvertMeshData(MeshData* pMeshData, u8 convertionFlags);
