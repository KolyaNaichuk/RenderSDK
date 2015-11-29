#include "Common/MeshData.h"
#include "Math/Vector2f.h"
#include "Math/Vector3f.h"

MeshData::MeshData()
	: m_NumIndices(0)
	, m_pIndices16Bit(nullptr)
	, m_pIndices32Bit(nullptr)
	, m_NumVertices(0)
	, m_pPositions(nullptr)
	, m_pNormals(nullptr)
	, m_pTexCoords(nullptr)
	, m_NumSubMeshes(0)
	, m_pSubMeshes(nullptr)
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

void MeshData::SetVertexData(u32 numVertices, const Vector3f* pPositions, const Vector3f* pNormals, const Vector2f* pTexCoords)
{
	assert((numVertices > 0) && (pPositions != nullptr));
	m_NumVertices = numVertices;
	
	SafeArrayDelete(m_pPositions);
	m_pPositions = new Vector3f[numVertices];
	std::memcpy(m_pPositions, pPositions, numVertices * sizeof(Vector3f));

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

void MeshData::SetIndexData(u32 numIndices, const u16* pIndices)
{
	assert((numIndices > 0) && (pIndices != nullptr));
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
	assert((numIndices > 0) && (pIndices != nullptr));
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

void MeshData::Clear()
{
	m_NumIndices = 0;
	SafeArrayDelete(m_pIndices16Bit);
	SafeArrayDelete(m_pIndices32Bit);

	m_NumVertices = 0;
	SafeArrayDelete(m_pPositions);
	SafeArrayDelete(m_pNormals);
	SafeArrayDelete(m_pTexCoords);

	m_NumSubMeshes = 0;
	SafeArrayDelete(m_pSubMeshes);
}
