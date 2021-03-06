#include "RenderPasses/MeshRenderResources.h"
#include "D3DWrapper/GraphicsResource.h"
#include "D3DWrapper/CommandList.h"
#include "D3DWrapper/PipelineState.h"
#include "D3DWrapper/RenderEnv.h"
#include "D3DWrapper/GraphicsUtils.h"
#include "Math/Vector2.h"
#include "Math/Vector3.h"
#include "Math/Vector4.h"
#include "Math/Transform.h"
#include "Scene/Mesh.h"
#include "Scene/MeshBatch.h"

namespace
{
	u32 CalcMaxNumInstancesPerMesh(u32 numMeshTypes, MeshBatch** ppFirstMeshType);
	const Matrix4f ExtractUnitAABBToWorldOBBTransform(const OrientedBox& worldOBB);
}

MeshRenderResources::MeshRenderResources(RenderEnv* pRenderEnv, u32 numMeshTypes, MeshBatch** ppFirstMeshType)
	: m_NumMeshTypes(numMeshTypes)
	, m_TotalNumMeshes(0)
	, m_TotalNumInstances(0)
	, m_MaxNumInstancesPerMesh(CalcMaxNumInstancesPerMesh(numMeshTypes, ppFirstMeshType))
	, m_pMeshInfoBuffer(nullptr)
	, m_pInstanceWorldMatrixBuffer(nullptr)
	, m_pInstanceWorldAABBBuffer(nullptr)
	, m_pInstanceWorldOBBMatrixBuffer(nullptr)
{
	InitPerMeshTypeResources(pRenderEnv, numMeshTypes, ppFirstMeshType);
	InitPerMeshResources(pRenderEnv, numMeshTypes, ppFirstMeshType);
	InitPerMeshInstanceResources(pRenderEnv, numMeshTypes, ppFirstMeshType);
}

MeshRenderResources::~MeshRenderResources()
{
	SafeDelete(m_pMeshInfoBuffer);
	SafeDelete(m_pInstanceWorldMatrixBuffer);
	SafeDelete(m_pInstanceWorldAABBBuffer);
	SafeDelete(m_pInstanceWorldOBBMatrixBuffer);

	for (u32 meshType = 0; meshType < m_NumMeshTypes; ++meshType)
	{
		SafeDelete(m_VertexBuffers[meshType]);
		SafeDelete(m_IndexBuffers[meshType]);
	}
}

void MeshRenderResources::InitPerMeshResources(RenderEnv* pRenderEnv, u32 numMeshTypes, MeshBatch** ppFirstMeshType)
{
	m_TotalNumMeshes = 0;
	for (u32 meshType = 0; meshType < numMeshTypes; ++meshType)
	{
		const MeshBatch* pMeshBatch = ppFirstMeshType[meshType];
		m_TotalNumMeshes += pMeshBatch->GetNumMeshes();
	}

	std::vector<MeshRenderInfo> meshInfoBufferData;
	meshInfoBufferData.reserve(m_TotalNumMeshes);
	
	u32 meshTypeOffset = 0;
	u32 instanceOffset = 0;

	for (u32 meshType = 0; meshType < numMeshTypes; ++meshType)
	{
		m_MeshTypeOffsets[meshType] = meshTypeOffset;
		const MeshBatch* pMeshBatch = ppFirstMeshType[meshType];

		const MeshInfo* pFirstMeshInfo = pMeshBatch->GetMeshInfos();
		for (u32 meshIndex = 0; meshIndex < pMeshBatch->GetNumMeshes(); ++meshIndex)
		{
			const MeshInfo& meshInfo = pFirstMeshInfo[meshIndex];
			const u32 materialID = meshInfo.m_MaterialID + 1;

			meshInfoBufferData.emplace_back(
				meshInfo.m_InstanceCount,
				instanceOffset,
				meshType,
				meshTypeOffset,
				materialID,
				meshInfo.m_IndexCount,
				meshInfo.m_StartIndexLocation,
				meshInfo.m_BaseVertexLocation);

			instanceOffset += meshInfo.m_InstanceCount;
		}
		meshTypeOffset += pMeshBatch->GetNumMeshes();
	}

	StructuredBufferDesc meshInfoBufferDesc(m_TotalNumMeshes, sizeof(MeshRenderInfo), true, false);
	m_pMeshInfoBuffer = new Buffer(pRenderEnv, pRenderEnv->m_pDefaultHeapProps, &meshInfoBufferDesc,
		D3D12_RESOURCE_STATE_COPY_DEST, L"MeshRenderResources::m_pMeshInfoBuffer");
	
	UploadData(pRenderEnv, m_pMeshInfoBuffer, meshInfoBufferDesc, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
		meshInfoBufferData.data(), m_TotalNumMeshes * sizeof(MeshRenderInfo));
}

void MeshRenderResources::InitPerMeshInstanceResources(RenderEnv* pRenderEnv, u32 numMeshTypes, MeshBatch** ppFirstMeshType)
{
	m_TotalNumInstances = 0;
	for (u32 meshType = 0; meshType < numMeshTypes; ++meshType)
	{
		const MeshBatch* pMeshBatch = ppFirstMeshType[meshType];
		m_TotalNumInstances += pMeshBatch->GetNumMeshInstances();
	}

	std::vector<AxisAlignedBox> instanceWorldAABBBufferData;
	instanceWorldAABBBufferData.reserve(m_TotalNumInstances);

	std::vector<Matrix4f> instanceWorldOBBMatrixBufferData;
	instanceWorldOBBMatrixBufferData.reserve(m_TotalNumInstances);

	std::vector<Matrix4f> instanceWorldMatrixBufferData;
	instanceWorldMatrixBufferData.reserve(m_TotalNumInstances);

	for (u32 meshType = 0; meshType < numMeshTypes; ++meshType)
	{
		const MeshBatch* pMeshBatch = ppFirstMeshType[meshType];
		const u32 numInstances = pMeshBatch->GetNumMeshInstances();

		const AxisAlignedBox* pFirstInstanceWorldAABB = pMeshBatch->GetMeshInstanceWorldAABBs();
		instanceWorldAABBBufferData.insert(
			instanceWorldAABBBufferData.end(),
			pFirstInstanceWorldAABB,
			pFirstInstanceWorldAABB + numInstances);

		const OrientedBox* pFirstInstanceWorldOBB = pMeshBatch->GetMeshInstanceWorldOBBs();
		for (u32 instanceIndex = 0; instanceIndex < numInstances; ++instanceIndex)
		{
			const OrientedBox& instanceWorldOBB = pFirstInstanceWorldOBB[instanceIndex];
			instanceWorldOBBMatrixBufferData.emplace_back(ExtractUnitAABBToWorldOBBTransform(instanceWorldOBB));
		}

		const Matrix4f* pFirstInstanceWorldMatrix = pMeshBatch->GetMeshInstanceWorldMatrices();
		instanceWorldMatrixBufferData.insert(
			instanceWorldMatrixBufferData.end(),
			pFirstInstanceWorldMatrix,
			pFirstInstanceWorldMatrix + numInstances);
	}

	StructuredBufferDesc instanceWorldAABBBufferDesc(m_TotalNumInstances, sizeof(AxisAlignedBox), true, false);
	m_pInstanceWorldAABBBuffer = new Buffer(pRenderEnv, pRenderEnv->m_pDefaultHeapProps,
		&instanceWorldAABBBufferDesc, D3D12_RESOURCE_STATE_COPY_DEST, L"MeshRenderResources::m_pInstanceWorldAABBBuffer");
	
	UploadData(pRenderEnv, m_pInstanceWorldAABBBuffer, instanceWorldAABBBufferDesc,
		D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, instanceWorldAABBBufferData.data(), m_TotalNumInstances * sizeof(AxisAlignedBox));

	StructuredBufferDesc instanceWorldOBBMatrixBufferDesc(m_TotalNumInstances, sizeof(Matrix4f), true, false);
	m_pInstanceWorldOBBMatrixBuffer = new Buffer(pRenderEnv, pRenderEnv->m_pDefaultHeapProps,
		&instanceWorldOBBMatrixBufferDesc, D3D12_RESOURCE_STATE_COPY_DEST, L"MeshRenderResources::m_pInstanceWorldOBBMatrixBuffer");
	
	UploadData(pRenderEnv, m_pInstanceWorldOBBMatrixBuffer, instanceWorldOBBMatrixBufferDesc,
		D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, instanceWorldOBBMatrixBufferData.data(), m_TotalNumInstances * sizeof(Matrix4f));

	StructuredBufferDesc instanceWorldMatrixBufferDesc(m_TotalNumInstances, sizeof(Matrix4f), true, false);
	m_pInstanceWorldMatrixBuffer = new Buffer(pRenderEnv, pRenderEnv->m_pDefaultHeapProps,
		&instanceWorldMatrixBufferDesc, D3D12_RESOURCE_STATE_COPY_DEST, L"MeshRenderResources::m_pInstanceWorldMatrixBuffer");
	
	UploadData(pRenderEnv, m_pInstanceWorldMatrixBuffer, instanceWorldMatrixBufferDesc,
		D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, instanceWorldMatrixBufferData.data(), m_TotalNumInstances * sizeof(Matrix4f));
}

void MeshRenderResources::InitPerMeshTypeResources(RenderEnv* pRenderEnv, u32 numMeshTypes, MeshBatch** ppFirstMeshType)
{
	m_MeshTypeOffsets.resize(numMeshTypes);
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
	m_InputLayouts[meshType] = InputLayoutDesc((UINT)m_InputElements[meshType].size(), m_InputElements[meshType].data());
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

	StructuredBufferDesc bufferDesc(numVertices, m_VertexStrideInBytes[meshType], false, false, true);
	m_VertexBuffers[meshType] = new Buffer(pRenderEnv, pRenderEnv->m_pDefaultHeapProps, &bufferDesc,
		D3D12_RESOURCE_STATE_COPY_DEST, L"MeshRenderResources::m_pVertexBuffer");
	
	UploadData(pRenderEnv, m_VertexBuffers[meshType], bufferDesc,
		D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, pVertexData, sizeInBytes);
	
	SafeArrayDelete(pVertexData);
}

void MeshRenderResources::InitIndexBuffer(RenderEnv* pRenderEnv, u32 meshType, const MeshBatch* pMeshBatch)
{
	const u32 numIndices = pMeshBatch->GetNumIndices();
			
	u32 sizeInBytes = 0;
	const void* pIndexData = nullptr;
	
	if (pMeshBatch->GetIndexFormat() == DXGI_FORMAT_R16_UINT)
	{
		sizeInBytes = numIndices * sizeof(u16);
		pIndexData = pMeshBatch->Get16BitIndices();
	}
	else
	{
		sizeInBytes = numIndices * sizeof(u32);
		pIndexData = pMeshBatch->Get32BitIndices();
	}

	FormattedBufferDesc bufferDesc(numIndices, pMeshBatch->GetIndexFormat(), false, false, true);
	m_IndexBuffers[meshType] = new Buffer(pRenderEnv, pRenderEnv->m_pDefaultHeapProps, &bufferDesc,
		D3D12_RESOURCE_STATE_COPY_DEST, L"MeshRenderResources::m_pIndexBuffer");

	UploadData(pRenderEnv, m_IndexBuffers[meshType], bufferDesc,
		D3D12_RESOURCE_STATE_INDEX_BUFFER, pIndexData, sizeInBytes);
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

	const Matrix4f ExtractUnitAABBToWorldOBBTransform(const OrientedBox& worldOBB)
	{
		f32 xScale = AreEqual(worldOBB.m_Radius.m_X, 0.0f, EPSILON) ? 1.0f : worldOBB.m_Radius.m_X;
		f32 yScale = AreEqual(worldOBB.m_Radius.m_Y, 0.0f, EPSILON) ? 1.0f : worldOBB.m_Radius.m_Y;
		f32 zScale = AreEqual(worldOBB.m_Radius.m_Z, 0.0f, EPSILON) ? 1.0f : worldOBB.m_Radius.m_Z;

		Matrix4f scalingMatrix = CreateScalingMatrix(xScale, yScale, zScale);
		Matrix4f rotationMatrix = CreateRotationMatrix(worldOBB.m_Orientation);
		Matrix4f translationMatrix = CreateTranslationMatrix(worldOBB.m_Center);

		return (scalingMatrix * rotationMatrix * translationMatrix);
	}
}
