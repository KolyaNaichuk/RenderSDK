#pragma once

#include "Math/Vector2.h"
#include "Math/Vector3.h"
#include "Math/Vector4.h"
#include "Math/Matrix4.h"
#include "Math/AxisAlignedBox.h"
#include "D3DWrapper/Common.h"

class Mesh;

struct MeshInfo
{
	MeshInfo(u32 instanceCount, u32 instanceOffset, u32 indexCountPerInstance, u32 startIndexLocation, i32 baseVertexLocation, u32 materialIndex)
		: m_InstanceCount(instanceCount)
		, m_InstanceOffset(instanceOffset)
		, m_IndexCountPerInstance(indexCountPerInstance)
		, m_StartIndexLocation(startIndexLocation)
		, m_BaseVertexLocation(baseVertexLocation)
		, m_MaterialIndex(materialIndex)
	{}
	u32 m_InstanceCount;
	u32 m_InstanceOffset;
	u32 m_IndexCountPerInstance;
	u32 m_StartIndexLocation;
	i32 m_BaseVertexLocation;
	u32 m_MaterialIndex;
};

class MeshBatch
{
public:
	MeshBatch(u8 vertexFormatFlags, DXGI_FORMAT indexFormat, D3D12_PRIMITIVE_TOPOLOGY_TYPE primitiveTopologyType, D3D12_PRIMITIVE_TOPOLOGY primitiveTopology);

	void AddMesh(const Mesh* pMesh);

	u8 GetVertexFormatFlags() const { return m_VertexFormatFlags; }
	DXGI_FORMAT GetIndexFormat() const { return m_IndexFormat; }

	D3D12_PRIMITIVE_TOPOLOGY_TYPE GetPrimitiveTopologyType() const { return m_PrimitiveTopologyType; }
	D3D12_PRIMITIVE_TOPOLOGY GetPrimitiveTopology() const { return m_PrimitiveTopology; }

	u32 GetNumMeshes() const { return m_MeshInfos.size(); }
	const MeshInfo* GetMeshInfos() const { return m_MeshInfos.data(); }

	u32 GetNumMeshInstances() const { return m_MeshInstanceWorldAABBs.size(); }
	const AxisAlignedBox* GetMeshInstanceWorldAABBs() const { return m_MeshInstanceWorldAABBs.data(); }
	const Matrix4f* GetMeshInstanceWorldMatrices() const { return m_MeshInstanceWorldMatrices.data(); }

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

	std::vector<MeshInfo> m_MeshInfos;
	std::vector<AxisAlignedBox> m_MeshInstanceWorldAABBs;
	std::vector<Matrix4f> m_MeshInstanceWorldMatrices;
};