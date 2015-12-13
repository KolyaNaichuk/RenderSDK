#include "Common/Mesh.h"
#include "Common/MeshData.h"
#include "DX/DXResource.h"
#include "DX/DXCommandList.h"
#include "DX/DXPipelineState.h"
#include "Math/Vector2f.h"
#include "Math/Vector3f.h"

namespace
{
	u8 ComputeVertexElementFlags(const MeshData* pMeshData);
}

Mesh::Mesh(DXDevice* pDevice, const MeshData* pMeshData)
	: m_pUploadHeapVB(nullptr)
	, m_pUploadHeapIB(nullptr)
	, m_pDefaultHeapVB(nullptr)
	, m_pDefaultHeapIB(nullptr)
	, m_pVBView(nullptr)
	, m_pIBView(nullptr)
	, m_VertexElementFlags(ComputeVertexElementFlags(pMeshData))
	, m_NumSubMeshes(0)
	, m_pSubMeshes(nullptr)
{
	InitVertexBuffer(pDevice, pMeshData);
	InitIndexBuffer(pDevice, pMeshData);
	InitSubMeshes(pMeshData);
}

Mesh::~Mesh()
{
	SafeDelete(m_pUploadHeapVB);
	SafeDelete(m_pUploadHeapIB);
	SafeDelete(m_pDefaultHeapVB);
	SafeDelete(m_pDefaultHeapIB);
	SafeDelete(m_pVBView);
	SafeDelete(m_pIBView);
	SafeArrayDelete(m_pSubMeshes);
}

void Mesh::RecordDataForUpload(DXCommandList* pCommandList)
{
	pCommandList->CopyResource(m_pDefaultHeapVB, m_pUploadHeapVB);
	pCommandList->TransitionBarrier(m_pDefaultHeapVB, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);

	pCommandList->CopyResource(m_pDefaultHeapIB, m_pUploadHeapIB);
	pCommandList->TransitionBarrier(m_pDefaultHeapIB, D3D12_RESOURCE_STATE_INDEX_BUFFER);
}

void Mesh::RemoveDataForUpload()
{
	SafeDelete(m_pUploadHeapVB);
	SafeDelete(m_pUploadHeapIB);
}

u16 Mesh::GetVertexElementFlags() const
{
	return m_VertexElementFlags;
}

DXResource* Mesh::GetVertexBuffer()
{
	return m_pDefaultHeapVB;
}

DXVertexBufferView* Mesh::GetVertexBufferView()
{
	return m_pVBView;
}

DXResource* Mesh::GetIndexBuffer()
{
	return m_pDefaultHeapIB;
}

DXIndexBufferView* Mesh::GetIndexBufferView()
{
	return m_pIBView;
}

u32 Mesh::GetNumSubMeshes() const
{
	return m_NumSubMeshes;
}

const SubMeshData* Mesh::GetSubMeshes() const
{
	return m_pSubMeshes;
}

void Mesh::InitVertexBuffer(DXDevice* pDevice, const MeshData* pMeshData)
{
	DXHeapProperties uploadHeapProperties(D3D12_HEAP_TYPE_UPLOAD);
	DXHeapProperties defaultHeapProperties(D3D12_HEAP_TYPE_DEFAULT);

	const u32 numVertices = pMeshData->GetNumVertices();

	const Vector3f* pPositions = pMeshData->GetPositions();
	const Vector4f* pColors = pMeshData->GetColors();
	const Vector3f* pNormals = pMeshData->GetNormals();
	const Vector2f* pTexCoords = pMeshData->GetTexCoords();
	const Vector3f* pTangents = pMeshData->GetTangents();
		
	if (pNormals != nullptr)
	{
		struct Vertex
		{
			Vector3f mPosition;
			Vector3f mNormal;
		};

		std::vector<Vertex> vertices(numVertices);
		for (u32 i = 0; i < numVertices; ++i)
		{
			vertices[i].mPosition = pPositions[i];
			vertices[i].mNormal = pNormals[i];
		}

		const u32 strideInBytes = sizeof(Vertex);
		const u32 sizeInBytes = numVertices * strideInBytes;

		DXBufferResourceDesc bufferDesc(sizeInBytes);

		m_pDefaultHeapVB = new DXResource(pDevice, &defaultHeapProperties, D3D12_HEAP_FLAG_NONE, &bufferDesc, D3D12_RESOURCE_STATE_COPY_DEST, L"Mesh::m_pDefaultHeapVB");
		m_pVBView = new DXVertexBufferView(m_pDefaultHeapVB, sizeInBytes, strideInBytes);

		m_pUploadHeapVB = new DXResource(pDevice, &uploadHeapProperties, D3D12_HEAP_FLAG_NONE, &bufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, L"Mesh::m_pUploadHeapVB");
		m_pUploadHeapVB->Write(&vertices[0], sizeInBytes);
	}
	else
	{
		assert(false);
	}
}

void Mesh::InitIndexBuffer(DXDevice* pDevice, const MeshData* pMeshData)
{
	DXHeapProperties uploadHeapProperties(D3D12_HEAP_TYPE_UPLOAD);
	DXHeapProperties defaultHeapProperties(D3D12_HEAP_TYPE_DEFAULT);

	const u32 numIndices = pMeshData->GetNumIndices();
	
	const u16* p16BitIndices = pMeshData->Get16BitIndices();
	const u32* p32BitIndices = pMeshData->Get32BitIndices();
	
	assert((p16BitIndices != nullptr) || (p32BitIndices != nullptr));
	const bool is32BitIndices = p32BitIndices != nullptr;

	const u32 strideInBytes = is32BitIndices ? 32 : 16;
	const u32 sizeInBytes = numIndices * strideInBytes;
	
	DXBufferResourceDesc bufferDesc(sizeInBytes);

	m_pDefaultHeapIB = new DXResource(pDevice, &defaultHeapProperties, D3D12_HEAP_FLAG_NONE, &bufferDesc, D3D12_RESOURCE_STATE_COPY_DEST, L"Mesh::m_pDefaultHeapIB");
	m_pIBView = new DXIndexBufferView(m_pDefaultHeapIB, sizeInBytes, strideInBytes);

	m_pUploadHeapIB = new DXResource(pDevice, &uploadHeapProperties, D3D12_HEAP_FLAG_NONE, &bufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, L"Mesh::m_pUploadHeapIB");
	if (is32BitIndices)
		m_pUploadHeapIB->Write(p32BitIndices, sizeInBytes);
	else
		m_pUploadHeapIB->Write(p16BitIndices, sizeInBytes);
}

void Mesh::InitSubMeshes(const MeshData* pMeshData)
{
	const u32 numSubMeshes = pMeshData->GetNumSubMeshes();
	const SubMeshData* pSubMeshes = pMeshData->GetSubMeshes();

	m_pSubMeshes = new SubMeshData[numSubMeshes];
	std::memcpy(m_pSubMeshes, pSubMeshes, numSubMeshes * sizeof(SubMeshData));
}

namespace
{
	u8 ComputeVertexElementFlags(const MeshData* pMeshData)
	{
		const Vector3f* pPositions = pMeshData->GetPositions();
		assert(pPositions != nullptr);
		u8 flags = VertexElementFlag_Position;
		
		const Vector3f* pNormals = pMeshData->GetNormals();
		if (pNormals != nullptr)
			flags |= VertexElementFlag_Normal;

		const Vector4f* pColors = pMeshData->GetColors();
		if (pColors != nullptr)
			flags |= VertexElementFlag_Color;

		const Vector3f* pTangents = pMeshData->GetTangents();
		if (pTangents != nullptr)
			flags |= VertexElementFlag_Tangent;

		const Vector3f* pBiTangents = pMeshData->GetBiTangents();
		if (pBiTangents != nullptr)
			flags |= VertexElementFlag_BiTangent;
		
		const Vector2f* pTexCoords = pMeshData->GetTexCoords();
		if (pTexCoords != nullptr)
			flags |= VertexElementFlag_TexCoords;

		return flags;
	}
}