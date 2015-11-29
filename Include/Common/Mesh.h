#pragma once

#include "Common/Common.h"

class DXDevice;
class DXResource;
class DXCommandList;
struct DXVertexBufferView;
struct DXIndexBufferView;

class MeshData;
struct SubMeshData;

class Mesh
{
public:
	Mesh(DXDevice* pDevice, const MeshData* pMeshData);
	~Mesh();

	void RecordDataForUpload(DXCommandList* pCommandList);
	void RemoveDataForUpload();
	
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
	
	u32 m_NumSubMeshes;
	SubMeshData* m_pSubMeshes;
};