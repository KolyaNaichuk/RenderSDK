#pragma once

#include "DX/DXPipelineState.h"

class DXBuffer;
class DXCommandList;

struct DXRenderEnvironment;
struct DXVertexBufferView;
struct DXIndexBufferView;
struct DXInputLayoutDesc;

class MeshData;
struct SubMeshData;

enum VertexElementFlags
{
	VertexElementFlag_Position = 1,
	VertexElementFlag_Normal = 2,
	VertexElementFlag_Color = 4,
	VertexElementFlag_Tangent = 8,
	VertexElementFlag_BiTangent = 16,
	VertexElementFlag_TexCoords = 32
};

class Mesh
{
public:
	Mesh(DXRenderEnvironment* pEnv, const MeshData* pMeshData);
	~Mesh();

	void RecordDataForUpload(DXCommandList* pCommandList);
	void RemoveDataForUpload();
	
	u8 GetVertexElementFlags() const;

	DXBuffer* GetVertexBuffer();
	DXBuffer* GetIndexBuffer();
	
	u32 GetNumSubMeshes() const;
	const SubMeshData* GetSubMeshes() const;
		
private:
	void InitVertexBuffer(DXRenderEnvironment* pEnv, const MeshData* pMeshData);
	void InitIndexBuffer(DXRenderEnvironment* pEnv, const MeshData* pMeshData);
	void InitSubMeshes(const MeshData* pMeshData);

private:
	DXBuffer* m_pUploadHeapVB;
	DXBuffer* m_pUploadHeapIB;

	DXBuffer* m_pDefaultHeapVB;	
	DXBuffer* m_pDefaultHeapIB;
		
	u8 m_VertexElementFlags;
	
	u32 m_NumSubMeshes;
	SubMeshData* m_pSubMeshes;
};

void GenerateInputElements(std::vector<DXInputElementDesc>& inputElements, u8 inputElementFlags, u8 vertexElementFlags);