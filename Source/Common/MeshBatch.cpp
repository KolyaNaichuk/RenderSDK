#include "Common/MeshBatch.h"
#include "Common/MeshBatchData.h"
#include "Common/MeshData.h"
#include "DX/DXResource.h"
#include "DX/DXCommandList.h"
#include "DX/DXPipelineState.h"
#include "DX/DXRenderEnvironment.h"
#include "Math/Vector2.h"
#include "Math/Vector3.h"
#include "Math/Vector4.h"

namespace
{
	u32 GetBytesPerElement(DXGI_FORMAT format);
	DXInputLayoutDesc* CreateInputLayout(u8 vertexFormatFlags, u32* pOutStrideInBytes = nullptr);
}

MeshBatch::MeshBatch(DXRenderEnvironment* pEnv, const MeshBatchData* pBatchData)
	: m_pUploadVertexBuffer(nullptr)
	, m_pUploadIndexBuffer(nullptr)
	, m_pUploadMeshBoundsBuffer(nullptr)
	, m_pUploadMeshDescBuffer(nullptr)
	, m_pUploadMaterialBuffer(nullptr)
	, m_pVertexBuffer(nullptr)
	, m_pIndexBuffer(nullptr)
	, m_pMeshBoundsBuffer(nullptr)
	, m_pMeshDescBuffer(nullptr)
	, m_pMaterialBuffer(nullptr)
	, m_pInputLayout(nullptr)
	, m_PrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED)
	, m_PrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_UNDEFINED)
{
	InitVertexBuffer(pEnv, pBatchData);
	InitIndexBuffer(pEnv, pBatchData);
	InitMeshBoundsBuffer(pEnv, pBatchData);
	InitMeshDescBuffer(pEnv, pBatchData);
	InitMaterialBuffer(pEnv, pBatchData);
}

MeshBatch::~MeshBatch()
{
	RemoveDataForUpload();

	SafeDelete(m_pVertexBuffer);
	SafeDelete(m_pIndexBuffer);
	SafeDelete(m_pMeshBoundsBuffer);
	SafeDelete(m_pMeshDescBuffer);
	SafeDelete(m_pMaterialBuffer);
}

void MeshBatch::RecordDataForUpload(DXCommandList* pCommandList)
{
	pCommandList->CopyResource(m_pVertexBuffer, m_pUploadVertexBuffer);
	pCommandList->CopyResource(m_pIndexBuffer, m_pUploadIndexBuffer);
	pCommandList->CopyResource(m_pMeshBoundsBuffer, m_pUploadMeshBoundsBuffer);
	pCommandList->CopyResource(m_pMeshDescBuffer, m_pUploadMeshDescBuffer);
	pCommandList->CopyResource(m_pMaterialBuffer, m_pUploadMaterialBuffer);
	
	const D3D12_RESOURCE_BARRIER resourceTransitions[] =
	{
		DXResourceTransitionBarrier(m_pVertexBuffer, m_pVertexBuffer->GetState(), m_pVertexBuffer->GetReadState()),
		DXResourceTransitionBarrier(m_pIndexBuffer, m_pIndexBuffer->GetState(), m_pIndexBuffer->GetReadState()),
		DXResourceTransitionBarrier(m_pMeshBoundsBuffer, m_pMeshBoundsBuffer->GetState(), m_pMeshBoundsBuffer->GetReadState()),
		DXResourceTransitionBarrier(m_pMeshDescBuffer, m_pMeshDescBuffer->GetState(), m_pMeshDescBuffer->GetReadState()),
		DXResourceTransitionBarrier(m_pMaterialBuffer, m_pMaterialBuffer->GetState(), m_pMaterialBuffer->GetReadState())
	};
	pCommandList->ResourceBarrier(ARRAYSIZE(resourceTransitions), &resourceTransitions[0]);
	
	m_pVertexBuffer->SetState(m_pVertexBuffer->GetReadState());
	m_pIndexBuffer->SetState(m_pIndexBuffer->GetReadState());
	m_pMeshBoundsBuffer->SetState(m_pMeshBoundsBuffer->GetReadState());
	m_pMeshDescBuffer->SetState(m_pMeshDescBuffer->GetReadState());
	m_pMaterialBuffer->SetState(m_pMaterialBuffer->GetReadState());
}

void MeshBatch::RemoveDataForUpload()
{
	SafeDelete(m_pUploadVertexBuffer);
	SafeDelete(m_pUploadIndexBuffer);
	SafeDelete(m_pUploadMeshBoundsBuffer);
	SafeDelete(m_pUploadMeshDescBuffer);
	SafeDelete(m_pUploadMaterialBuffer);
}

void MeshBatch::InitVertexBuffer(DXRenderEnvironment* pEnv, const MeshBatchData* pBatchData)
{
	const u32 numVertices = pBatchData->GetNumVertices();
	const u8 vertexFormatFlags = pBatchData->GetVertexFormatFlags();

	u32 strideInBytes = 0;
	m_pInputLayout = CreateInputLayout(vertexFormatFlags, &strideInBytes);

	m_PrimitiveTopologyType = pBatchData->GetPrimitiveTopologyType();
	m_PrimitiveTopology = pBatchData->GetPrimitiveTopology();

	const u32 sizeInBytes = numVertices * strideInBytes;
	
	u8* pVertexData = new u8[sizeInBytes];
	u32 vertexOffset = 0;

	assert((vertexFormatFlags & VertexData::FormatFlag_Position) != 0);
	{
		const Vector3f* pPositions = pBatchData->GetPositions();
		const u32 elementSizeInBytes = sizeof(pPositions[0]);

		for (u32 index = 0; index < numVertices; ++index)
			std::memcpy(pVertexData + index * strideInBytes + vertexOffset, &pPositions[index], elementSizeInBytes);

		vertexOffset += elementSizeInBytes;
	}
	if ((vertexFormatFlags & VertexData::FormatFlag_Normal) != 0)
	{
		const Vector3f* pNormals = pBatchData->GetNormals();
		const u32 elementSizeInBytes = sizeof(pNormals[0]);

		for (u32 index = 0; index < numVertices; ++index)
			std::memcpy(pVertexData + index * strideInBytes + vertexOffset, &pNormals[index], elementSizeInBytes);

		vertexOffset += elementSizeInBytes;
	}
	if ((vertexFormatFlags & VertexData::FormatFlag_Color) != 0)
	{
		const Vector4f* pColors = pBatchData->GetColors();
		const u32 elementSizeInBytes = sizeof(pColors[0]);

		for (u32 index = 0; index < numVertices; ++index)
			std::memcpy(pVertexData + index * strideInBytes + vertexOffset, &pColors[index], elementSizeInBytes);
		
		vertexOffset += elementSizeInBytes;
	}
	if ((vertexFormatFlags & VertexData::FormatFlag_Tangent) != 0)
	{
		const Vector3f* pTangents = pBatchData->GetTangents();
		const u32 elementSizeInBytes = sizeof(pTangents[0]);

		for (u32 index = 0; index < numVertices; ++index)
			std::memcpy(pVertexData + index * strideInBytes + vertexOffset, &pTangents[index], elementSizeInBytes);
		
		vertexOffset += elementSizeInBytes;
	}
	if ((vertexFormatFlags & VertexData::FormatFlag_TexCoords) != 0)
	{
		const Vector2f* pTexCoords = pBatchData->GetTexCoords();
		const u32 elementSizeInBytes = sizeof(pTexCoords[0]);

		for (u32 index = 0; index < numVertices; ++index)
			std::memcpy(pVertexData + index * strideInBytes + vertexOffset, &pTexCoords[index], elementSizeInBytes);

		vertexOffset += elementSizeInBytes;
	}
	
	DXVertexBufferDesc bufferDesc(numVertices, strideInBytes);
	m_pVertexBuffer = new DXBuffer(pEnv, pEnv->m_pDefaultHeapProps, &bufferDesc, D3D12_RESOURCE_STATE_COPY_DEST, L"MeshBatch::m_pVertexBuffer");
	
	m_pUploadVertexBuffer = new DXBuffer(pEnv, pEnv->m_pUploadHeapProps, &bufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, L"MeshBatch::m_pUploadVertexBuffer");
	m_pUploadVertexBuffer->Write(pVertexData, sizeInBytes);
	
	SafeArrayDelete(pVertexData);
}

void MeshBatch::InitIndexBuffer(DXRenderEnvironment* pEnv, const MeshBatchData* pBatchData)
{
	const u32 numIndices = pBatchData->GetNumIndices();
	const bool use16BitIndices = (pBatchData->GetIndexFormat() == DXGI_FORMAT_R16_UINT);

	const u32 strideInBytes = use16BitIndices ? sizeof(u16) : sizeof(u32);
	const u32 sizeInBytes = numIndices * strideInBytes;
	
	DXIndexBufferDesc bufferDesc(numIndices, strideInBytes);
	m_pIndexBuffer = new DXBuffer(pEnv, pEnv->m_pDefaultHeapProps, &bufferDesc, D3D12_RESOURCE_STATE_COPY_DEST, L"MeshBatch::m_pIndexBuffer");
	
	m_pUploadIndexBuffer = new DXBuffer(pEnv, pEnv->m_pUploadHeapProps, &bufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, L"MeshBatch::m_pUploadIndexBuffer");
	if (use16BitIndices)
		m_pUploadIndexBuffer->Write(pBatchData->Get16BitIndices(), sizeInBytes);
	else
		m_pUploadIndexBuffer->Write(pBatchData->Get32BitIndices(), sizeInBytes);
}

void MeshBatch::InitMeshBoundsBuffer(DXRenderEnvironment* pEnv, const MeshBatchData* pBatchData)
{
	const u32 numMeshes = pBatchData->GetNumMeshes();
	const u32 structureByteStride = sizeof(AxisAlignedBox);

	DXStructuredBufferDesc bufferDesc(numMeshes, structureByteStride, true, false);
	m_pMeshBoundsBuffer = new DXBuffer(pEnv, pEnv->m_pDefaultHeapProps, &bufferDesc, D3D12_RESOURCE_STATE_COPY_DEST, L"MeshBatch::m_pMeshBoundsBuffer");

	m_pUploadMeshBoundsBuffer = new DXBuffer(pEnv, pEnv->m_pUploadHeapProps, &bufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, L"MeshBatch::m_pUploadMeshBoundsBuffer");
	m_pUploadMeshBoundsBuffer->Write(pBatchData->GetMeshAABBs(), numMeshes * structureByteStride);
}

void MeshBatch::InitMeshDescBuffer(DXRenderEnvironment* pEnv, const MeshBatchData* pBatchData)
{
	const u32 numMeshes = pBatchData->GetNumMeshes();
	const u32 structureByteStride = sizeof(MeshDesc);

	DXStructuredBufferDesc bufferDesc(numMeshes, structureByteStride, true, false);
	m_pMeshDescBuffer = new DXBuffer(pEnv, pEnv->m_pDefaultHeapProps, &bufferDesc, D3D12_RESOURCE_STATE_COPY_DEST, L"MeshBatch::m_pMeshDescBuffer");

	m_pUploadMeshDescBuffer = new DXBuffer(pEnv, pEnv->m_pUploadHeapProps, &bufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, L"MeshBatch::m_pUploadMeshDescBuffer");
	m_pUploadMeshDescBuffer->Write(pBatchData->GetMeshDescs(), numMeshes * structureByteStride);
}

void MeshBatch::InitMaterialBuffer(DXRenderEnvironment* pEnv, const MeshBatchData* pBatchData)
{
	const u32 numMeshes = pBatchData->GetNumMeshes();
	const u32 structureByteStride = sizeof(Material);

	DXStructuredBufferDesc bufferDesc(numMeshes, structureByteStride, true, false);
	m_pMaterialBuffer = new DXBuffer(pEnv, pEnv->m_pDefaultHeapProps, &bufferDesc, D3D12_RESOURCE_STATE_COPY_DEST, L"MeshBatch::m_pMaterialBuffer");

	m_pUploadMaterialBuffer = new DXBuffer(pEnv, pEnv->m_pUploadHeapProps, &bufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, L"MeshBatch::m_pUploadMaterialBuffer");
	m_pUploadMaterialBuffer->Write(pBatchData->GetMaterials(), numMeshes * structureByteStride);
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

	DXInputLayoutDesc* CreateInputLayout(u8 vertexFormatFlags, u32* pOutStrideInBytes)
	{
		std::vector<DXInputElementDesc> inputElements;
		inputElements.reserve(6);

		u32 byteOffset = 0;
		assert((vertexFormatFlags & VertexData::FormatFlag_Position) != 0);
		{
			const DXGI_FORMAT format = DXGI_FORMAT_R32G32B32_FLOAT;
			inputElements.emplace_back("POSITION", 0, format, 0, byteOffset);

			byteOffset += GetBytesPerElement(format);
		}
		if ((vertexFormatFlags & VertexData::FormatFlag_Normal) != 0)
		{
			const DXGI_FORMAT format = DXGI_FORMAT_R32G32B32_FLOAT;
			inputElements.emplace_back("NORMAL", 0, format, 0, byteOffset);

			byteOffset += GetBytesPerElement(format);
		}
		if ((vertexFormatFlags & VertexData::FormatFlag_Color) != 0)
		{
			const DXGI_FORMAT format = DXGI_FORMAT_R32G32B32A32_FLOAT;
			inputElements.emplace_back("COLOR", 0, format, 0, byteOffset);

			byteOffset += GetBytesPerElement(format);
		}
		if ((vertexFormatFlags & VertexData::FormatFlag_Tangent) != 0)
		{
			const DXGI_FORMAT format = DXGI_FORMAT_R32G32B32_FLOAT;
			inputElements.emplace_back("TANGENT", 0, format, 0, byteOffset);

			byteOffset += GetBytesPerElement(format);
		}
		if ((vertexFormatFlags & VertexData::FormatFlag_TexCoords) != 0)
		{
			const DXGI_FORMAT format = DXGI_FORMAT_R32G32_FLOAT;
			inputElements.emplace_back("TEXCOORD", 0, format, 0, byteOffset);

			byteOffset += GetBytesPerElement(format);
		}
		if (pOutStrideInBytes != nullptr)
			*pOutStrideInBytes = byteOffset;

		return new DXInputLayoutDesc(inputElements.size(), &inputElements[0]);
	}
}
