#include "Common/MeshData.h"
#include "Math/Vector2f.h"
#include "Math/Vector3f.h"
#include "Math/Vector4f.h"
#include "Math/BoundingBox.h"

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
}

MeshData::MeshData()
	: m_NumIndices(0)
	, m_pIndices16Bit(nullptr)
	, m_pIndices32Bit(nullptr)
	, m_NumVertices(0)
	, m_pPositions(nullptr)
	, m_pNormals(nullptr)
	, m_pTexCoords(nullptr)
	, m_pColors(nullptr)
	, m_pTangents(nullptr)
	, m_NumSubMeshes(0)
	, m_pSubMeshes(nullptr)
	, m_pBoundingBox(nullptr)
{
}

MeshData::~MeshData()
{
	Clear();
}

u32 MeshData::GetNumFaces() const
{
	return m_NumIndices / 3;
}

u32 MeshData::GetNumIndices() const
{
	return m_NumIndices;
}

u32 MeshData::GetNumVertices() const
{
	return m_NumVertices;
}

void MeshData::SetVertexData(u32 numVertices, const Vector3f* pPositions, const Vector4f* pColors, const Vector3f* pNormals, const Vector2f* pTexCoords, const Vector4f* pTangents)
{
	assert((numVertices > 0) && (pPositions != nullptr));
	m_NumVertices = numVertices;
	
	SafeArrayDelete(m_pPositions);
	m_pPositions = new Vector3f[numVertices];
	std::memcpy(m_pPositions, pPositions, numVertices * sizeof(Vector3f));
		
	SafeArrayDelete(m_pColors);
	if (pColors != nullptr)
	{
		m_pColors = new Vector4f[numVertices];
		std::memcpy(m_pColors, pColors, numVertices * sizeof(Vector4f));
	}
	
	SafeArrayDelete(m_pNormals);
	if (pNormals != nullptr)
	{
		m_pNormals = new Vector3f[numVertices];
		std::memcpy(m_pNormals, pNormals, numVertices * sizeof(Vector3f));
	}

	SafeArrayDelete(m_pTexCoords);
	if (pTexCoords != nullptr)
	{
		m_pTexCoords = new Vector2f[numVertices];
		std::memcpy(m_pTexCoords, pTexCoords, numVertices * sizeof(Vector2f));
	}

	SafeArrayDelete(m_pTangents);
	if (pTangents != nullptr)
	{
		m_pTangents = new Vector4f[numVertices];
		std::memcpy(m_pTangents, pTangents, numVertices * sizeof(Vector4f));
	}

	SafeDelete(m_pBoundingBox);
}

const Vector3f* MeshData::GetPositions() const
{
	return m_pPositions;
}

const Vector3f* MeshData::GetNormals() const
{
	return m_pNormals;
}

const Vector2f* MeshData::GetTexCoords() const
{
	return m_pTexCoords;
}

const Vector4f* MeshData::GetColors() const
{
	return m_pColors;
}

const Vector4f* MeshData::GetTangents() const
{
	return m_pTangents;
}

void MeshData::SetIndexData(u32 numIndices, const u16* pIndices)
{
	assert((numIndices > 0) && ((numIndices % 3) == 0) && (pIndices != nullptr));
	m_NumIndices = numIndices;
		
	SafeArrayDelete(m_pIndices32Bit);
	SafeArrayDelete(m_pIndices16Bit);

	m_pIndices16Bit = new u16[numIndices];
	std::memcpy(m_pIndices16Bit, pIndices, numIndices * sizeof(u16));
}

const u16* MeshData::Get16BitIndices() const
{
	return m_pIndices16Bit;
}

void MeshData::SetIndexData(u32 numIndices, const u32* pIndices)
{
	assert((numIndices > 0) && ((numIndices % 3) == 0) && (pIndices != nullptr));
	m_NumIndices = numIndices;

	SafeArrayDelete(m_pIndices16Bit);
	SafeArrayDelete(m_pIndices32Bit);

	m_pIndices32Bit = new u32[numIndices];
	std::memcpy(m_pIndices32Bit, pIndices, numIndices * sizeof(u32));
}

const u32* MeshData::Get32BitIndices() const
{
	return m_pIndices32Bit;
}

void MeshData::SetSubMeshData(u32 numSubMeshes, const SubMeshData* pSubMeshes)
{
	assert((numSubMeshes > 0) && (pSubMeshes != nullptr));
	m_NumSubMeshes = numSubMeshes;

	SafeArrayDelete(m_pSubMeshes);
	m_pSubMeshes = new SubMeshData[numSubMeshes];
	std::memcpy(m_pSubMeshes, pSubMeshes, numSubMeshes * sizeof(SubMeshData));
}

u32 MeshData::GetNumSubMeshes() const
{
	return m_NumSubMeshes;
}

const SubMeshData* MeshData::GetSubMeshes() const
{
	return m_pSubMeshes;
}

void MeshData::ComputeBoundingBox()
{
	SafeDelete(m_pBoundingBox);
	m_pBoundingBox = new BoundingBox(m_NumVertices, m_pPositions);
}

void MeshData::ComputeNormals(FaceNormalWeight faceNormalWeight)
{
	SafeArrayDelete(m_pNormals);
	m_pNormals = new Vector3f[m_NumVertices];
	
	assert((m_pIndices16Bit != nullptr) || (m_pIndices32Bit != nullptr));
	if (faceNormalWeight == FaceNormalWeight_Equal)
	{
		if (m_pIndices16Bit != nullptr)
		{
			::ComputeNormalsEqualWeight(m_NumVertices, m_pPositions, m_NumIndices, m_pIndices16Bit, m_pNormals);
		}
		else
		{
			::ComputeNormalsEqualWeight(m_NumVertices, m_pPositions, m_NumIndices, m_pIndices32Bit, m_pNormals);
		}
	}
	else if (faceNormalWeight == FaceNormalWeight_ByArea)
	{
		if (m_pIndices16Bit != nullptr)
		{
			::ComputeNormalsWeightedByAngle(m_NumVertices, m_pPositions, m_NumIndices, m_pIndices16Bit, m_pNormals);
		}
		else
		{
			::ComputeNormalsWeightedByAngle(m_NumVertices, m_pPositions, m_NumIndices, m_pIndices32Bit, m_pNormals);
		}
	}
	else if (faceNormalWeight == FaceNormalWeight_ByAngle)
	{
		if (m_pIndices16Bit != nullptr)
		{
			::ComputeNormalsWeightedByAngle(m_NumVertices, m_pPositions, m_NumIndices, m_pIndices16Bit, m_pNormals);
		}
		else
		{
			::ComputeNormalsWeightedByAngle(m_NumVertices, m_pPositions, m_NumIndices, m_pIndices32Bit, m_pNormals);
		}
	}
	else
	{
		assert(false);
	}
}

const BoundingBox* MeshData::GetBoundingBox() const
{
	return m_pBoundingBox;
}

void MeshData::Clear()
{
	m_NumIndices = 0;
	SafeArrayDelete(m_pIndices16Bit);
	SafeArrayDelete(m_pIndices32Bit);

	m_NumVertices = 0;
	SafeArrayDelete(m_pPositions);
	SafeArrayDelete(m_pNormals);
	SafeArrayDelete(m_pTexCoords);
	SafeArrayDelete(m_pColors);
	SafeArrayDelete(m_pTangents);

	m_NumSubMeshes = 0;
	SafeArrayDelete(m_pSubMeshes);

	SafeDelete(m_pBoundingBox);
}
