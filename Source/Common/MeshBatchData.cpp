#include "Common/MeshBatchData.h"
#include "Common/MeshData.h"

MeshBatchData::MeshBatchData(u8 vertexFormatFlags, DXGI_FORMAT indexFormat, D3D12_PRIMITIVE_TOPOLOGY_TYPE primitiveTopologyType, D3D12_PRIMITIVE_TOPOLOGY primitiveTopology)
	: m_VertexFormatFlags(vertexFormatFlags)
	, m_IndexFormat(indexFormat)
	, m_PrimitiveTopologyType(primitiveTopologyType)
	, m_PrimitiveTopology(primitiveTopology)
{
	assert((m_IndexFormat == DXGI_FORMAT_R16_UINT) || (m_IndexFormat == DXGI_FORMAT_R32_UINT));
}

void MeshBatchData::AddMeshData(const MeshData* pMeshData)
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
		m_Positions.insert(m_Positions.end(),
			pVertexData->GetPositions(),
			pVertexData->GetPositions() + numVertices);
	}	
	if ((m_VertexFormatFlags & VertexData::FormatFlag_Normal) != 0)
	{
		m_Normals.insert(m_Normals.end(),
			pVertexData->GetNormals(),
			pVertexData->GetNormals() + numVertices);
	}
	if ((m_VertexFormatFlags & VertexData::FormatFlag_TexCoords) != 0)
	{
		m_TexCoords.insert(m_TexCoords.end(),
			pVertexData->GetTexCoords(),
			pVertexData->GetTexCoords() + numVertices);
	}
	if ((m_VertexFormatFlags & VertexData::FormatFlag_Color) != 0)
	{
		m_Colors.insert(m_Colors.end(),
			pVertexData->GetColors(),
			pVertexData->GetColors() + numVertices);
	}
	if ((m_VertexFormatFlags & VertexData::FormatFlag_Tangent) != 0)
	{
		m_Tangents.insert(m_Tangents.end(),
			pVertexData->GetTangents(),
			pVertexData->GetTangents() + numVertices);
	}
	
	const u32 startIndexLocation = GetNumIndices();
	const u32 numIndices = pIndexData->GetNumIndices();

	if (m_IndexFormat == DXGI_FORMAT_R16_UINT)
	{
		m_16BitIndices.insert(m_16BitIndices.end(),
			pIndexData->Get16BitIndices(),
			pIndexData->Get16BitIndices() + numIndices);
	}
	else
	{
		m_32BitIndices.insert(m_32BitIndices.end(),
			pIndexData->Get32BitIndices(),
			pIndexData->Get32BitIndices() + numIndices);
	}

	const u32 numInstances = pMeshData->GetNumInstances();
	const u32 instanceOffset = GetNumMeshInstances();
	
	m_MeshInfos.emplace_back(numInstances,
		instanceOffset,
		numIndices,
		startIndexLocation,
		baseVertexLocation,
		pMeshData->GetMaterialIndex());
	
	m_MeshInstanceWorldAABBs.insert(m_MeshInstanceWorldAABBs.end(),
		pMeshData->GetInstanceWorldAABBs(),
		pMeshData->GetInstanceWorldAABBs() + numInstances);
	
	m_MeshInstanceWorldMatrices.insert(m_MeshInstanceWorldMatrices.end(),
		pMeshData->GetInstanceWorldMatrices(),
		pMeshData->GetInstanceWorldMatrices() + numInstances);
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
