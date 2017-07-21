#pragma once

#include "D3DWrapper/PipelineState.h"

class Buffer;
class CommandList;
class MeshBatchData;

struct InputLayoutDesc;
struct RenderEnv;

class MeshBatch
{
public:
	MeshBatch(RenderEnv* pRenderEnv, const MeshBatchData* pBatchData, u32 meshType, u32 meshTypeOffset);
	~MeshBatch();

	void RecordDataForUpload(CommandList* pCommandList);
	void RemoveDataForUpload();

	u32 GetNumMeshes() const { return m_NumMeshes; }
	
	InputLayoutDesc* GetInputLayout() { return m_pInputLayout; }
	D3D12_PRIMITIVE_TOPOLOGY_TYPE GetPrimitiveTopologyType() const { return m_PrimitiveTopologyType; }
	D3D12_PRIMITIVE_TOPOLOGY GetPrimitiveTopology() const { return m_PrimitiveTopology; }

	Buffer* GetVertexBuffer() { return m_pVertexBuffer; }
	Buffer* GetIndexBuffer() { return m_pIndexBuffer; }
	Buffer* GetMeshInfoBuffer() { return m_pMeshInfoBuffer; }
	Buffer* GetInstanceAABBBuffer() { return m_pInstanceAABBBuffer; }

private:
	void InitInputLayout(RenderEnv* pRenderEnv, const MeshBatchData* pBatchData);
	void InitVertexBuffer(RenderEnv* pRenderEnv, const MeshBatchData* pBatchData);
	void InitIndexBuffer(RenderEnv* pRenderEnv, const MeshBatchData* pBatchData);
	void InitMeshInfoBuffer(RenderEnv* pRenderEnv, const MeshBatchData* pBatchData, u32 meshType, u32 meshTypeOffset);
	void InitInstanceAABBBuffer(RenderEnv* pRenderEnv, const MeshBatchData* pBatchData);
		
private:
	u32 m_NumMeshes;

	u32 m_VertexStrideInBytes;
	std::vector<InputElementDesc> m_InputElements;
	InputLayoutDesc* m_pInputLayout;
	D3D12_PRIMITIVE_TOPOLOGY_TYPE m_PrimitiveTopologyType;
	D3D12_PRIMITIVE_TOPOLOGY m_PrimitiveTopology;

	Buffer* m_pUploadVertexBuffer;
	Buffer* m_pUploadIndexBuffer;
	Buffer* m_pUploadMeshInfoBuffer;
	Buffer* m_pUploadInstanceAABBBuffer;

	Buffer* m_pVertexBuffer;	
	Buffer* m_pIndexBuffer;
	Buffer* m_pMeshInfoBuffer;
	Buffer* m_pInstanceAABBBuffer;
};

class MeshRenderResources
{
public:
	MeshRenderResources(RenderEnv* pRenderEnv, u32 numMeshTypes, const MeshBatchData* pFirstMeshTypeData);
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
	void InitPerMeshResources(RenderEnv* pRenderEnv, u32 numMeshTypes, const MeshBatchData* pFirstMeshTypeData);
	void InitPerMeshInstanceResources(RenderEnv* pRenderEnv, u32 numMeshTypes, const MeshBatchData* pFirstMeshTypeData);
	void InitPerMeshTypeResources(RenderEnv* pRenderEnv, u32 numMeshTypes, const MeshBatchData* pFirstMeshTypeData);

	void InitInputLayout(RenderEnv* pRenderEnv, u32 meshType, const MeshBatchData& batchData);
	void InitVertexBuffer(RenderEnv* pRenderEnv, u32 meshType, const MeshBatchData& batchData);
	void InitIndexBuffer(RenderEnv* pRenderEnv, u32 meshType, const MeshBatchData& batchData);

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

	std::vector<Buffer*> m_UploadVertexBuffers;
	std::vector<Buffer*> m_UploadIndexBuffers;
};
