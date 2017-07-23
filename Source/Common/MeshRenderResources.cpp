#include "Common/MeshRenderResources.h"
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

MeshRenderResources::MeshRenderResources(RenderEnv* pRenderEnv, u32 numMeshTypes, const MeshBatchData* pFirstMeshTypeData)
	: m_NumMeshTypes(numMeshTypes)
	, m_pMeshInfoBuffer(nullptr)
	, m_pMeshInstanceRangeBuffer(nullptr)
	, m_pInstanceWorldMatrixBuffer(nullptr)
	, m_pInstanceWorldAABBBuffer(nullptr)
{
	InitPerMeshResources(pRenderEnv, numMeshTypes, pFirstMeshTypeData);
	InitPerMeshInstanceResources(pRenderEnv, numMeshTypes, pFirstMeshTypeData);
	InitPerMeshTypeResources(pRenderEnv, numMeshTypes, pFirstMeshTypeData);
}

MeshRenderResources::~MeshRenderResources()
{
	SafeDelete(m_pMeshInfoBuffer);
	SafeDelete(m_pMeshInstanceRangeBuffer);
	SafeDelete(m_pInstanceWorldMatrixBuffer);
	SafeDelete(m_pInstanceWorldAABBBuffer);

	for (u32 meshType = 0; meshType < m_NumMeshTypes; ++meshType)
	{
		SafeDelete(m_VertexBuffers[meshType]);
		SafeDelete(m_IndexBuffers[meshType]);
	}
}

void MeshRenderResources::InitPerMeshResources(RenderEnv* pRenderEnv, u32 numMeshTypes, const MeshBatchData* pFirstMeshTypeData)
{
	struct MeshInstanceRange
	{
		MeshInstanceRange(u32 instanceOffset, u32 numInstances, u32 meshIndex)
			: m_InstanceOffset(instanceOffset)
			, m_NumInstances(numInstances)
			, m_MeshIndex(meshIndex)
		{
		}
		u32 m_InstanceOffset;
		u32 m_NumInstances;
		u32 m_MeshIndex;
	};

	struct MeshRenderInfo
	{
		MeshRenderInfo(u32 meshType, u32 meshTypeOffset, u32 materialIndex, u32 indexCountPerInstance, u32 startIndexLocation, i32 baseVertexLocation)
			: m_MeshType(meshType)
			, m_MeshTypeOffset(meshTypeOffset)
			, m_MaterialIndex(materialIndex)
			, m_IndexCountPerInstance(indexCountPerInstance)
			, m_StartIndexLocation(startIndexLocation)
			, m_BaseVertexLocation(baseVertexLocation)
		{
		}
		u32 m_MeshType;
		u32 m_MeshTypeOffset;
		u32 m_MaterialIndex;
		u32 m_IndexCountPerInstance;
		u32 m_StartIndexLocation;
		i32 m_BaseVertexLocation;
	};

	u32 numAllMeshes = 0;
	for (u32 meshType = 0; meshType < numMeshTypes; ++meshType)
	{
		const MeshBatchData& meshBatch = pFirstMeshTypeData[meshType];
		numAllMeshes += meshBatch.GetNumMeshes();
	}

	std::vector<MeshRenderInfo> meshInfoBufferData;
	meshInfoBufferData.reserve(numAllMeshes);

	std::vector<MeshInstanceRange> meshInstanceRangeBufferData;
	meshInstanceRangeBufferData.reserve(numAllMeshes);

	u32 meshTypeOffset = 0;
	u32 instanceOffset = 0;

	for (u32 meshType = 0; meshType < numMeshTypes; ++meshType)
	{
		const MeshBatchData& meshBatch = pFirstMeshTypeData[meshType];

		const MeshInfo* pFirstMeshInfo = meshBatch.GetMeshInfos();
		for (u32 meshIndex = 0; meshIndex < meshBatch.GetNumMeshes(); ++meshIndex)
		{
			u32 globalMeshIndex = meshInfoBufferData.size();
			const MeshInfo& meshInfo = pFirstMeshInfo[meshIndex];

			meshInfoBufferData.emplace_back(
				meshType,
				meshTypeOffset,
				meshInfo.m_MaterialIndex,
				meshInfo.m_IndexCountPerInstance,
				meshInfo.m_StartIndexLocation,
				meshInfo.m_BaseVertexLocation);

			meshInstanceRangeBufferData.emplace_back(
				instanceOffset,
				meshInfo.m_InstanceCount,
				globalMeshIndex);

			instanceOffset += meshInfo.m_InstanceCount;
		}

		meshTypeOffset += meshBatch.GetNumMeshes();
	}

	StructuredBufferDesc meshInfoBufferDesc(numAllMeshes, sizeof(MeshRenderInfo), true, false);
	m_pMeshInfoBuffer = new Buffer(pRenderEnv, pRenderEnv->m_pDefaultHeapProps, &meshInfoBufferDesc, D3D12_RESOURCE_STATE_COPY_DEST, L"MeshRenderResources::m_pMeshInfoBuffer");
	UploadData(pRenderEnv, m_pMeshInfoBuffer, &meshInfoBufferDesc, meshInfoBufferData.data(), numAllMeshes * sizeof(MeshRenderInfo));

	StructuredBufferDesc meshInstanceRangeBufferDesc(numAllMeshes, sizeof(MeshInstanceRange), true, false);
	m_pMeshInstanceRangeBuffer = new Buffer(pRenderEnv, pRenderEnv->m_pDefaultHeapProps, &meshInstanceRangeBufferDesc, D3D12_RESOURCE_STATE_COPY_DEST, L"MeshRenderResources::m_pMeshInstanceRangeBuffer");
	UploadData(pRenderEnv, m_pMeshInstanceRangeBuffer, &meshInstanceRangeBufferDesc, meshInstanceRangeBufferData.data(), numAllMeshes * sizeof(MeshInstanceRange));
}

void MeshRenderResources::InitPerMeshInstanceResources(RenderEnv* pRenderEnv, u32 numMeshTypes, const MeshBatchData* pFirstMeshTypeData)
{
	u32 numAllInstances = 0;
	for (u32 meshType = 0; meshType < numMeshTypes; ++meshType)
	{
		const MeshBatchData& meshBatch = pFirstMeshTypeData[meshType];
		numAllInstances += meshBatch.GetNumMeshInstances();
	}

	std::vector<AxisAlignedBox> instanceAABBBufferData;
	instanceAABBBufferData.reserve(numAllInstances);

	std::vector<Matrix4f> instanceWorldMatrixBufferData;
	instanceWorldMatrixBufferData.reserve(numAllInstances);

	for (u32 meshType = 0; meshType < numMeshTypes; ++meshType)
	{
		const MeshBatchData& meshBatch = pFirstMeshTypeData[meshType];

		const AxisAlignedBox* pFirstInstanceWorldAABB = meshBatch.GetMeshInstanceWorldAABBs();
		instanceAABBBufferData.insert(
			instanceAABBBufferData.end(),
			pFirstInstanceWorldAABB,
			pFirstInstanceWorldAABB + meshBatch.GetNumMeshInstances());

		const Matrix4f* pFirstInstanceWorldMatrix = meshBatch.GetMeshInstanceWorldMatrices();
		instanceWorldMatrixBufferData.insert(
			instanceWorldMatrixBufferData.end(),
			pFirstInstanceWorldMatrix,
			pFirstInstanceWorldMatrix + meshBatch.GetNumMeshInstances());
	}

	StructuredBufferDesc instanceAABBBufferDesc(numAllInstances, sizeof(AxisAlignedBox), true, false);
	m_pInstanceWorldAABBBuffer = new Buffer(pRenderEnv, pRenderEnv->m_pDefaultHeapProps, &instanceAABBBufferDesc, D3D12_RESOURCE_STATE_COPY_DEST, L"MeshRenderResources::m_pInstanceWorldAABBBuffer");
	UploadData(pRenderEnv, m_pInstanceWorldAABBBuffer, &instanceAABBBufferDesc, instanceAABBBufferData.data(), numAllInstances * sizeof(AxisAlignedBox));

	StructuredBufferDesc instanceWorldMatrixBufferDesc(numAllInstances, sizeof(Matrix4f), true, false);
	m_pInstanceWorldMatrixBuffer = new Buffer(pRenderEnv, pRenderEnv->m_pDefaultHeapProps, &instanceWorldMatrixBufferDesc, D3D12_RESOURCE_STATE_COPY_DEST, L"MeshRenderResources::m_pInstanceWorldMatrixBuffer");
	UploadData(pRenderEnv, m_pInstanceWorldMatrixBuffer, &instanceWorldMatrixBufferDesc, instanceWorldMatrixBufferData.data(), numAllInstances * sizeof(Matrix4f));
}

void MeshRenderResources::InitPerMeshTypeResources(RenderEnv* pRenderEnv, u32 numMeshTypes, const MeshBatchData* pFirstMeshTypeData)
{
	m_VertexStrideInBytes.resize(numMeshTypes);
	m_InputLayouts.resize(numMeshTypes);
	m_PrimitiveTopologyTypes.resize(numMeshTypes);
	m_PrimitiveTopologies.resize(numMeshTypes);
	m_VertexBuffers.resize(numMeshTypes);
	m_IndexBuffers.resize(numMeshTypes);

	for (u32 meshType = 0; meshType < numMeshTypes; ++meshType)
	{
		const MeshBatchData& batchData = pFirstMeshTypeData[meshType];

		m_PrimitiveTopologyTypes[meshType] = batchData.GetPrimitiveTopologyType();
		m_PrimitiveTopologies[meshType] = batchData.GetPrimitiveTopology();

		InitInputLayout(pRenderEnv, meshType, batchData);
		InitVertexBuffer(pRenderEnv, meshType, batchData);
		InitIndexBuffer(pRenderEnv, meshType, batchData);
	}
}

void MeshRenderResources::InitInputLayout(RenderEnv* pRenderEnv, u32 meshType, const MeshBatchData& batchData)
{
	const u8 vertexFormatFlags = batchData.GetVertexFormatFlags();

	assert(m_InputElements[meshType].empty());
	m_InputElements[meshType].reserve(5);

	u32 byteOffset = 0;
	assert((vertexFormatFlags & VertexData::FormatFlag_Position) != 0);
	{
		const DXGI_FORMAT format = DXGI_FORMAT_R32G32B32_FLOAT;
		m_InputElements[meshType].emplace_back("POSITION", 0, format, 0, byteOffset);

		byteOffset += GetSizeInBytes(format);
	}
	if ((vertexFormatFlags & VertexData::FormatFlag_Normal) != 0)
	{
		const DXGI_FORMAT format = DXGI_FORMAT_R32G32B32_FLOAT;
		m_InputElements[meshType].emplace_back("NORMAL", 0, format, 0, byteOffset);

		byteOffset += GetSizeInBytes(format);
	}
	if ((vertexFormatFlags & VertexData::FormatFlag_Color) != 0)
	{
		const DXGI_FORMAT format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		m_InputElements[meshType].emplace_back("COLOR", 0, format, 0, byteOffset);

		byteOffset += GetSizeInBytes(format);
	}
	if ((vertexFormatFlags & VertexData::FormatFlag_Tangent) != 0)
	{
		const DXGI_FORMAT format = DXGI_FORMAT_R32G32B32_FLOAT;
		m_InputElements[meshType].emplace_back("TANGENT", 0, format, 0, byteOffset);

		byteOffset += GetSizeInBytes(format);
	}
	if ((vertexFormatFlags & VertexData::FormatFlag_TexCoords) != 0)
	{
		const DXGI_FORMAT format = DXGI_FORMAT_R32G32_FLOAT;
		m_InputElements[meshType].emplace_back("TEXCOORD", 0, format, 0, byteOffset);

		byteOffset += GetSizeInBytes(format);
	}

	m_VertexStrideInBytes[meshType] = byteOffset;
	m_InputLayouts[meshType] = InputLayoutDesc(m_InputElements[meshType].size(), m_InputElements[meshType].data());
}

void MeshRenderResources::InitVertexBuffer(RenderEnv* pRenderEnv, u32 meshType, const MeshBatchData& batchData)
{
	const u32 numVertices = batchData.GetNumVertices();
	const u8 vertexFormatFlags = batchData.GetVertexFormatFlags();

	assert(m_VertexStrideInBytes[meshType] > 0);
	const u32 sizeInBytes = numVertices * m_VertexStrideInBytes[meshType];

	u8* pVertexData = new u8[sizeInBytes];
	u32 vertexOffset = 0;

	assert((vertexFormatFlags & VertexData::FormatFlag_Position) != 0);
	{
		const Vector3f* pPositions = batchData.GetPositions();
		const u32 elementSizeInBytes = sizeof(pPositions[0]);

		for (u32 index = 0; index < numVertices; ++index)
			std::memcpy(pVertexData + index * m_VertexStrideInBytes[meshType] + vertexOffset, &pPositions[index], elementSizeInBytes);

		vertexOffset += elementSizeInBytes;
	}
	if ((vertexFormatFlags & VertexData::FormatFlag_Normal) != 0)
	{
		const Vector3f* pNormals = batchData.GetNormals();
		const u32 elementSizeInBytes = sizeof(pNormals[0]);

		for (u32 index = 0; index < numVertices; ++index)
			std::memcpy(pVertexData + index * m_VertexStrideInBytes[meshType] + vertexOffset, &pNormals[index], elementSizeInBytes);

		vertexOffset += elementSizeInBytes;
	}
	if ((vertexFormatFlags & VertexData::FormatFlag_Color) != 0)
	{
		const Vector4f* pColors = batchData.GetColors();
		const u32 elementSizeInBytes = sizeof(pColors[0]);

		for (u32 index = 0; index < numVertices; ++index)
			std::memcpy(pVertexData + index * m_VertexStrideInBytes[meshType] + vertexOffset, &pColors[index], elementSizeInBytes);

		vertexOffset += elementSizeInBytes;
	}
	if ((vertexFormatFlags & VertexData::FormatFlag_Tangent) != 0)
	{
		const Vector3f* pTangents = batchData.GetTangents();
		const u32 elementSizeInBytes = sizeof(pTangents[0]);

		for (u32 index = 0; index < numVertices; ++index)
			std::memcpy(pVertexData + index * m_VertexStrideInBytes[meshType] + vertexOffset, &pTangents[index], elementSizeInBytes);

		vertexOffset += elementSizeInBytes;
	}
	if ((vertexFormatFlags & VertexData::FormatFlag_TexCoords) != 0)
	{
		const Vector2f* pTexCoords = batchData.GetTexCoords();
		const u32 elementSizeInBytes = sizeof(pTexCoords[0]);

		for (u32 index = 0; index < numVertices; ++index)
			std::memcpy(pVertexData + index * m_VertexStrideInBytes[meshType] + vertexOffset, &pTexCoords[index], elementSizeInBytes);

		vertexOffset += elementSizeInBytes;
	}

	VertexBufferDesc bufferDesc(numVertices, m_VertexStrideInBytes[meshType]);
	m_VertexBuffers[meshType] = new Buffer(pRenderEnv, pRenderEnv->m_pDefaultHeapProps, &bufferDesc, D3D12_RESOURCE_STATE_COPY_DEST, L"MeshRenderResources::m_pVertexBuffer");
	UploadData(pRenderEnv, m_VertexBuffers[meshType], &bufferDesc, pVertexData, sizeInBytes);	
	
	SafeArrayDelete(pVertexData);
}

void MeshRenderResources::InitIndexBuffer(RenderEnv* pRenderEnv, u32 meshType, const MeshBatchData& batchData)
{
	const u32 numIndices = batchData.GetNumIndices();
	const bool use16BitIndices = batchData.GetIndexFormat() == DXGI_FORMAT_R16_UINT;

	const u32 strideInBytes = use16BitIndices ? sizeof(u16) : sizeof(u32);
	const u32 sizeInBytes = numIndices * strideInBytes;

	IndexBufferDesc bufferDesc(numIndices, strideInBytes);
	m_IndexBuffers[meshType] = new Buffer(pRenderEnv, pRenderEnv->m_pDefaultHeapProps, &bufferDesc, D3D12_RESOURCE_STATE_COPY_DEST, L"MeshRenderResources::m_pIndexBuffer");
	
	const void* pIndexData = nullptr;
	if (use16BitIndices)
		pIndexData = batchData.Get16BitIndices();
	else
		pIndexData = batchData.Get32BitIndices();
	
	UploadData(pRenderEnv, m_IndexBuffers[meshType], &bufferDesc, pIndexData, sizeInBytes);
}
