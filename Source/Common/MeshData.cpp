#include "Common/MeshData.h"
#include "Math/Vector2.h"
#include "Math/Vector3.h"
#include "Math/Vector4.h"

VertexData::~VertexData()
{
	SafeArrayDelete(m_pPositions);
	SafeArrayDelete(m_pNormals);
	SafeArrayDelete(m_pTexCoords);
	SafeArrayDelete(m_pColors);
	SafeArrayDelete(m_pTangents);
}

VertexData::VertexData(u32 numVertices, const Vector3f* pPositions, const Vector3f* pNormals,
	const Vector4f* pColors, const Vector2f* pTexCoords, const Vector3f* pTangents)
	: m_NumVertices(numVertices)
	, m_FormatFlags(0)
	, m_pPositions(nullptr)
	, m_pNormals(nullptr)
	, m_pTexCoords(nullptr)
	, m_pColors(nullptr)
	, m_pTangents(nullptr)
{
	assert(m_NumVertices > 0);
	assert(pPositions != nullptr);
	{
		m_pPositions = new Vector3f[numVertices];
		std::memcpy(m_pPositions, pPositions, numVertices * sizeof(Vector3f));
		
		m_FormatFlags |= FormatFlag_Position;
	}
	if (pColors != nullptr)
	{
		m_pColors = new Vector4f[numVertices];
		std::memcpy(m_pColors, pColors, numVertices * sizeof(Vector4f));
		
		m_FormatFlags |= FormatFlag_Color;
	}
	if (pNormals != nullptr)
	{
		m_pNormals = new Vector3f[numVertices];
		std::memcpy(m_pNormals, pNormals, numVertices * sizeof(Vector3f));

		m_FormatFlags |= FormatFlag_Normal;
	}
	if (pTexCoords != nullptr)
	{
		m_pTexCoords = new Vector2f[numVertices];
		std::memcpy(m_pTexCoords, pTexCoords, numVertices * sizeof(Vector2f));

		m_FormatFlags |= FormatFlag_TexCoords;
	}
	if (pTangents != nullptr)
	{
		m_pTangents = new Vector3f[numVertices];
		std::memcpy(m_pTangents, pTangents, numVertices * sizeof(Vector3f));

		m_FormatFlags |= FormatFlag_Tangent;
	}
}

Vector3f* VertexData::GetPositions()
{
	assert((m_FormatFlags & FormatFlag_Position) != 0);
	return m_pPositions;
}

const Vector3f* VertexData::GetPositions() const
{
	assert((m_FormatFlags & FormatFlag_Position) != 0);
	return m_pPositions;
}

Vector3f* VertexData::GetNormals()
{
	assert((m_FormatFlags & FormatFlag_Normal) != 0);
	return m_pNormals;
}

const Vector3f* VertexData::GetNormals() const
{
	assert((m_FormatFlags & FormatFlag_Normal) != 0);
	return m_pNormals;
}

Vector2f* VertexData::GetTexCoords()
{
	assert((m_FormatFlags & FormatFlag_TexCoords) != 0);
	return m_pTexCoords;
}

const Vector2f* VertexData::GetTexCoords() const
{
	assert((m_FormatFlags & FormatFlag_TexCoords) != 0);
	return m_pTexCoords;
}

Vector4f* VertexData::GetColors()
{
	assert((m_FormatFlags & FormatFlag_Color) != 0);
	return m_pColors;
}

const Vector4f* VertexData::GetColors() const
{
	assert((m_FormatFlags & FormatFlag_Color) != 0);
	return m_pColors;
}

Vector3f* VertexData::GetTangents()
{
	assert((m_FormatFlags & FormatFlag_Tangent) != 0);
	return m_pTangents;
}

const Vector3f* VertexData::GetTangents() const
{
	assert((m_FormatFlags & FormatFlag_Tangent) != 0);
	return m_pTangents;
}

IndexData::IndexData(u32 numIndices, const u16* p16BitIndices)
	: m_Format(DXGI_FORMAT_R16_UINT)
	, m_NumIndices(numIndices)
	, m_p16BitIndices(new u16[numIndices])
	, m_p32BitIndices(nullptr)
{
	assert(((numIndices % 3) == 0) && (p16BitIndices != nullptr));
	std::memcpy(m_p16BitIndices, p16BitIndices, numIndices * sizeof(u16));
}

IndexData::IndexData(u32 numIndices, const u32* p32BitIndices)
	: m_Format(DXGI_FORMAT_R32_UINT)
	, m_NumIndices(numIndices)
	, m_p16BitIndices(nullptr)
	, m_p32BitIndices(new u32[numIndices])
{
	assert(((numIndices % 3) == 0) && (p32BitIndices != nullptr));
	std::memcpy(m_p32BitIndices, p32BitIndices, numIndices * sizeof(u32));
}

IndexData::~IndexData()
{
	SafeArrayDelete(m_p16BitIndices);
	SafeArrayDelete(m_p32BitIndices);
}

u16* IndexData::Get16BitIndices()
{
	assert(m_Format == DXGI_FORMAT_R16_UINT);
	return m_p16BitIndices;
}

const u16* IndexData::Get16BitIndices() const
{
	assert(m_Format == DXGI_FORMAT_R16_UINT);
	return m_p16BitIndices;
}

u32* IndexData::Get32BitIndices()
{
	assert(m_Format == DXGI_FORMAT_R32_UINT);
	return m_p32BitIndices;
}

const u32* IndexData::Get32BitIndices() const
{
	assert(m_Format == DXGI_FORMAT_R32_UINT);
	return m_p32BitIndices;
}

MeshData::MeshData(VertexData* pVertexData, IndexData* pIndexData, D3D12_PRIMITIVE_TOPOLOGY_TYPE primitiveTopologyType, D3D12_PRIMITIVE_TOPOLOGY primitiveTopology, u32 materialIndex)
	: m_pVertexData(pVertexData)
	, m_pIndexData(pIndexData)
	, m_PrimitiveTopologyType(primitiveTopologyType)
	, m_PrimitiveTopology(primitiveTopology)
	, m_MaterialIndex(materialIndex)
{
}

MeshData::~MeshData()
{
	SafeDelete(m_pVertexData);
	SafeDelete(m_pIndexData);
}