#include "Common/MeshRenderResources.h"
#include "Common/Mesh.h"
#include "Common/MeshBatch.h"
#include "D3DWrapper/GraphicsResource.h"
#include "D3DWrapper/CommandList.h"
#include "D3DWrapper/PipelineState.h"
#include "D3DWrapper/RenderEnv.h"
#include "D3DWrapper/GraphicsUtils.h"
#include "Math/Vector2.h"
#include "Math/Vector3.h"
#include "Math/Vector4.h"

namespace
{
	u32 CalcMaxNumInstancesPerMesh(u32 numMeshTypes, MeshBatch** ppFirstMeshType);
}

MeshRenderResources::MeshRenderResources(RenderEnv* pRenderEnv, u32 numMeshTypes, MeshBatch** ppFirstMeshType)
	: m_NumMeshTypes(numMeshTypes)
	, m_TotalNumMeshes(0)
	, m_TotalNumInstances(0)
	, m_MaxNumInstancesPerMesh(CalcMaxNumInstancesPerMesh(numMeshTypes, ppFirstMeshType))
	, m_pMeshInfoBuffer(nullptr)
	, m_pMeshInstanceRangeBuffer(nullptr)
	, m_pInstanceWorldMatrixBuffer(nullptr)
	, m_pInstanceWorldAABBBuffer(nullptr)
{
	InitPerMeshResources(pRenderEnv, numMeshTypes, ppFirstMeshType);
	InitPerMeshInstanceResources(pRenderEnv, numMeshTypes, ppFirstMeshType);
	InitPerMeshTypeResources(pRenderEnv, numMeshTypes, ppFirstMeshType);
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

void MeshRenderResources::InitPerMeshResources(RenderEnv* pRenderEnv, u32 numMeshTypes, MeshBatch** ppFirstMeshType)
{
	struct MeshInstanceRange
	{
		MeshInstanceRange(u32 instanceOffset, u32 numInstances, u32 meshIndex)
			: m_InstanceOffset(instanceOffset)
			, m_NumInstances(numInstances)
			, m_MeshIndex(meshIndex)
		{}
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
		{}
		u32 m_MeshType;
		u32 m_MeshTypeOffset;
		u32 m_MaterialIndex;
		u32 m_IndexCountPerInstance;
		u32 m_StartIndexLocation;
		i32 m_BaseVertexLocation;
	};

	m_TotalNumMeshes = 0;
	for (u32 meshType = 0; meshType < numMeshTypes; ++meshType)
	{
		const MeshBatch* pMeshBatch = ppFirstMeshType[meshType];
		m_TotalNumMeshes += pMeshBatch->GetNumMeshes();
	}

	std::vector<MeshRenderInfo> meshInfoBufferData;
	meshInfoBufferData.reserve(m_TotalNumMeshes);

	std::vector<MeshInstanceRange> meshInstanceRangeBufferData;
	meshInstanceRangeBufferData.reserve(m_TotalNumMeshes);

	u32 meshTypeOffset = 0;
	u32 instanceOffset = 0;

	for (u32 meshType = 0; meshType < numMeshTypes; ++meshType)
	{
		const MeshBatch* pMeshBatch = ppFirstMeshType[meshType];

		const MeshInfo* pFirstMeshInfo = pMeshBatch->GetMeshInfos();
		for (u32 meshIndex = 0; meshIndex < pMeshBatch->GetNumMeshes(); ++meshIndex)
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

		meshTypeOffset += pMeshBatch->GetNumMeshes();
	}

	StructuredBufferDesc meshInfoBufferDesc(m_TotalNumMeshes, sizeof(MeshRenderInfo), true, false);
	m_pMeshInfoBuffer = new Buffer(pRenderEnv, pRenderEnv->m_pDefaultHeapProps, &meshInfoBufferDesc, D3D12_RESOURCE_STATE_COPY_DEST, L"MeshRenderResources::m_pMeshInfoBuffer");
	UploadData(pRenderEnv, m_pMeshInfoBuffer, &meshInfoBufferDesc, meshInfoBufferData.data(), m_TotalNumMeshes * sizeof(MeshRenderInfo));

	StructuredBufferDesc meshInstanceRangeBufferDesc(m_TotalNumMeshes, sizeof(MeshInstanceRange), true, false);
	m_pMeshInstanceRangeBuffer = new Buffer(pRenderEnv, pRenderEnv->m_pDefaultHeapProps, &meshInstanceRangeBufferDesc, D3D12_RESOURCE_STATE_COPY_DEST, L"MeshRenderResources::m_pMeshInstanceRangeBuffer");
	UploadData(pRenderEnv, m_pMeshInstanceRangeBuffer, &meshInstanceRangeBufferDesc, meshInstanceRangeBufferData.data(), m_TotalNumMeshes * sizeof(MeshInstanceRange));
}

void MeshRenderResources::InitPerMeshInstanceResources(RenderEnv* pRenderEnv, u32 numMeshTypes, MeshBatch** ppFirstMeshType)
{
	m_TotalNumInstances = 0;
	for (u32 meshType = 0; meshType < numMeshTypes; ++meshType)
	{
		const MeshBatch* pMeshBatch = ppFirstMeshType[meshType];
		m_TotalNumInstances += pMeshBatch->GetNumMeshInstances();
	}

	std::vector<AxisAlignedBox> instanceAABBBufferData;
	instanceAABBBufferData.reserve(m_TotalNumInstances);

	std::vector<Matrix4f> instanceWorldMatrixBufferData;
	instanceWorldMatrixBufferData.reserve(m_TotalNumInstances);

	for (u32 meshType = 0; meshType < numMeshTypes; ++meshType)
	{
		const MeshBatch* pMeshBatch = ppFirstMeshType[meshType];

		const AxisAlignedBox* pFirstInstanceWorldAABB = pMeshBatch->GetMeshInstanceWorldAABBs();
		instanceAABBBufferData.insert(
			instanceAABBBufferData.end(),
			pFirstInstanceWorldAABB,
			pFirstInstanceWorldAABB + pMeshBatch->GetNumMeshInstances());

		const Matrix4f* pFirstInstanceWorldMatrix = pMeshBatch->GetMeshInstanceWorldMatrices();
		instanceWorldMatrixBufferData.insert(
			instanceWorldMatrixBufferData.end(),
			pFirstInstanceWorldMatrix,
			pFirstInstanceWorldMatrix + pMeshBatch->GetNumMeshInstances());
	}

	StructuredBufferDesc instanceAABBBufferDesc(m_TotalNumInstances, sizeof(AxisAlignedBox), true, false);
	m_pInstanceWorldAABBBuffer = new Buffer(pRenderEnv, pRenderEnv->m_pDefaultHeapProps, &instanceAABBBufferDesc, D3D12_RESOURCE_STATE_COPY_DEST, L"MeshRenderResources::m_pInstanceWorldAABBBuffer");
	UploadData(pRenderEnv, m_pInstanceWorldAABBBuffer, &instanceAABBBufferDesc, instanceAABBBufferData.data(), m_TotalNumInstances * sizeof(AxisAlignedBox));

	StructuredBufferDesc instanceWorldMatrixBufferDesc(m_TotalNumInstances, sizeof(Matrix4f), true, false);
	m_pInstanceWorldMatrixBuffer = new Buffer(pRenderEnv, pRenderEnv->m_pDefaultHeapProps, &instanceWorldMatrixBufferDesc, D3D12_RESOURCE_STATE_COPY_DEST, L"MeshRenderResources::m_pInstanceWorldMatrixBuffer");
	UploadData(pRenderEnv, m_pInstanceWorldMatrixBuffer, &instanceWorldMatrixBufferDesc, instanceWorldMatrixBufferData.data(), m_TotalNumInstances * sizeof(Matrix4f));
}

void MeshRenderResources::InitPerMeshTypeResources(RenderEnv* pRenderEnv, u32 numMeshTypes, MeshBatch** ppFirstMeshType)
{
	m_VertexStrideInBytes.resize(numMeshTypes);
	m_InputElements.resize(numMeshTypes);
	m_InputLayouts.resize(numMeshTypes);
	m_PrimitiveTopologyTypes.resize(numMeshTypes);
	m_PrimitiveTopologies.resize(numMeshTypes);
	m_VertexBuffers.resize(numMeshTypes);
	m_IndexBuffers.resize(numMeshTypes);

	for (u32 meshType = 0; meshType < numMeshTypes; ++meshType)
	{
		const MeshBatch* pMeshBatch = ppFirstMeshType[meshType];

		m_PrimitiveTopologyTypes[meshType] = pMeshBatch->GetPrimitiveTopologyType();
		m_PrimitiveTopologies[meshType] = pMeshBatch->GetPrimitiveTopology();

		InitInputLayout(pRenderEnv, meshType, pMeshBatch);
		InitVertexBuffer(pRenderEnv, meshType, pMeshBatch);
		InitIndexBuffer(pRenderEnv, meshType, pMeshBatch);
	}
}

void MeshRenderResources::InitInputLayout(RenderEnv* pRenderEnv, u32 meshType, const MeshBatch* pMeshBatch)
{
	const u8 vertexFormatFlags = pMeshBatch->GetVertexFormatFlags();

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

void MeshRenderResources::InitVertexBuffer(RenderEnv* pRenderEnv, u32 meshType, const MeshBatch* pMeshBatch)
{
	const u32 numVertices = pMeshBatch->GetNumVertices();
	const u8 vertexFormatFlags = pMeshBatch->GetVertexFormatFlags();

	assert(m_VertexStrideInBytes[meshType] > 0);
	const u32 sizeInBytes = numVertices * m_VertexStrideInBytes[meshType];

	u8* pVertexData = new u8[sizeInBytes];
	u32 vertexOffset = 0;

	assert((vertexFormatFlags & VertexData::FormatFlag_Position) != 0);
	{
		const Vector3f* pPositions = pMeshBatch->GetPositions();
		const u32 elementSizeInBytes = sizeof(pPositions[0]);

		for (u32 index = 0; index < numVertices; ++index)
			std::memcpy(pVertexData + index * m_VertexStrideInBytes[meshType] + vertexOffset, &pPositions[index], elementSizeInBytes);

		vertexOffset += elementSizeInBytes;
	}
	if ((vertexFormatFlags & VertexData::FormatFlag_Normal) != 0)
	{
		const Vector3f* pNormals = pMeshBatch->GetNormals();
		const u32 elementSizeInBytes = sizeof(pNormals[0]);

		for (u32 index = 0; index < numVertices; ++index)
			std::memcpy(pVertexData + index * m_VertexStrideInBytes[meshType] + vertexOffset, &pNormals[index], elementSizeInBytes);

		vertexOffset += elementSizeInBytes;
	}
	if ((vertexFormatFlags & VertexData::FormatFlag_Color) != 0)
	{
		const Vector4f* pColors = pMeshBatch->GetColors();
		const u32 elementSizeInBytes = sizeof(pColors[0]);

		for (u32 index = 0; index < numVertices; ++index)
			std::memcpy(pVertexData + index * m_VertexStrideInBytes[meshType] + vertexOffset, &pColors[index], elementSizeInBytes);

		vertexOffset += elementSizeInBytes;
	}
	if ((vertexFormatFlags & VertexData::FormatFlag_Tangent) != 0)
	{
		const Vector3f* pTangents = pMeshBatch->GetTangents();
		const u32 elementSizeInBytes = sizeof(pTangents[0]);

		for (u32 index = 0; index < numVertices; ++index)
			std::memcpy(pVertexData + index * m_VertexStrideInBytes[meshType] + vertexOffset, &pTangents[index], elementSizeInBytes);

		vertexOffset += elementSizeInBytes;
	}
	if ((vertexFormatFlags & VertexData::FormatFlag_TexCoords) != 0)
	{
		const Vector2f* pTexCoords = pMeshBatch->GetTexCoords();
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

void MeshRenderResources::InitIndexBuffer(RenderEnv* pRenderEnv, u32 meshType, const MeshBatch* pMeshBatch)
{
	const u32 numIndices = pMeshBatch->GetNumIndices();
	const bool use16BitIndices = pMeshBatch->GetIndexFormat() == DXGI_FORMAT_R16_UINT;

	const u32 strideInBytes = use16BitIndices ? sizeof(u16) : sizeof(u32);
	const u32 sizeInBytes = numIndices * strideInBytes;

	IndexBufferDesc bufferDesc(numIndices, strideInBytes);
	m_IndexBuffers[meshType] = new Buffer(pRenderEnv, pRenderEnv->m_pDefaultHeapProps, &bufferDesc, D3D12_RESOURCE_STATE_COPY_DEST, L"MeshRenderResources::m_pIndexBuffer");
	
	const void* pIndexData = nullptr;
	if (use16BitIndices)
		pIndexData = pMeshBatch->Get16BitIndices();
	else
		pIndexData = pMeshBatch->Get32BitIndices();
	
	UploadData(pRenderEnv, m_IndexBuffers[meshType], &bufferDesc, pIndexData, sizeInBytes);
}

namespace
{
	u32 CalcMaxNumInstancesPerMesh(u32 numMeshTypes, MeshBatch** ppFirstMeshType)
	{
		u32 maxNumInstancesPerMesh = ppFirstMeshType[0]->GetMaxNumInstancesPerMesh();
		for (u32 meshType = 1; meshType < numMeshTypes; ++meshType)
			maxNumInstancesPerMesh = Max(maxNumInstancesPerMesh, ppFirstMeshType[meshType]->GetMaxNumInstancesPerMesh());

		return maxNumInstancesPerMesh;
	}
}
