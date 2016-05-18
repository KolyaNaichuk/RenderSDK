#include "Common/MeshDataUtilities.h"
#include "Common/MeshData.h"
#include "Math/Vector2.h"
#include "Math/Vector3.h"
#include "Math/Vector4.h"

namespace
{
	template <typename Index>
	void FlipIndicesWindingOrder(u32 numIndices, Index* pIndices)
	{
		assert((numIndices % 3) == 0);
		for (u32 offset = 0; offset < numIndices; offset += 3)
			std::swap(pIndices[offset], pIndices[offset + 2]);
	}
}

void ConvertMeshData(MeshData* pMeshData, u8 convertionFlags)
{
	VertexData* pVertexData = pMeshData->GetVertexData();

	const u32 numVertices = pVertexData->GetNumVertices();
	const u8 vertexFormatFlags = pVertexData->GetFormatFlags();

	if ((convertionFlags & ConvertionFlag_FlipZCoord) != 0)
	{
		assert((vertexFormatFlags & VertexData::FormatFlag_Position) != 0);
		{
			Vector3f* pPositions = pVertexData->GetPositions();

			for (u32 index = 0; index < numVertices; ++index)
				pPositions[index].m_Z *= -1.0f;
		}
		if ((vertexFormatFlags & VertexData::FormatFlag_Normal) != 0)		
		{
			Vector3f* pNormals = pVertexData->GetNormals();

			for (u32 index = 0; index < numVertices; ++index)
				pNormals[index].m_Z *= -1.0f;
		}
		if ((vertexFormatFlags & VertexData::FormatFlag_Tangent) != 0)
		{
			Vector3f* pTangents = pVertexData->GetTangents();

			for (u32 index = 0; index < numVertices; ++index)
				pTangents[index].m_Z *= -1.0f;
		}
	}

	if ((convertionFlags & ConvertionFlag_FlipTexCoords) != 0)
	{
		if ((vertexFormatFlags & VertexData::FormatFlag_TexCoords) != 0)
		{
			Vector2f* pTexCoords = pVertexData->GetTexCoords();

			for (u32 index = 0; index < numVertices; ++index)
				pTexCoords[index].m_Y = 1.0f - pTexCoords[index].m_Y;
		}
	}
	
	if (convertionFlags & ConvertionFlag_FlipWindingOrder)
	{
		IndexData* pIndexData = pMeshData->GetIndexData();
		const u32 numIndices = pIndexData->GetNumIndices();

		if (pIndexData->GetFormat() == DXGI_FORMAT_R16_UINT)
			FlipIndicesWindingOrder(numIndices, pIndexData->Get16BitIndices());
		else
			FlipIndicesWindingOrder(numIndices, pIndexData->Get32BitIndices()); 
	}
}
