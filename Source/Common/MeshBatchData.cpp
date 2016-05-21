#include "Common/MeshBatchData.h"
#include "Common/MeshData.h"

MeshDesc::MeshDesc(u32 numIndices, u32 startIndexLocation, i32 baseVertexLocation, u32 materialIndex)
	: m_NumIndices(numIndices)
	, m_StartIndexLocation(startIndexLocation)
	, m_BaseVertexLocation(baseVertexLocation)
	, m_MaterialIndex(materialIndex)
{
}

MeshBatchData::MeshBatchData(u8 vertexFormatFlags, DXGI_FORMAT indexFormat, D3D12_PRIMITIVE_TOPOLOGY_TYPE primitiveTopologyType, D3D12_PRIMITIVE_TOPOLOGY primitiveTopology)
	: m_VertexFormatFlags(vertexFormatFlags)
	, m_IndexFormat(indexFormat)
	, m_PrimitiveTopologyType(primitiveTopologyType)
	, m_PrimitiveTopology(primitiveTopology)
{
	assert((m_IndexFormat == DXGI_FORMAT_R16_UINT) || (m_IndexFormat == DXGI_FORMAT_R32_UINT));
}

void MeshBatchData::Append(const MeshData* pMeshData)
{
	assert(m_PrimitiveTopology == pMeshData->GetPrimitiveTopology());
	assert(m_PrimitiveTopologyType == pMeshData->GetPrimitiveTopologyType());

	const VertexData* pVertexData = pMeshData->GetVertexData();
	assert(pVertexData != nullptr);
	assert(m_VertexFormatFlags == (m_VertexFormatFlags & pVertexData->GetFormatFlags()));

	const IndexData* pIndexData = pMeshData->GetIndexData();
	assert(pIndexData != nullptr);
	assert(m_IndexFormat == pIndexData->GetFormat());

	const i32 baseVertexLocation = (i32)GetNumVertices();
	const u32 numVertices = pVertexData->GetNumVertices();

	{
		const Vector3f* pPositions = pVertexData->GetPositions();
		m_Positions.insert(m_Positions.end(), pPositions, pPositions + numVertices);
	}	
	if ((m_VertexFormatFlags & VertexData::FormatFlag_Normal) != 0)
	{
		const Vector3f* pNormals = pVertexData->GetNormals();
		m_Normals.insert(m_Normals.end(), pNormals, pNormals + numVertices);
	}
	if ((m_VertexFormatFlags & VertexData::FormatFlag_TexCoords) != 0)
	{
		const Vector2f* pTexCoords = pVertexData->GetTexCoords();
		m_TexCoords.insert(m_TexCoords.end(), pTexCoords, pTexCoords + numVertices);
	}
	if ((m_VertexFormatFlags & VertexData::FormatFlag_Color) != 0)
	{
		const Vector4f* pColors = pVertexData->GetColors();
		m_Colors.insert(m_Colors.end(), pColors, pColors + numVertices);
	}
	if ((m_VertexFormatFlags & VertexData::FormatFlag_Tangent) != 0)
	{
		const Vector3f* pTangents = pVertexData->GetTangents();
		m_Tangents.insert(m_Tangents.end(), pTangents, pTangents + numVertices);
	}
	
	const u32 startIndexLocation = GetNumIndices();
	const u32 numIndices = pIndexData->GetNumIndices();

	if (m_IndexFormat == DXGI_FORMAT_R16_UINT)
	{
		const u16* p16BitIndices = pIndexData->Get16BitIndices();
		m_16BitIndices.insert(m_16BitIndices.end(), p16BitIndices, p16BitIndices + numIndices);
	}
	else
	{
		const u32* p32BitIndices = pIndexData->Get32BitIndices();
		m_32BitIndices.insert(m_32BitIndices.end(), p32BitIndices, p32BitIndices + numIndices);
	}

	const u32 materialIndex = m_Materials.size();
	m_Materials.emplace_back(*pMeshData->GetMaterial());
	
	m_MeshDescs.emplace_back(numIndices, startIndexLocation, baseVertexLocation, materialIndex);
	m_MeshAABBs.emplace_back(*pMeshData->GetAABB());
}

u32 MeshBatchData::GetNumVertices() const
{
	return m_Positions.size();
}

const Vector3f* MeshBatchData::GetPositions() const
{
	assert((m_VertexFormatFlags & VertexData::FormatFlag_Position) != 0);
	return &m_Positions[0];
}

const Vector3f* MeshBatchData::GetNormals() const
{
	assert((m_VertexFormatFlags & VertexData::FormatFlag_Normal) != 0);
	return &m_Normals[0];
}

const Vector2f* MeshBatchData::GetTexCoords() const
{
	assert((m_VertexFormatFlags & VertexData::FormatFlag_TexCoords) != 0);
	return &m_TexCoords[0];
}

const Vector4f* MeshBatchData::GetColors() const
{
	assert((m_VertexFormatFlags & VertexData::FormatFlag_Color) != 0);
	return &m_Colors[0];
}

const Vector3f* MeshBatchData::GetTangents() const
{
	assert((m_VertexFormatFlags & VertexData::FormatFlag_Tangent) != 0);
	return &m_Tangents[0];
}

u32 MeshBatchData::GetNumIndices() const
{
	if (m_IndexFormat == DXGI_FORMAT_R16_UINT)
		return m_16BitIndices.size();
	return m_32BitIndices.size();
}

const u16* MeshBatchData::Get16BitIndices() const
{
	assert(m_IndexFormat == DXGI_FORMAT_R16_UINT);
	return &m_16BitIndices[0];
}

const u32* MeshBatchData::Get32BitIndices() const
{
	assert(m_IndexFormat == DXGI_FORMAT_R32_UINT);
	return &m_32BitIndices[0];
}
