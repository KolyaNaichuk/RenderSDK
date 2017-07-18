#include "Common/MeshBatch.h"
#include "Common/MeshBatchData.h"
#include "Common/MeshData.h"
#include "D3DWrapper/GraphicsResource.h"
#include "D3DWrapper/CommandList.h"
#include "D3DWrapper/PipelineState.h"
#include "D3DWrapper/RenderEnv.h"
#include "D3DWrapper/GraphicsUtils.h"
#include "Math/Vector2.h"
#include "Math/Vector3.h"
#include "Math/Vector4.h"

MeshBatch::MeshBatch(RenderEnv* pRenderEnv, const MeshBatchData* pBatchData, u32 meshType, u32 meshTypeOffset)
	: m_NumMeshes(pBatchData->GetNumMeshes())
	, m_VertexStrideInBytes(0)
	, m_pInputLayout(nullptr)
	, m_PrimitiveTopologyType(pBatchData->GetPrimitiveTopologyType())
	, m_PrimitiveTopology(pBatchData->GetPrimitiveTopology())
	, m_pUploadVertexBuffer(nullptr)
	, m_pUploadIndexBuffer(nullptr)
	, m_pUploadMeshInfoBuffer(nullptr)
	, m_pUploadInstanceAABBBuffer(nullptr)
	, m_pVertexBuffer(nullptr)
	, m_pIndexBuffer(nullptr)
	, m_pMeshInfoBuffer(nullptr)
	, m_pInstanceAABBBuffer(nullptr)
{
	InitInputLayout(pRenderEnv, pBatchData);
	InitVertexBuffer(pRenderEnv, pBatchData);
	InitIndexBuffer(pRenderEnv, pBatchData);
	InitMeshInfoBuffer(pRenderEnv, pBatchData, meshType, meshTypeOffset);
	InitInstanceAABBBuffer(pRenderEnv, pBatchData);
}

MeshBatch::~MeshBatch()
{
	RemoveDataForUpload();

	SafeDelete(m_pVertexBuffer);
	SafeDelete(m_pIndexBuffer);
	SafeDelete(m_pMeshInfoBuffer);
	SafeDelete(m_pInstanceAABBBuffer);
}

void MeshBatch::RecordDataForUpload(CommandList* pCommandList)
{
	pCommandList->CopyResource(m_pVertexBuffer, m_pUploadVertexBuffer);
	pCommandList->CopyResource(m_pIndexBuffer, m_pUploadIndexBuffer);
	pCommandList->CopyResource(m_pMeshInfoBuffer, m_pUploadMeshInfoBuffer);
	pCommandList->CopyResource(m_pInstanceAABBBuffer, m_pUploadInstanceAABBBuffer);
		
	const D3D12_RESOURCE_BARRIER resourceTransitions[] =
	{
		ResourceTransitionBarrier(m_pVertexBuffer, m_pVertexBuffer->GetState(), m_pVertexBuffer->GetReadState()),
		ResourceTransitionBarrier(m_pIndexBuffer, m_pIndexBuffer->GetState(), m_pIndexBuffer->GetReadState()),
		ResourceTransitionBarrier(m_pMeshInfoBuffer, m_pMeshInfoBuffer->GetState(), m_pMeshInfoBuffer->GetReadState()),
		ResourceTransitionBarrier(m_pInstanceAABBBuffer, m_pInstanceAABBBuffer->GetState(), m_pInstanceAABBBuffer->GetReadState())
	};
	pCommandList->ResourceBarrier(ARRAYSIZE(resourceTransitions), &resourceTransitions[0]);
	
	m_pVertexBuffer->SetState(m_pVertexBuffer->GetReadState());
	m_pIndexBuffer->SetState(m_pIndexBuffer->GetReadState());
	m_pMeshInfoBuffer->SetState(m_pMeshInfoBuffer->GetReadState());
	m_pInstanceAABBBuffer->SetState(m_pInstanceAABBBuffer->GetReadState());
}

void MeshBatch::RemoveDataForUpload()
{
	SafeDelete(m_pUploadVertexBuffer);
	SafeDelete(m_pUploadIndexBuffer);
	SafeDelete(m_pUploadInstanceAABBBuffer);
	SafeDelete(m_pUploadMeshInfoBuffer);
}

void MeshBatch::InitInputLayout(RenderEnv* pRenderEnv, const MeshBatchData* pBatchData)
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
	m_pInputLayout = new InputLayoutDesc(m_InputElements.size(), &m_InputElements[0]);
}

void MeshBatch::InitVertexBuffer(RenderEnv* pRenderEnv, const MeshBatchData* pBatchData)
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
	
	VertexBufferDesc bufferDesc(numVertices, m_VertexStrideInBytes);
	m_pVertexBuffer = new Buffer(pRenderEnv, pRenderEnv->m_pDefaultHeapProps, &bufferDesc, D3D12_RESOURCE_STATE_COPY_DEST, L"MeshBatch::m_pVertexBuffer");
	
	m_pUploadVertexBuffer = new Buffer(pRenderEnv, pRenderEnv->m_pUploadHeapProps, &bufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, L"MeshBatch::m_pUploadVertexBuffer");
	m_pUploadVertexBuffer->Write(pVertexData, sizeInBytes);
	
	SafeArrayDelete(pVertexData);
}

void MeshBatch::InitIndexBuffer(RenderEnv* pRenderEnv, const MeshBatchData* pBatchData)
{
	const u32 numIndices = pBatchData->GetNumIndices();
	const bool use16BitIndices = (pBatchData->GetIndexFormat() == DXGI_FORMAT_R16_UINT);

	const u32 strideInBytes = use16BitIndices ? sizeof(u16) : sizeof(u32);
	const u32 sizeInBytes = numIndices * strideInBytes;
	
	IndexBufferDesc bufferDesc(numIndices, strideInBytes);
	m_pIndexBuffer = new Buffer(pRenderEnv, pRenderEnv->m_pDefaultHeapProps, &bufferDesc, D3D12_RESOURCE_STATE_COPY_DEST, L"MeshBatch::m_pIndexBuffer");
	
	m_pUploadIndexBuffer = new Buffer(pRenderEnv, pRenderEnv->m_pUploadHeapProps, &bufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, L"MeshBatch::m_pUploadIndexBuffer");
	if (use16BitIndices)
		m_pUploadIndexBuffer->Write(pBatchData->Get16BitIndices(), sizeInBytes);
	else
		m_pUploadIndexBuffer->Write(pBatchData->Get32BitIndices(), sizeInBytes);
}

void MeshBatch::InitInstanceAABBBuffer(RenderEnv* pRenderEnv, const MeshBatchData* pBatchData)
{
	const u32 numInstances = pBatchData->GetNumMeshInstances();
	const u32 structureByteStride = sizeof(AxisAlignedBox);

	StructuredBufferDesc bufferDesc(numInstances, structureByteStride, true, false);
	m_pInstanceAABBBuffer = new Buffer(pRenderEnv, pRenderEnv->m_pDefaultHeapProps, &bufferDesc, D3D12_RESOURCE_STATE_COPY_DEST, L"MeshBatch::m_pInstanceAABBBuffer");

	m_pUploadInstanceAABBBuffer = new Buffer(pRenderEnv, pRenderEnv->m_pUploadHeapProps, &bufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, L"MeshBatch::m_pUploadInstanceAABBBuffer");
	m_pUploadInstanceAABBBuffer->Write(pBatchData->GetMeshInstanceAABBs(), numInstances * structureByteStride);
}

void MeshBatch::InitMeshInfoBuffer(RenderEnv* pRenderEnv, const MeshBatchData* pBatchData, u32 meshType, u32 meshTypeOffset)
{
	struct Data
	{
		Data(u32 meshType, u32 meshTypeOffset, u32 materialIndex, u32 indexCountPerInstance, u32 startIndexLocation, i32 baseVertexLocation)
			: m_MeshType(meshType)
			, m_MeshTypeOffset(meshTypeOffset)
			, m_MaterialIndex(materialIndex)
			, m_IndexCountPerInstance(indexCountPerInstance)
			, m_StartIndexLocation(startIndexLocation)
			, m_BaseVertexLocation(baseVertexLocation)
		{}
		u32 m_MeshType;
		u32 m_MeshTypeOffset;
		u32 m_MaterialIndex;
		u32 m_IndexCountPerInstance;
		u32 m_StartIndexLocation;
		i32 m_BaseVertexLocation;
	};

	const u32 numMeshes = pBatchData->GetNumMeshes();
	const u32 structureByteStride = sizeof(Data);

	std::vector<Data> dataBuffer;
	dataBuffer.reserve(numMeshes);

	const MeshInfo* pMeshInfos = pBatchData->GetMeshInfos();
	for (u32 index = 0; index < numMeshes; ++index)
	{
		dataBuffer.emplace_back(meshType,
			meshTypeOffset,
			pMeshInfos[index].m_MaterialIndex,
			pMeshInfos[index].m_IndexCountPerInstance,
			pMeshInfos[index].m_StartIndexLocation,
			pMeshInfos[index].m_BaseVertexLocation);
	}

	StructuredBufferDesc bufferDesc(numMeshes, structureByteStride, true, false);
	m_pMeshInfoBuffer = new Buffer(pRenderEnv, pRenderEnv->m_pDefaultHeapProps, &bufferDesc, D3D12_RESOURCE_STATE_COPY_DEST, L"MeshBatch::m_pMeshInfoBuffer");

	m_pUploadMeshInfoBuffer = new Buffer(pRenderEnv, pRenderEnv->m_pUploadHeapProps, &bufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, L"MeshBatch::m_pUploadMeshInfoBuffer");
	m_pUploadMeshInfoBuffer->Write(dataBuffer.data(), numMeshes * structureByteStride);
}

MeshRenderResources::MeshRenderResources(u32 numMeshTypes, const MeshBatchData* pFirstMeshTypeData)
	: m_NumMeshTypes(numMeshTypes)
	, m_pMeshInfoBuffer(nullptr)
	, m_pMeshInstanceRangeBuffer(nullptr)
	, m_pInstanceWorldMatrixBuffer(nullptr)
	, m_pInstanceWorldAABBBuffer(nullptr)
{
}

MeshRenderResources::~MeshRenderResources()
{
}
