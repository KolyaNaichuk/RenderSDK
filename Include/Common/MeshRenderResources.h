#pragma once

#include "D3DWrapper/PipelineState.h"

class Buffer;
class MeshBatch;
struct RenderEnv;

struct MeshRenderInfo
{
	MeshRenderInfo(u32 numInstances, u32 instanceOffset, u32 meshType, u32 meshTypeOffset, u32 materialIndex, u32 indexCountPerInstance, u32 startIndexLocation, i32 baseVertexLocation)
		: m_NumInstances(numInstances)
		, m_InstanceOffset(instanceOffset)
		, m_MeshType(meshType)
		, m_MeshTypeOffset(meshTypeOffset)
		, m_MaterialIndex(materialIndex)
		, m_IndexCountPerInstance(indexCountPerInstance)
		, m_StartIndexLocation(startIndexLocation)
		, m_BaseVertexLocation(baseVertexLocation)
	{}
	u32 m_NumInstances;
	u32 m_InstanceOffset;
	u32 m_MeshType;
	u32 m_MeshTypeOffset;
	u32 m_MaterialIndex;
	u32 m_IndexCountPerInstance;
	u32 m_StartIndexLocation;
	i32 m_BaseVertexLocation;
};

class MeshRenderResources
{
public:
	MeshRenderResources(RenderEnv* pRenderEnv, u32 numMeshTypes, MeshBatch** ppFirstMeshType);
	~MeshRenderResources();

	u32 GetNumMeshTypes() const { return m_NumMeshTypes; }
	u32 GetTotalNumMeshes() const { return m_TotalNumMeshes; }
	u32 GetTotalNumInstances() const { return m_TotalNumInstances; }
	u32 GetMaxNumInstancesPerMesh() const { return m_MaxNumInstancesPerMesh; }
		
	Buffer* GetMeshInfoBuffer() { return m_pMeshInfoBuffer; }
	Buffer* GetInstanceWorldMatrixBuffer() { return m_pInstanceWorldMatrixBuffer; }
	Buffer* GetInstanceWorldAABBBuffer() { return m_pInstanceWorldAABBBuffer; }
	Buffer* GetInstanceWorldOBBMatrixBuffer() { return m_pInstanceWorldOBBMatrixBuffer; }

	u32 GetMeshTypeOffset(u32 meshType) const { return m_MeshTypeOffsets[meshType]; }
	const InputLayoutDesc& GetInputLayout(u32 meshType) const { return m_InputLayouts[meshType]; }
	D3D12_PRIMITIVE_TOPOLOGY_TYPE GetPrimitiveTopologyType(u32 meshType) const { return m_PrimitiveTopologyTypes[meshType]; }
	D3D12_PRIMITIVE_TOPOLOGY GetPrimitiveTopology(u32 meshType) const { return m_PrimitiveTopologies[meshType]; }
	Buffer* GetVertexBuffer(u32 meshType) { return m_VertexBuffers[meshType]; }
	Buffer* GetIndexBuffer(u32 meshType) { return m_IndexBuffers[meshType]; }

private:
	void InitPerMeshResources(RenderEnv* pRenderEnv, u32 numMeshTypes, MeshBatch** ppFirstMeshType);
	void InitPerMeshInstanceResources(RenderEnv* pRenderEnv, u32 numMeshTypes, MeshBatch** ppFirstMeshType);
	void InitPerMeshTypeResources(RenderEnv* pRenderEnv, u32 numMeshTypes, MeshBatch** ppFirstMeshType);

	void InitInputLayout(RenderEnv* pRenderEnv, u32 meshType, const MeshBatch* pMeshBatch);
	void InitVertexBuffer(RenderEnv* pRenderEnv, u32 meshType, const MeshBatch* pMeshBatch);
	void InitIndexBuffer(RenderEnv* pRenderEnv, u32 meshType, const MeshBatch* pMeshBatch);

private:
	u32 m_NumMeshTypes;
	u32 m_TotalNumMeshes;
	u32 m_TotalNumInstances;
	u32 m_MaxNumInstancesPerMesh;

	Buffer* m_pMeshInfoBuffer;
	Buffer* m_pInstanceWorldMatrixBuffer;
	Buffer* m_pInstanceWorldAABBBuffer;
	Buffer* m_pInstanceWorldOBBMatrixBuffer;

	using InputElements = std::vector<InputElementDesc>;
	
	std::vector<u32> m_MeshTypeOffsets;
	std::vector<u32> m_VertexStrideInBytes;
	std::vector<InputElements> m_InputElements;
	std::vector<InputLayoutDesc> m_InputLayouts;
	std::vector<D3D12_PRIMITIVE_TOPOLOGY_TYPE> m_PrimitiveTopologyTypes;
	std::vector<D3D12_PRIMITIVE_TOPOLOGY> m_PrimitiveTopologies;
	std::vector<Buffer*> m_VertexBuffers;
	std::vector<Buffer*> m_IndexBuffers;
};
