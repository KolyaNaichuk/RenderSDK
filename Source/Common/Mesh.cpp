#include "Common/Mesh.h"
#include "Common/MeshData.h"
#include "DX/DXResource.h"
#include "DX/DXCommandList.h"
#include "DX/DXPipelineState.h"
#include "Math/Vector2.h"
#include "Math/Vector3.h"
#include "Math/Vector4.h"

namespace
{
	u32 GetBytesPerElement(DXGI_FORMAT format);
	void RetrieveVertexInfo(const MeshData* pMeshData, u8* pVertexElementFlags, u32* pStrideInBytes);
}

Mesh::Mesh(DXDevice* pDevice, const MeshData* pMeshData)
	: m_pUploadHeapVB(nullptr)
	, m_pUploadHeapIB(nullptr)
	, m_pDefaultHeapVB(nullptr)
	, m_pDefaultHeapIB(nullptr)
	, m_pVBView(nullptr)
	, m_pIBView(nullptr)
	, m_VertexElementFlags(0)
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

u8 Mesh::GetVertexElementFlags() const
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

	u32 strideInBytes = 0;
	RetrieveVertexInfo(pMeshData, &m_VertexElementFlags, &strideInBytes);

	const u32 sizeInBytes = numVertices * strideInBytes;
	
	u8* pVertexData = new u8[sizeInBytes];
	u32 vertexOffset = 0;

	const Vector3f* pPositions = pMeshData->GetPositions();
	assert(pPositions != nullptr);
	{             
		const u32 elementSizeInBytes = sizeof(pPositions[0]);

		for (u32 index = 0; index < numVertices; ++index)
			std::memcpy(pVertexData + index * strideInBytes + vertexOffset, &pPositions[index], elementSizeInBytes);

		vertexOffset += elementSizeInBytes;
	}

	const Vector3f* pNormals = pMeshData->GetNormals();
	if (pNormals != nullptr)
	{
		const u32 elementSizeInBytes = sizeof(pNormals[0]);

		for (u32 index = 0; index < numVertices; ++index)
			std::memcpy(pVertexData + index * strideInBytes + vertexOffset, &pNormals[index], elementSizeInBytes);

		vertexOffset += elementSizeInBytes;
	}

	const Vector4f* pColors = pMeshData->GetColors();
	if (pColors != nullptr)
	{
		const u32 elementSizeInBytes = sizeof(pColors[0]);

		for (u32 index = 0; index < numVertices; ++index)
			std::memcpy(pVertexData + index * strideInBytes + vertexOffset, &pColors[index], elementSizeInBytes);
		
		vertexOffset += elementSizeInBytes;
	}

	const Vector3f* pTangents = pMeshData->GetTangents();
	if (pTangents != nullptr)
	{
		const u32 elementSizeInBytes = sizeof(pTangents[0]);

		for (u32 index = 0; index < numVertices; ++index)
			std::memcpy(pVertexData + index * strideInBytes + vertexOffset, &pTangents[index], elementSizeInBytes);
		
		vertexOffset += elementSizeInBytes;
	}

	const Vector3f* pBiTangents = pMeshData->GetBiTangents();
	if (pBiTangents != nullptr)
	{
		const u32 elementSizeInBytes = sizeof(pBiTangents[0]);

		for (u32 index = 0; index < numVertices; ++index)
			std::memcpy(pVertexData + index * strideInBytes + vertexOffset, &pBiTangents[index], elementSizeInBytes);
		
		vertexOffset += elementSizeInBytes;
	}

	const Vector2f* pTexCoords = pMeshData->GetTexCoords();
	if (pTexCoords != nullptr)
	{
		const u32 elementSizeInBytes = sizeof(pTexCoords[0]);

		for (u32 index = 0; index < numVertices; ++index)
			std::memcpy(pVertexData + index * strideInBytes + vertexOffset, &pTexCoords[index], elementSizeInBytes);

		vertexOffset += elementSizeInBytes;
	}
	
	DXBufferResourceDesc bufferDesc(sizeInBytes);

	m_pDefaultHeapVB = new DXResource(pDevice, &defaultHeapProperties, D3D12_HEAP_FLAG_NONE, &bufferDesc, D3D12_RESOURCE_STATE_COPY_DEST, L"Mesh::m_pDefaultHeapVB");
	m_pVBView = new DXVertexBufferView(m_pDefaultHeapVB, sizeInBytes, strideInBytes);

	m_pUploadHeapVB = new DXResource(pDevice, &uploadHeapProperties, D3D12_HEAP_FLAG_NONE, &bufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, L"Mesh::m_pUploadHeapVB");
	m_pUploadHeapVB->Write(pVertexData, sizeInBytes);
	
	SafeArrayDelete(pVertexData);
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

	const u32 strideInBytes = is32BitIndices ? sizeof(u32) : sizeof(u16);
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
	m_NumSubMeshes = pMeshData->GetNumSubMeshes();
	const SubMeshData* pSubMeshes = pMeshData->GetSubMeshes();

	m_pSubMeshes = new SubMeshData[m_NumSubMeshes];
	std::memcpy(m_pSubMeshes, pSubMeshes, m_NumSubMeshes * sizeof(SubMeshData));
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

	void RetrieveVertexInfo(const MeshData* pMeshData, u8* pVertexElementFlags, u32* pStrideInBytes)
	{
		u8 vertexElementFlags = 0;
		u8 strideInBytes = 0;

		const Vector3f* pPositions = pMeshData->GetPositions();
		assert(pPositions != nullptr);
		{
			vertexElementFlags |= VertexElementFlag_Position;
			strideInBytes += sizeof(pPositions[0]);
		}

		const Vector3f* pNormals = pMeshData->GetNormals();
		if (pNormals != nullptr)
		{
			vertexElementFlags |= VertexElementFlag_Normal;
			strideInBytes += sizeof(pNormals[0]);
		}			

		const Vector4f* pColors = pMeshData->GetColors();
		if (pColors != nullptr)
		{
			vertexElementFlags |= VertexElementFlag_Color;
			strideInBytes += sizeof(pColors[0]);
		}

		const Vector3f* pTangents = pMeshData->GetTangents();
		if (pTangents != nullptr)
		{
			vertexElementFlags |= VertexElementFlag_Tangent;
			strideInBytes += sizeof(pTangents[0]);
		}			

		const Vector3f* pBiTangents = pMeshData->GetBiTangents();
		if (pBiTangents != nullptr)
		{
			vertexElementFlags |= VertexElementFlag_BiTangent;
			strideInBytes += sizeof(pBiTangents[0]);
		}			
		
		const Vector2f* pTexCoords = pMeshData->GetTexCoords();
		if (pTexCoords != nullptr)
		{
			vertexElementFlags |= VertexElementFlag_TexCoords;
			strideInBytes += sizeof(pTexCoords[0]);
		}

		if (pVertexElementFlags != nullptr)
			*pVertexElementFlags = vertexElementFlags;

		if (pStrideInBytes != nullptr)
			*pStrideInBytes = strideInBytes;
	}
}

void GenerateInputElements(std::vector<DXInputElementDesc>& inputElements, u8 inputElementFlags, u8 vertexElementFlags)
{
	assert((inputElementFlags & vertexElementFlags) == inputElementFlags);

	assert(inputElements.empty());
	inputElements.reserve(6);

	UINT byteOffset = 0;
	assert(vertexElementFlags & VertexElementFlag_Position);
	{
		const DXGI_FORMAT format = DXGI_FORMAT_R32G32B32_FLOAT;

		if (inputElementFlags & VertexElementFlag_Position)
			inputElements.push_back(DXInputElementDesc("POSITION", 0, format, 0, byteOffset));

		byteOffset += GetBytesPerElement(format);
	}

	if (vertexElementFlags & VertexElementFlag_Normal)
	{
		const DXGI_FORMAT format = DXGI_FORMAT_R32G32B32_FLOAT;

		if (inputElementFlags & VertexElementFlag_Normal)
			inputElements.push_back(DXInputElementDesc("NORMAL", 0, format, 0, byteOffset));

		byteOffset += GetBytesPerElement(format);
	}

	if (vertexElementFlags & VertexElementFlag_Color)
	{
		const DXGI_FORMAT format = DXGI_FORMAT_R32G32B32A32_FLOAT;

		if (inputElementFlags & VertexElementFlag_Color)
			inputElements.push_back(DXInputElementDesc("COLOR", 0, format, 0, byteOffset));

		byteOffset += GetBytesPerElement(format);
	}

	if (vertexElementFlags & VertexElementFlag_Tangent)
	{
		const DXGI_FORMAT format = DXGI_FORMAT_R32G32B32_FLOAT;

		if (inputElementFlags & VertexElementFlag_Tangent)
			inputElements.push_back(DXInputElementDesc("TANGENT", 0, format, 0, byteOffset));

		byteOffset += GetBytesPerElement(format);
	}

	if (vertexElementFlags & VertexElementFlag_BiTangent)
	{
		const DXGI_FORMAT format = DXGI_FORMAT_R32G32B32_FLOAT;

		if (inputElementFlags & VertexElementFlag_BiTangent)
			inputElements.push_back(DXInputElementDesc("BITANGENT", 0, format, 0, byteOffset));

		byteOffset += GetBytesPerElement(format);
	}

	if (vertexElementFlags & VertexElementFlag_TexCoords)
	{
		const DXGI_FORMAT format = DXGI_FORMAT_R32G32_FLOAT;

		if (inputElementFlags & VertexElementFlag_TexCoords)
			inputElements.push_back(DXInputElementDesc("TEXCOORD", 0, format, 0, byteOffset));

		byteOffset += GetBytesPerElement(format);
	}
}