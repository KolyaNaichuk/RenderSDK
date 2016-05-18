#pragma once

#include "Math/Vector2.h"
#include "Math/Vector3.h"
#include "Math/Vector4.h"

class MeshData;

struct BatchMeshData
{
	BatchMeshData(u32 numIndices, u32 startIndexLocation, i32 baseVertexLocation, u32 materialIndex);

	u32 m_NumIndices;
	u32 m_StartIndexLocation;
	i32 m_BaseVertexLocation;
	u32 m_MaterialIndex;
};

class MeshBatchData
{
public:
	MeshBatchData(u8 vertexFormatFlags, DXGI_FORMAT indexFormat, D3D12_PRIMITIVE_TOPOLOGY_TYPE primitiveTopologyType, D3D12_PRIMITIVE_TOPOLOGY primitiveTopology);

	void Append(const MeshData* pMeshData);

	u8 GetVertexFormatFlags() const { return m_VertexFormatFlags; }
	DXGI_FORMAT GetIndexFormat() const { return m_IndexFormat; }

	D3D12_PRIMITIVE_TOPOLOGY_TYPE GetPrimitiveTopologyType() const { return m_PrimitiveTopologyType; }
	D3D12_PRIMITIVE_TOPOLOGY GetPrimitiveTopology() const { return m_PrimitiveTopology; }
		
	u32 GetNumMeshes() { return m_BatchMeshes.size(); }
	const BatchMeshData* GetMeshData(u32 meshIndex) const { return &m_BatchMeshes[meshIndex]; }
	
	u32 GetNumVertices() const;
	const Vector3f* GetPositions() const;
	const Vector3f* GetNormals() const;
	const Vector2f* GetTexCoords() const;
	const Vector4f* GetColors() const;
	const Vector3f* GetTangents() const;

	u32 GetNumIndices() const;
	const u16* Get16BitIndices() const;
	const u32* Get32BitIndices() const;

private:
	u8 m_VertexFormatFlags;
	DXGI_FORMAT m_IndexFormat;

	D3D12_PRIMITIVE_TOPOLOGY_TYPE m_PrimitiveTopologyType;
	D3D12_PRIMITIVE_TOPOLOGY m_PrimitiveTopology;
	
	std::vector<Vector3f> m_Positions;
	std::vector<Vector3f> m_Normals;
	std::vector<Vector2f> m_TexCoords;
	std::vector<Vector4f> m_Colors;
	std::vector<Vector3f> m_Tangents;
	std::vector<u16> m_16BitIndices;
	std::vector<u32> m_32BitIndices;

	std::vector<BatchMeshData> m_BatchMeshes;
};