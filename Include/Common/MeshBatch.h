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
	
	DXInputLayoutDesc* GetInputLayout() { return m_pInputLayout; }
	D3D12_PRIMITIVE_TOPOLOGY_TYPE GetPrimitiveTopologyType() const { return m_PrimitiveTopologyType; }
	D3D12_PRIMITIVE_TOPOLOGY GetPrimitiveTopology() const { return m_PrimitiveTopology; }

	DXBuffer* GetVertexBuffer() { return m_pDefaultHeapVB; }
	DXBuffer* GetIndexBuffer() { return m_pDefaultHeapIB; }
					
private:
	void InitVertexBuffer(DXRenderEnvironment* pEnv, const MeshBatchData* pBatchData);
	void InitIndexBuffer(DXRenderEnvironment* pEnv, const MeshBatchData* pBatchData);
	
private:
	DXBuffer* m_pUploadHeapVB;
	DXBuffer* m_pUploadHeapIB;

	DXBuffer* m_pDefaultHeapVB;	
	DXBuffer* m_pDefaultHeapIB;

	DXInputLayoutDesc* m_pInputLayout;
	D3D12_PRIMITIVE_TOPOLOGY_TYPE m_PrimitiveTopologyType;
	D3D12_PRIMITIVE_TOPOLOGY m_PrimitiveTopology;
};
