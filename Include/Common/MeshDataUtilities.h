#pragma once

class MeshData;

enum ConvertionFlags
{
	ConvertionFlags_FlipZCoord = 1,
	ConvertionFlags_FlipWindingOrder = 2,
	ConvertionFlags_FlipTexCoords = 4,
	ConvertionFlags_LeftHandedCoordSystem = ConvertionFlags_FlipZCoord | ConvertionFlags_FlipWindingOrder | ConvertionFlags_FlipTexCoords
};

void ConvertMeshData(MeshData* pMeshData, ConvertionFlags flags);