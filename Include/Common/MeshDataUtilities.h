#pragma once

#include "DX/DXPipelineState.h"

class MeshData;

enum ConvertionFlags
{
	ConvertionFlag_FlipZCoord = 1,
	ConvertionFlag_FlipWindingOrder = 2,
	ConvertionFlag_FlipTexCoords = 4,
	ConvertionFlag_LeftHandedCoordSystem = ConvertionFlag_FlipZCoord | ConvertionFlag_FlipWindingOrder | ConvertionFlag_FlipTexCoords
};

void ConvertMeshData(MeshData* pMeshData, u8 convertionFlags);

enum InputElementFlags
{
	InputElementFlag_Position = 1,
	InputElementFlag_Normal = 2,
	InputElementFlag_Color = 4,
	InputElementFlag_Tangent = 8,
	InputElementFlag_BiTangent = 16,
	InputElementFlag_TexCoords = 32
};

void GenerateInputElements(std::vector<DXInputElementDesc>& inputElements, u8 inputElementFlags, u8 vertexElementFlags);
