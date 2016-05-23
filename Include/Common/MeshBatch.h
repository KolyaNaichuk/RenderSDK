#pragma once

#include "DX/DXPipelineState.h"

class DXBuffer;
class DXCommandList;

struct DXInputLayoutDesc;
struct DXRenderEnvironment;

class MeshBatchData;

class MeshBatch
{
public:
	MeshBatch(DXRenderEnvironment* pEnv, const MeshBatchData* pBatchData);
	~MeshBatch();

	void RecordDataForUpload(DXCommandList* pCommandList);
	void RemoveDataForUpload();

	u32 GetNumMeshes() const { return m_NumMeshes; }
	
	DXInputLayoutDesc* GetInputLayout() { return m_pInputLayout; }
	D3D12_PRIMITIVE_TOPOLOGY_TYPE GetPrimitiveTopologyType() const { return m_PrimitiveTopologyType; }
	D3D12_PRIMITIVE_TOPOLOGY GetPrimitiveTopology() const { return m_PrimitiveTopology; }

	DXBuffer* GetVertexBuffer() { return m_pVertexBuffer; }
	DXBuffer* GetIndexBuffer() { return m_pIndexBuffer; }
	DXBuffer* GetMeshBoundsBuffer() { return m_pMeshBoundsBuffer; }
	DXBuffer* GetMeshDescBuffer() { return m_pMeshDescBuffer; }
	DXBuffer* GetMaterialBuffer() { return m_pMaterialBuffer; }
					
private:
	void InitInputLayout(DXRenderEnvironment* pEnv, const MeshBatchData* pBatchData);
	void InitVertexBuffer(DXRenderEnvironment* pEnv, const MeshBatchData* pBatchData);
	void InitIndexBuffer(DXRenderEnvironment* pEnv, const MeshBatchData* pBatchData);
	void InitMeshBoundsBuffer(DXRenderEnvironment* pEnv, const MeshBatchData* pBatchData);
	void InitMeshDescBuffer(DXRenderEnvironment* pEnv, const MeshBatchData* pBatchData);
	void InitMaterialBuffer(DXRenderEnvironment* pEnv, const MeshBatchData* pBatchData);
	
private:
	u32 m_NumMeshes;

	u32 m_VertexStrideInBytes;
	std::vector<DXInputElementDesc> m_InputElements;
	DXInputLayoutDesc* m_pInputLayout;
	D3D12_PRIMITIVE_TOPOLOGY_TYPE m_PrimitiveTopologyType;
	D3D12_PRIMITIVE_TOPOLOGY m_PrimitiveTopology;

	DXBuffer* m_pUploadVertexBuffer;
	DXBuffer* m_pUploadIndexBuffer;
	DXBuffer* m_pUploadMeshBoundsBuffer;
	DXBuffer* m_pUploadMeshDescBuffer;
	DXBuffer* m_pUploadMaterialBuffer;

	DXBuffer* m_pVertexBuffer;	
	DXBuffer* m_pIndexBuffer;
	DXBuffer* m_pMeshBoundsBuffer;
	DXBuffer* m_pMeshDescBuffer;
	DXBuffer* m_pMaterialBuffer;
};
