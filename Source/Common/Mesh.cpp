#include "Common/Mesh.h"
#include "Common/MeshData.h"
#include "DX/DXResource.h"
#include "DX/DXCommandList.h"
#include "DX/DXPipelineState.h"
#include "Math/Vector2f.h"
#include "Math/Vector3f.h"

namespace
{
	u32 GetBytesPerElement(DXGI_FORMAT format);
}

Mesh::Mesh(DXDevice* pDevice, const MeshData* pMeshData)
	: m_pUploadHeapVB(nullptr)
	, m_pUploadHeapIB(nullptr)
	, m_pDefaultHeapVB(nullptr)
	, m_pDefaultHeapIB(nullptr)
	, m_pVBView(nullptr)
	, m_pIBView(nullptr)
	, m_pInputLayoutDesc(nullptr)
	, m_NumSubMeshes(0)
	, m_pSubMeshes(nullptr)
{
	InitInputLayoutDesc(pMeshData);
	InitVertexBuffer(pDevice, pMeshData);
	InitIndexBuffer(pDevice, pMeshData);
	InitSubMeshes(pMeshData);
}

Mesh::~Mesh()
{
	SafeDelete(m_pInputLayoutDesc);
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

const DXInputLayoutDesc* Mesh::GetInputLayoutDesc() const {
	return m_pInputLayoutDesc;
}

u32 Mesh::GetNumSubMeshes() const
{
	return m_NumSubMeshes;
}

const SubMeshData* Mesh::GetSubMeshes() const
{
	return m_pSubMeshes;
}

void Mesh::InitInputLayoutDesc(const MeshData* pMeshData)
{
	assert(false && "Check if D3D11_APPEND_ALIGNED_ELEMENT could be used for byteOffset");
	assert(false && "If using D3D11_APPEND_ALIGNED_ELEMENT then byteOffset is not needed probably at all");

	const Vector3f* pPositions = pMeshData->GetPositions();
	const Vector4f* pColors = pMeshData->GetColors();
	const Vector3f* pNormals = pMeshData->GetNormals();
	const Vector2f* pTexCoords = pMeshData->GetTexCoords();
	const Vector4f* pTangents = pMeshData->GetTangents();

	UINT byteOffset = 0;
	std::vector<DXInputElementDesc> inputElements;

	assert(pPositions != nullptr);
	{
		const DXGI_FORMAT format = DXGI_FORMAT_R32G32B32_FLOAT;
		inputElements.push_back(DXInputElementDesc("POSITION", 0, format, 0, byteOffset));
		byteOffset += GetBytesPerElement(format);
	}	

	if (pNormals != nullptr)
	{
		const DXGI_FORMAT format = DXGI_FORMAT_R32G32B32_FLOAT;
		inputElements.push_back(DXInputElementDesc("NORMAL", 0, format, 0, byteOffset));
		byteOffset += GetBytesPerElement(format);
	}

	if (pColors != nullptr)
	{
		const DXGI_FORMAT format = DXGI_FORMAT_B8G8R8A8_UNORM;
		inputElements.push_back(DXInputElementDesc("COLOR", 0, format, 0, byteOffset));
		byteOffset += GetBytesPerElement(format);
	}

	if (pTangents != nullptr)
	{
		const DXGI_FORMAT format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		inputElements.push_back(DXInputElementDesc("TANGENT", 0, format, 0, byteOffset));
		byteOffset += GetBytesPerElement(format);
	}

	if (pTexCoords != nullptr)
	{
		const DXGI_FORMAT format = DXGI_FORMAT_R32G32_FLOAT;
		inputElements.push_back(DXInputElementDesc("TEXCOORD", 0, format, 0, byteOffset));
		byteOffset += GetBytesPerElement(format);
	}

	m_pInputLayoutDesc = new DXInputLayoutDesc(inputElements.size(), &inputElements[0]);
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
	const Vector4f* pTangents = pMeshData->GetTangents();
		
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
	u32 GetBytesPerElement(DXGI_FORMAT format)
	{
		switch (format)
		{
			case DXGI_FORMAT_R32G32B32A32_FLOAT:
			case DXGI_FORMAT_R32G32B32A32_UINT:
			case DXGI_FORMAT_R32G32B32A32_SINT:
				return 16;

			case DXGI_FORMAT_R32G32B32_FLOAT:
			case DXGI_FORMAT_R32G32B32_UINT:
			case DXGI_FORMAT_R32G32B32_SINT:
				return 12;

			case DXGI_FORMAT_R16G16B16A16_FLOAT:
			case DXGI_FORMAT_R16G16B16A16_UNORM:
			case DXGI_FORMAT_R16G16B16A16_UINT:
			case DXGI_FORMAT_R16G16B16A16_SNORM:
			case DXGI_FORMAT_R16G16B16A16_SINT:
			case DXGI_FORMAT_R32G32_FLOAT:
			case DXGI_FORMAT_R32G32_UINT:
			case DXGI_FORMAT_R32G32_SINT:
				return 8;

			case DXGI_FORMAT_R10G10B10A2_UNORM:
			case DXGI_FORMAT_R10G10B10A2_UINT:
			case DXGI_FORMAT_R11G11B10_FLOAT:
			case DXGI_FORMAT_R8G8B8A8_UNORM:
			case DXGI_FORMAT_R8G8B8A8_UINT:
			case DXGI_FORMAT_R8G8B8A8_SNORM:
			case DXGI_FORMAT_R8G8B8A8_SINT:
			case DXGI_FORMAT_R16G16_FLOAT:
			case DXGI_FORMAT_R16G16_UNORM:
			case DXGI_FORMAT_R16G16_UINT:
			case DXGI_FORMAT_R16G16_SNORM:
			case DXGI_FORMAT_R16G16_SINT:
			case DXGI_FORMAT_R32_FLOAT:
			case DXGI_FORMAT_R32_UINT:
			case DXGI_FORMAT_R32_SINT:
			case DXGI_FORMAT_B8G8R8A8_UNORM:
			case DXGI_FORMAT_B8G8R8X8_UNORM:
				return 4;

			case DXGI_FORMAT_R8G8_UNORM:
			case DXGI_FORMAT_R8G8_UINT:
			case DXGI_FORMAT_R8G8_SNORM:
			case DXGI_FORMAT_R8G8_SINT:
			case DXGI_FORMAT_R16_FLOAT:
			case DXGI_FORMAT_R16_UNORM:
			case DXGI_FORMAT_R16_UINT:
			case DXGI_FORMAT_R16_SNORM:
			case DXGI_FORMAT_R16_SINT:
			case DXGI_FORMAT_B5G6R5_UNORM:
			case DXGI_FORMAT_B5G5R5A1_UNORM:
				return 2;

			case DXGI_FORMAT_R8_UNORM:
			case DXGI_FORMAT_R8_UINT:
			case DXGI_FORMAT_R8_SNORM:
			case DXGI_FORMAT_R8_SINT:
				return 1;

			case DXGI_FORMAT_B4G4R4A4_UNORM:
				return 2;

			default:
				assert(false);
				return 0;
		}
	}
}