#include "Common/MeshBatch.h"
#include "Common/MeshBatchData.h"
#include "Common/MeshData.h"
#include "D3DWrapper/D3DResource.h"
#include "D3DWrapper/D3DCommandList.h"
#include "D3DWrapper/D3DPipelineState.h"
#include "D3DWrapper/D3DRenderEnv.h"
#include "D3DWrapper/D3DUtils.h"
#include "Math/Vector2.h"
#include "Math/Vector3.h"
#include "Math/Vector4.h"

MeshBatch::MeshBatch(D3DRenderEnv* pRenderEnv, const MeshBatchData* pBatchData)
	: m_NumMeshes(pBatchData->GetNumMeshes())
	, m_VertexStrideInBytes(0)
	, m_pInputLayout(nullptr)
	, m_PrimitiveTopologyType(pBatchData->GetPrimitiveTopologyType())
	, m_PrimitiveTopology(pBatchData->GetPrimitiveTopology())
	, m_pUploadVertexBuffer(nullptr)
	, m_pUploadIndexBuffer(nullptr)
	, m_pUploadMeshBoundsBuffer(nullptr)
	, m_pUploadMeshDescBuffer(nullptr)
	, m_pUploadMaterialBuffer(nullptr)
	, m_pVertexBuffer(nullptr)
	, m_pIndexBuffer(nullptr)
	, m_pMeshBoundsBuffer(nullptr)
	, m_pMeshDescBuffer(nullptr)
	, m_pMaterialBuffer(nullptr)
{
	InitInputLayout(pRenderEnv, pBatchData);
	InitVertexBuffer(pRenderEnv, pBatchData);
	InitIndexBuffer(pRenderEnv, pBatchData);
	InitMeshBoundsBuffer(pRenderEnv, pBatchData);
	InitMeshDescBuffer(pRenderEnv, pBatchData);
	InitMaterialBuffer(pRenderEnv, pBatchData);
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

void MeshBatch::RecordDataForUpload(D3DCommandList* pCommandList)
{
	pCommandList->CopyResource(m_pVertexBuffer, m_pUploadVertexBuffer);
	pCommandList->CopyResource(m_pIndexBuffer, m_pUploadIndexBuffer);
	pCommandList->CopyResource(m_pMeshBoundsBuffer, m_pUploadMeshBoundsBuffer);
	pCommandList->CopyResource(m_pMeshDescBuffer, m_pUploadMeshDescBuffer);
	pCommandList->CopyResource(m_pMaterialBuffer, m_pUploadMaterialBuffer);
	
	const D3D12_RESOURCE_BARRIER resourceTransitions[] =
	{
		D3DResourceTransitionBarrier(m_pVertexBuffer, m_pVertexBuffer->GetState(), m_pVertexBuffer->GetReadState()),
		D3DResourceTransitionBarrier(m_pIndexBuffer, m_pIndexBuffer->GetState(), m_pIndexBuffer->GetReadState()),
		D3DResourceTransitionBarrier(m_pMeshBoundsBuffer, m_pMeshBoundsBuffer->GetState(), m_pMeshBoundsBuffer->GetReadState()),
		D3DResourceTransitionBarrier(m_pMeshDescBuffer, m_pMeshDescBuffer->GetState(), m_pMeshDescBuffer->GetReadState()),
		D3DResourceTransitionBarrier(m_pMaterialBuffer, m_pMaterialBuffer->GetState(), m_pMaterialBuffer->GetReadState())
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

void MeshBatch::InitInputLayout(D3DRenderEnv* pRenderEnv, const MeshBatchData* pBatchData)
{
	const u8 vertexFormatFlags = pBatchData->GetVertexFormatFlags();
	
	assert(m_InputElements.empty());
	m_InputElements.reserve(5);

	u32 byteOffset = 0;
	assert((vertexFormatFlags & VertexData::FormatFlag_Position) != 0);
	{
		const DXGI_FORMAT format = DXGI_FORMAT_R32G32B32_FLOAT;
		m_InputElements.emplace_back("POSITION", 0, format, 0, byteOffset);

		byteOffset += GetSizeInBytes(format);
	}
	if ((vertexFormatFlags & VertexData::FormatFlag_Normal) != 0)
	{
		const DXGI_FORMAT format = DXGI_FORMAT_R32G32B32_FLOAT;
		m_InputElements.emplace_back("NORMAL", 0, format, 0, byteOffset);

		byteOffset += GetSizeInBytes(format);
	}
	if ((vertexFormatFlags & VertexData::FormatFlag_Color) != 0)
	{
		const DXGI_FORMAT format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		m_InputElements.emplace_back("COLOR", 0, format, 0, byteOffset);

		byteOffset += GetSizeInBytes(format);
	}
	if ((vertexFormatFlags & VertexData::FormatFlag_Tangent) != 0)
	{
		const DXGI_FORMAT format = DXGI_FORMAT_R32G32B32_FLOAT;
		m_InputElements.emplace_back("TANGENT", 0, format, 0, byteOffset);

		byteOffset += GetSizeInBytes(format);
	}
	if ((vertexFormatFlags & VertexData::FormatFlag_TexCoords) != 0)
	{
		const DXGI_FORMAT format = DXGI_FORMAT_R32G32_FLOAT;
		m_InputElements.emplace_back("TEXCOORD", 0, format, 0, byteOffset);

		byteOffset += GetSizeInBytes(format);
	}

	m_VertexStrideInBytes = byteOffset;
	m_pInputLayout = new D3DInputLayoutDesc(m_InputElements.size(), &m_InputElements[0]);
}

void MeshBatch::InitVertexBuffer(D3DRenderEnv* pRenderEnv, const MeshBatchData* pBatchData)
{
	const u32 numVertices = pBatchData->GetNumVertices();
	const u8 vertexFormatFlags = pBatchData->GetVertexFormatFlags();
		
	assert(m_VertexStrideInBytes > 0);
	const u32 sizeInBytes = numVertices * m_VertexStrideInBytes;
	
	u8* pVertexData = new u8[sizeInBytes];
	u32 vertexOffset = 0;

	assert((vertexFormatFlags & VertexData::FormatFlag_Position) != 0);
	{
		const Vector3f* pPositions = pBatchData->GetPositions();
		const u32 elementSizeInBytes = sizeof(pPositions[0]);

		for (u32 index = 0; index < numVertices; ++index)
			std::memcpy(pVertexData + index * m_VertexStrideInBytes + vertexOffset, &pPositions[index], elementSizeInBytes);

		vertexOffset += elementSizeInBytes;
	}
	if ((vertexFormatFlags & VertexData::FormatFlag_Normal) != 0)
	{
		const Vector3f* pNormals = pBatchData->GetNormals();
		const u32 elementSizeInBytes = sizeof(pNormals[0]);

		for (u32 index = 0; index < numVertices; ++index)
			std::memcpy(pVertexData + index * m_VertexStrideInBytes + vertexOffset, &pNormals[index], elementSizeInBytes);

		vertexOffset += elementSizeInBytes;
	}
	if ((vertexFormatFlags & VertexData::FormatFlag_Color) != 0)
	{
		const Vector4f* pColors = pBatchData->GetColors();
		const u32 elementSizeInBytes = sizeof(pColors[0]);

		for (u32 index = 0; index < numVertices; ++index)
			std::memcpy(pVertexData + index * m_VertexStrideInBytes + vertexOffset, &pColors[index], elementSizeInBytes);
		
		vertexOffset += elementSizeInBytes;
	}
	if ((vertexFormatFlags & VertexData::FormatFlag_Tangent) != 0)
	{
		const Vector3f* pTangents = pBatchData->GetTangents();
		const u32 elementSizeInBytes = sizeof(pTangents[0]);

		for (u32 index = 0; index < numVertices; ++index)
			std::memcpy(pVertexData + index * m_VertexStrideInBytes + vertexOffset, &pTangents[index], elementSizeInBytes);
		
		vertexOffset += elementSizeInBytes;
	}
	if ((vertexFormatFlags & VertexData::FormatFlag_TexCoords) != 0)
	{
		const Vector2f* pTexCoords = pBatchData->GetTexCoords();
		const u32 elementSizeInBytes = sizeof(pTexCoords[0]);

		for (u32 index = 0; index < numVertices; ++index)
			std::memcpy(pVertexData + index * m_VertexStrideInBytes + vertexOffset, &pTexCoords[index], elementSizeInBytes);

		vertexOffset += elementSizeInBytes;
	}
	
	D3DVertexBufferDesc bufferDesc(numVertices, m_VertexStrideInBytes);
	m_pVertexBuffer = new D3DBuffer(pRenderEnv, pRenderEnv->m_pDefaultHeapProps, &bufferDesc, D3D12_RESOURCE_STATE_COPY_DEST, L"MeshBatch::m_pVertexBuffer");
	
	m_pUploadVertexBuffer = new D3DBuffer(pRenderEnv, pRenderEnv->m_pUploadHeapProps, &bufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, L"MeshBatch::m_pUploadVertexBuffer");
	m_pUploadVertexBuffer->Write(pVertexData, sizeInBytes);
	
	SafeArrayDelete(pVertexData);
}

void MeshBatch::InitIndexBuffer(D3DRenderEnv* pRenderEnv, const MeshBatchData* pBatchData)
{
	const u32 numIndices = pBatchData->GetNumIndices();
	const bool use16BitIndices = (pBatchData->GetIndexFormat() == DXGI_FORMAT_R16_UINT);

	const u32 strideInBytes = use16BitIndices ? sizeof(u16) : sizeof(u32);
	const u32 sizeInBytes = numIndices * strideInBytes;
	
	D3DIndexBufferDesc bufferDesc(numIndices, strideInBytes);
	m_pIndexBuffer = new D3DBuffer(pRenderEnv, pRenderEnv->m_pDefaultHeapProps, &bufferDesc, D3D12_RESOURCE_STATE_COPY_DEST, L"MeshBatch::m_pIndexBuffer");
	
	m_pUploadIndexBuffer = new D3DBuffer(pRenderEnv, pRenderEnv->m_pUploadHeapProps, &bufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, L"MeshBatch::m_pUploadIndexBuffer");
	if (use16BitIndices)
		m_pUploadIndexBuffer->Write(pBatchData->Get16BitIndices(), sizeInBytes);
	else
		m_pUploadIndexBuffer->Write(pBatchData->Get32BitIndices(), sizeInBytes);
}

void MeshBatch::InitMeshBoundsBuffer(D3DRenderEnv* pRenderEnv, const MeshBatchData* pBatchData)
{
	const u32 numMeshes = pBatchData->GetNumMeshes();
	const u32 structureByteStride = sizeof(AxisAlignedBox);

	D3DStructuredBufferDesc bufferDesc(numMeshes, structureByteStride, true, false);
	m_pMeshBoundsBuffer = new D3DBuffer(pRenderEnv, pRenderEnv->m_pDefaultHeapProps, &bufferDesc, D3D12_RESOURCE_STATE_COPY_DEST, L"MeshBatch::m_pMeshBoundsBuffer");

	m_pUploadMeshBoundsBuffer = new D3DBuffer(pRenderEnv, pRenderEnv->m_pUploadHeapProps, &bufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, L"MeshBatch::m_pUploadMeshBoundsBuffer");
	m_pUploadMeshBoundsBuffer->Write(pBatchData->GetMeshAABBs(), numMeshes * structureByteStride);
}

void MeshBatch::InitMeshDescBuffer(D3DRenderEnv* pRenderEnv, const MeshBatchData* pBatchData)
{
	const u32 numMeshes = pBatchData->GetNumMeshes();
	const u32 structureByteStride = sizeof(MeshDesc);

	D3DStructuredBufferDesc bufferDesc(numMeshes, structureByteStride, true, false);
	m_pMeshDescBuffer = new D3DBuffer(pRenderEnv, pRenderEnv->m_pDefaultHeapProps, &bufferDesc, D3D12_RESOURCE_STATE_COPY_DEST, L"MeshBatch::m_pMeshDescBuffer");

	m_pUploadMeshDescBuffer = new D3DBuffer(pRenderEnv, pRenderEnv->m_pUploadHeapProps, &bufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, L"MeshBatch::m_pUploadMeshDescBuffer");
	m_pUploadMeshDescBuffer->Write(pBatchData->GetMeshDescs(), numMeshes * structureByteStride);
}

void MeshBatch::InitMaterialBuffer(D3DRenderEnv* pRenderEnv, const MeshBatchData* pBatchData)
{
	const u32 numMeshes = pBatchData->GetNumMeshes();
	const u32 structureByteStride = sizeof(Material);

	D3DStructuredBufferDesc bufferDesc(numMeshes, structureByteStride, true, false);
	m_pMaterialBuffer = new D3DBuffer(pRenderEnv, pRenderEnv->m_pDefaultHeapProps, &bufferDesc, D3D12_RESOURCE_STATE_COPY_DEST, L"MeshBatch::m_pMaterialBuffer");

	m_pUploadMaterialBuffer = new D3DBuffer(pRenderEnv, pRenderEnv->m_pUploadHeapProps, &bufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, L"MeshBatch::m_pUploadMaterialBuffer");
	m_pUploadMaterialBuffer->Write(pBatchData->GetMaterials(), numMeshes * structureByteStride);
}
