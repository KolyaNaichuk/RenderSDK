#pragma once

/*
http://docs.unity3d.com/Manual/SL-VertexFragmentShaderExamples.html
http://docs.unity3d.com/Manual/SL-VertexProgramInputs.html
*/

#include "Common/Common.h"

class DXDevice;
class DXResource;
class DXCommandList;
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
	Mesh(DXDevice* pDevice, const MeshData* pMeshData);
	~Mesh();

	void RecordDataForUpload(DXCommandList* pCommandList);
	void RemoveDataForUpload();
	
	u8 GetVertexElementFlags() const;

	DXResource* GetVertexBuffer();
	DXVertexBufferView* GetVertexBufferView();
	
	DXResource* GetIndexBuffer();
	DXIndexBufferView* GetIndexBufferView();
	
	u32 GetNumSubMeshes() const;
	const SubMeshData* GetSubMeshes() const;
		
private:
	void InitVertexBuffer(DXDevice* pDevice, const MeshData* pMeshData);
	void InitIndexBuffer(DXDevice* pDevice, const MeshData* pMeshData);
	void InitSubMeshes(const MeshData* pMeshData);

private:
	DXResource* m_pUploadHeapVB;
	DXResource* m_pUploadHeapIB;

	DXResource* m_pDefaultHeapVB;	
	DXResource* m_pDefaultHeapIB;
	
	DXVertexBufferView* m_pVBView;
	DXIndexBufferView* m_pIBView;

	u8 m_VertexElementFlags;
	
	u32 m_NumSubMeshes;
	SubMeshData* m_pSubMeshes;
};