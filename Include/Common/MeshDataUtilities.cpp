#include "Common/MeshDataUtilities.h"
#include "Common/MeshData.h"
#include "Common/Mesh.h"
#include "Math/Vector2f.h"
#include "Math/Vector3f.h"
#include "Math/Vector4f.h"

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
	const u32 numVertices = pMeshData->GetNumVertices();

	if (convertionFlags & ConvertionFlag_FlipZCoord)
	{
		Vector3f* pPositions = pMeshData->GetPositions();
		assert(pPositions != nullptr);
		{
			for (u32 index = 0; index < numVertices; ++index)
				pPositions[index].m_Z *= -1.0f;
		}

		Vector3f* pNormals = pMeshData->GetNormals();
		if (pNormals != nullptr)
		{
			for (u32 index = 0; index < numVertices; ++index)
				pNormals[index].m_Z *= -1.0f;
		}

		Vector3f* pTangents = pMeshData->GetTangents();
		if (pTangents != nullptr)
		{
			for (u32 index = 0; index < numVertices; ++index)
				pTangents[index].m_Z *= -1.0f;
		}

		Vector3f* pBiTangents = pMeshData->GetBiTangents();
		if (pBiTangents != nullptr)
		{
			for (u32 index = 0; index < numVertices; ++index)
			{
				pBiTangents[index].m_X *= -1.0f;
				pBiTangents[index].m_Y *= -1.0f;
			}			
		}
	}
	
	if (convertionFlags & ConvertionFlag_FlipTexCoords)
	{
		Vector2f* pTexCoords = pMeshData->GetTexCoords();
		if (pTexCoords != nullptr)
		{
			for (u32 index = 0; index < numVertices; ++index)
				pTexCoords[index].m_Y = 1.0f - pTexCoords[index].m_Y;
		}
	}

	if (convertionFlags & ConvertionFlag_FlipWindingOrder)
	{
		const u32 numIndices = pMeshData->GetNumIndices();

		u16* pIndices16Bit = pMeshData->Get16BitIndices();
		if (pIndices16Bit != nullptr)
		{
			FlipIndicesWindingOrder(numIndices, pIndices16Bit);
		}
		else
		{
			u32* pIndices32Bit = pMeshData->Get32BitIndices();
			if (pIndices32Bit != nullptr)
			{
				FlipIndicesWindingOrder(numIndices, pIndices32Bit);
			}
			else
			{
				assert(false);
			}
		} 
	}
}
