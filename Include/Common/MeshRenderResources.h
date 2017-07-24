#pragma once

#include "D3DWrapper/PipelineState.h"

class Buffer;
class CommandList;
class MeshBatch;

struct InputLayoutDesc;
struct RenderEnv;

class MeshRenderResources
{
public:
	MeshRenderResources(RenderEnv* pRenderEnv, u32 numMeshTypes, MeshBatch** ppFirstMeshType);
	~MeshRenderResources();

	u32 GetNumMeshTypes() const { return m_NumMeshTypes; }

	Buffer* GetMeshInfoBuffer() { return m_pMeshInfoBuffer; }
	Buffer* GetMeshInstanceRangeBuffer() { return m_pMeshInstanceRangeBuffer; }

	Buffer* GetInstanceWorldMatrixBuffer() { return m_pInstanceWorldMatrixBuffer; }
	Buffer* GetInstanceWorldAABBBuffer() { return m_pInstanceWorldAABBBuffer; }

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

	Buffer* m_pMeshInfoBuffer;
	Buffer* m_pMeshInstanceRangeBuffer;

	Buffer* m_pInstanceWorldMatrixBuffer;
	Buffer* m_pInstanceWorldAABBBuffer;

	using InputElements = std::vector<InputElementDesc>;
	
	std::vector<u32> m_VertexStrideInBytes;
	std::vector<InputElements> m_InputElements;
	std::vector<InputLayoutDesc> m_InputLayouts;
	std::vector<D3D12_PRIMITIVE_TOPOLOGY_TYPE> m_PrimitiveTopologyTypes;
	std::vector<D3D12_PRIMITIVE_TOPOLOGY> m_PrimitiveTopologies;
	std::vector<Buffer*> m_VertexBuffers;
	std::vector<Buffer*> m_IndexBuffers;
};
