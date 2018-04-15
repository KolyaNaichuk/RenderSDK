#include "Scene/MeshBatch.h"
#include "Scene/Mesh.h"
#include "Math/Math.h"

MeshBatch::MeshBatch(u8 vertexFormatFlags, DXGI_FORMAT indexFormat, D3D12_PRIMITIVE_TOPOLOGY_TYPE primitiveTopologyType, D3D12_PRIMITIVE_TOPOLOGY primitiveTopology)
	: m_VertexFormatFlags(vertexFormatFlags)
	, m_IndexFormat(indexFormat)
	, m_PrimitiveTopologyType(primitiveTopologyType)
	, m_PrimitiveTopology(primitiveTopology)
	, m_MaxNumInstancesPerMesh(0)
{
	assert((m_IndexFormat == DXGI_FORMAT_R16_UINT) || (m_IndexFormat == DXGI_FORMAT_R32_UINT));
}

void MeshBatch::AddMesh(const Mesh* pMesh)
{
	m_MaxNumInstancesPerMesh = Max(m_MaxNumInstancesPerMesh, pMesh->GetNumInstances());

	assert(m_PrimitiveTopology == pMesh->GetPrimitiveTopology());
	assert(m_PrimitiveTopologyType == pMesh->GetPrimitiveTopologyType());

	const VertexData* pVertexData = pMesh->GetVertexData();
	assert(pVertexData != nullptr);
	assert(m_VertexFormatFlags == (m_VertexFormatFlags & pVertexData->GetFormatFlags()));

	const IndexData* pIndexData = pMesh->GetIndexData();
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

	const u32 numInstances = pMesh->GetNumInstances();
	const u32 instanceOffset = GetNumMeshInstances();
	
	m_MeshInfos.emplace_back(numInstances,
		instanceOffset,
		numIndices,
		startIndexLocation,
		baseVertexLocation,
		pMesh->GetMaterialID());
	
	m_MeshInstanceWorldAABBs.insert(m_MeshInstanceWorldAABBs.end(),
		pMesh->GetInstanceWorldAABBs(),
		pMesh->GetInstanceWorldAABBs() + numInstances);

	m_MeshInstanceWorldOBBs.insert(m_MeshInstanceWorldOBBs.end(),
		pMesh->GetInstanceWorldOBBs(),
		pMesh->GetInstanceWorldOBBs() + numInstances);
	
	m_MeshInstanceWorldMatrices.insert(m_MeshInstanceWorldMatrices.end(),
		pMesh->GetInstanceWorldMatrices(),
		pMesh->GetInstanceWorldMatrices() + numInstances);
}

u32 MeshBatch::GetNumVertices() const
{
	return m_Positions.size();
}

const Vector3f* MeshBatch::GetPositions() const
{
	assert((m_VertexFormatFlags & VertexData::FormatFlag_Position) != 0);
	return &m_Positions[0];
}

const Vector3f* MeshBatch::GetNormals() const
{
	assert((m_VertexFormatFlags & VertexData::FormatFlag_Normal) != 0);
	return &m_Normals[0];
}

const Vector2f* MeshBatch::GetTexCoords() const
{
	assert((m_VertexFormatFlags & VertexData::FormatFlag_TexCoords) != 0);
	return &m_TexCoords[0];
}

const Vector4f* MeshBatch::GetColors() const
{
	assert((m_VertexFormatFlags & VertexData::FormatFlag_Color) != 0);
	return &m_Colors[0];
}

const Vector3f* MeshBatch::GetTangents() const
{
	assert((m_VertexFormatFlags & VertexData::FormatFlag_Tangent) != 0);
	return &m_Tangents[0];
}

u32 MeshBatch::GetNumIndices() const
{
	if (m_IndexFormat == DXGI_FORMAT_R16_UINT)
		return m_16BitIndices.size();
	return m_32BitIndices.size();
}

const u16* MeshBatch::Get16BitIndices() const
{
	assert(m_IndexFormat == DXGI_FORMAT_R16_UINT);
	return &m_16BitIndices[0];
}

const u32* MeshBatch::Get32BitIndices() const
{
	assert(m_IndexFormat == DXGI_FORMAT_R32_UINT);
	return &m_32BitIndices[0];
}
