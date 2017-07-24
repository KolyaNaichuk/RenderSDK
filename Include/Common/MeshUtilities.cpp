#include "Common/MeshUtilities.h"
#include "Common/Mesh.h"
#include "Math/Vector2.h"
#include "Math/Vector3.h"
#include "Math/Vector4.h"

namespace
{
	template <typename Index>
	void ComputeNormalsEqualWeight(u32 numVertices, const Vector3f* pPositions, u32 numIndices, const Index* pIndices, Vector3f* pNormals)
	{
		assert((pPositions != nullptr) && (pIndices != nullptr));

		for (u32 offset = 0; offset < numVertices; ++offset)
			pNormals[offset] = Vector3f::ZERO;

		for (u32 offset = 0; offset < numIndices; offset += 3)
		{
			const Index index0 = pIndices[offset + 0];
			const Index index1 = pIndices[offset + 1];
			const Index index2 = pIndices[offset + 2];

			const Vector3f& point0 = pPositions[index0];
			const Vector3f& point1 = pPositions[index1];
			const Vector3f& point2 = pPositions[index2];

			const Vector3f faceNormal = Normalize(Cross(point1 - point0, point2 - point0));

			pNormals[index0] += faceNormal;
			pNormals[index1] += faceNormal;
			pNormals[index2] += faceNormal;
		}

		for (u32 offset = 0; offset < numVertices; ++offset)
			pNormals[offset] = Normalize(pNormals[offset]);
	}

	template <typename Index>
	void ComputeNormalsWeightedByArea(u32 numVertices, const Vector3f* pPositions, u32 numIndices, const Index* pIndices, Vector3f* pNormals)
	{
		assert((pPositions != nullptr) && (pIndices != nullptr));
		assert(false && "Needs impl");
	}

	template <typename Index>
	void ComputeNormalsWeightedByAngle(u32 numVertices, const Vector3f* pPositions, u32 numIndices, const Index* pIndices, Vector3f* pNormals)
	{
		assert((pPositions != nullptr) && (pIndices != nullptr));
		assert(false && "Needs impl");
	}

	template <typename Index>
	void FlipIndicesWindingOrder(u32 numIndices, Index* pIndices)
	{
		assert((numIndices % 3) == 0);
		for (u32 offset = 0; offset < numIndices; offset += 3)
			std::swap(pIndices[offset], pIndices[offset + 2]);
	}
}

void ComputeNormals(u32 numVertices, const Vector3f* pPositions, u32 numIndices, const u16* pIndices, Vector3f* pNormals, FaceNormalWeight faceNormalWeight)
{
	if (faceNormalWeight == FaceNormalWeight_Equal)
	{
		ComputeNormalsEqualWeight(numVertices, pPositions, numIndices, pIndices, pNormals);
		return;
	}
	if (faceNormalWeight == FaceNormalWeight_ByArea)
	{
		ComputeNormalsWeightedByAngle(numVertices, pPositions, numIndices, pIndices, pNormals);
		return;
	}
	if (faceNormalWeight == FaceNormalWeight_ByAngle)
	{
		ComputeNormalsWeightedByAngle(numVertices, pPositions, numIndices, pIndices, pNormals);
		return;
	}
	assert(false);
}

void ConvertMesh(Mesh* pMesh, u8 convertionFlags)
{
	VertexData* pVertexData = pMesh->GetVertexData();

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
		IndexData* pIndexData = pMesh->GetIndexData();
		const u32 numIndices = pIndexData->GetNumIndices();

		if (pIndexData->GetFormat() == DXGI_FORMAT_R16_UINT)
		{
			FlipIndicesWindingOrder(numIndices, pIndexData->Get16BitIndices());
		}			
		else
		{
			FlipIndicesWindingOrder(numIndices, pIndexData->Get32BitIndices());
		}
	}
}
