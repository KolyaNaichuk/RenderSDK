#pragma once

#include "D3DWrapper/D3DPipelineState.h"

class D3DBuffer;
class D3DCommandList;

struct D3DInputLayoutDesc;
struct D3DRenderEnv;

class MeshBatchData;

class MeshBatch
{
public:
	MeshBatch(D3DRenderEnv* pRenderEnv, const MeshBatchData* pBatchData);
	~MeshBatch();

	void RecordDataForUpload(D3DCommandList* pCommandList);
	void RemoveDataForUpload();

	u32 GetNumMeshes() const { return m_NumMeshes; }
	
	D3DInputLayoutDesc* GetInputLayout() { return m_pInputLayout; }
	D3D12_PRIMITIVE_TOPOLOGY_TYPE GetPrimitiveTopologyType() const { return m_PrimitiveTopologyType; }
	D3D12_PRIMITIVE_TOPOLOGY GetPrimitiveTopology() const { return m_PrimitiveTopology; }

	D3DBuffer* GetVertexBuffer() { return m_pVertexBuffer; }
	D3DBuffer* GetIndexBuffer() { return m_pIndexBuffer; }
	D3DBuffer* GetMeshBoundsBuffer() { return m_pMeshBoundsBuffer; }
	D3DBuffer* GetMeshDescBuffer() { return m_pMeshDescBuffer; }
	D3DBuffer* GetMaterialBuffer() { return m_pMaterialBuffer; }
					
private:
	void InitInputLayout(D3DRenderEnv* pRenderEnv, const MeshBatchData* pBatchData);
	void InitVertexBuffer(D3DRenderEnv* pRenderEnv, const MeshBatchData* pBatchData);
	void InitIndexBuffer(D3DRenderEnv* pRenderEnv, const MeshBatchData* pBatchData);
	void InitMeshBoundsBuffer(D3DRenderEnv* pRenderEnv, const MeshBatchData* pBatchData);
	void InitMeshDescBuffer(D3DRenderEnv* pRenderEnv, const MeshBatchData* pBatchData);
	void InitMaterialBuffer(D3DRenderEnv* pRenderEnv, const MeshBatchData* pBatchData);
	
private:
	u32 m_NumMeshes;

	u32 m_VertexStrideInBytes;
	std::vector<D3DInputElementDesc> m_InputElements;
	D3DInputLayoutDesc* m_pInputLayout;
	D3D12_PRIMITIVE_TOPOLOGY_TYPE m_PrimitiveTopologyType;
	D3D12_PRIMITIVE_TOPOLOGY m_PrimitiveTopology;

	D3DBuffer* m_pUploadVertexBuffer;
	D3DBuffer* m_pUploadIndexBuffer;
	D3DBuffer* m_pUploadMeshBoundsBuffer;
	D3DBuffer* m_pUploadMeshDescBuffer;
	D3DBuffer* m_pUploadMaterialBuffer;

	D3DBuffer* m_pVertexBuffer;	
	D3DBuffer* m_pIndexBuffer;
	D3DBuffer* m_pMeshBoundsBuffer;
	D3DBuffer* m_pMeshDescBuffer;
	D3DBuffer* m_pMaterialBuffer;
};
