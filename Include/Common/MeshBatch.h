#pragma once

#include "D3DWrapper/PipelineState.h"

class Buffer;
class CommandList;

struct InputLayoutDesc;
struct RenderEnv;

class MeshBatchData;

class MeshBatch
{
public:
	MeshBatch(RenderEnv* pRenderEnv, const MeshBatchData* pBatchData);
	~MeshBatch();

	void RecordDataForUpload(CommandList* pCommandList);
	void RemoveDataForUpload();

	u32 GetNumMeshes() const { return m_NumMeshes; }
	
	InputLayoutDesc* GetInputLayout() { return m_pInputLayout; }
	D3D12_PRIMITIVE_TOPOLOGY_TYPE GetPrimitiveTopologyType() const { return m_PrimitiveTopologyType; }
	D3D12_PRIMITIVE_TOPOLOGY GetPrimitiveTopology() const { return m_PrimitiveTopology; }

	Buffer* GetVertexBuffer() { return m_pVertexBuffer; }
	Buffer* GetIndexBuffer() { return m_pIndexBuffer; }
	Buffer* GetMeshBoundsBuffer() { return m_pMeshBoundsBuffer; }
	Buffer* GetMeshDescBuffer() { return m_pMeshDescBuffer; }
	Buffer* GetMaterialBuffer() { return m_pMaterialBuffer; }
					
private:
	void InitInputLayout(RenderEnv* pRenderEnv, const MeshBatchData* pBatchData);
	void InitVertexBuffer(RenderEnv* pRenderEnv, const MeshBatchData* pBatchData);
	void InitIndexBuffer(RenderEnv* pRenderEnv, const MeshBatchData* pBatchData);
	void InitMeshBoundsBuffer(RenderEnv* pRenderEnv, const MeshBatchData* pBatchData);
	void InitMeshDescBuffer(RenderEnv* pRenderEnv, const MeshBatchData* pBatchData);
	void InitMaterialBuffer(RenderEnv* pRenderEnv, const MeshBatchData* pBatchData);
	
private:
	u32 m_NumMeshes;

	u32 m_VertexStrideInBytes;
	std::vector<InputElementDesc> m_InputElements;
	InputLayoutDesc* m_pInputLayout;
	D3D12_PRIMITIVE_TOPOLOGY_TYPE m_PrimitiveTopologyType;
	D3D12_PRIMITIVE_TOPOLOGY m_PrimitiveTopology;

	Buffer* m_pUploadVertexBuffer;
	Buffer* m_pUploadIndexBuffer;
	Buffer* m_pUploadMeshBoundsBuffer;
	Buffer* m_pUploadMeshDescBuffer;
	Buffer* m_pUploadMaterialBuffer;

	Buffer* m_pVertexBuffer;	
	Buffer* m_pIndexBuffer;
	Buffer* m_pMeshBoundsBuffer;
	Buffer* m_pMeshDescBuffer;
	Buffer* m_pMaterialBuffer;
};
