#include "Common/MeshDataUtilities.h"
#include "Common/MeshData.h"
#include "Common/Mesh.h"
#include "Math/Vector2f.h"
#include "Math/Vector3f.h"
#include "Math/Vector4f.h"

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

	template <typename Index>
	void FlipIndicesWindingOrder(u32 numIndices, Index* pIndices)
	{
		assert((numIndices % 3) == 0);
		for (u32 offset = 0; offset < numIndices; offset += 3)
			std::swap(pIndices[offset], pIndices[offset + 2]);
	}
}

void ConvertMeshData(MeshData* pMeshData, u8 convertionFlags)
{
	const u32 numVertices = pMeshData->GetNumVertices();

	if (convertionFlags & ConvertionFlag_FlipZCoord)
	{
		Vector3f* pPositions = pMeshData->GetPositions();
		assert(pPositions != nullptr);
		{
			for (u32 index = 0; index < numVertices; ++index)
				pPositions[index].m_Z *= -1.0f;
		}

		Vector3f* pNormals = pMeshData->GetNormals();
		if (pNormals != nullptr)
		{
			for (u32 index = 0; index < numVertices; ++index)
				pNormals[index].m_Z *= -1.0f;
		}

		Vector3f* pTangents = pMeshData->GetTangents();
		if (pTangents != nullptr)
		{
			for (u32 index = 0; index < numVertices; ++index)
				pTangents[index].m_Z *= -1.0f;
		}

		Vector3f* pBiTangents = pMeshData->GetBiTangents();
		if (pBiTangents != nullptr)
		{
			for (u32 index = 0; index < numVertices; ++index)
			{
				pBiTangents[index].m_X *= -1.0f;
				pBiTangents[index].m_Y *= -1.0f;
			}			
		}
	}
	
	if (convertionFlags & ConvertionFlag_FlipTexCoords)
	{
		Vector2f* pTexCoords = pMeshData->GetTexCoords();
		if (pTexCoords != nullptr)
		{
			for (u32 index = 0; index < numVertices; ++index)
				pTexCoords[index].m_Y = 1.0f - pTexCoords[index].m_Y;
		}
	}

	if (convertionFlags & ConvertionFlag_FlipWindingOrder)
	{
		const u32 numIndices = pMeshData->GetNumIndices();

		u16* pIndices16Bit = pMeshData->Get16BitIndices();
		if (pIndices16Bit != nullptr)
		{
			FlipIndicesWindingOrder(numIndices, pIndices16Bit);
		}
		else
		{
			u32* pIndices32Bit = pMeshData->Get32BitIndices();
			if (pIndices32Bit != nullptr)
			{
				FlipIndicesWindingOrder(numIndices, pIndices32Bit);
			}
			else
			{
				assert(false);
			}
		} 
	}
}

void GenerateInputElements(std::vector<DXInputElementDesc>& inputElements, u8 inputElementFlags, u8 vertexElementFlags)
{
	assert(inputElements.empty());
	inputElements.reserve(6);

	UINT byteOffset = 0;
	assert(vertexElementFlags & VertexElementFlag_Position);
	{
		const DXGI_FORMAT format = DXGI_FORMAT_R32G32B32_FLOAT;

		if (inputElementFlags & InputElementFlag_Position)
			inputElements.push_back(DXInputElementDesc("POSITION", 0, format, 0, byteOffset));

		byteOffset += GetBytesPerElement(format);
	}

	if (vertexElementFlags & VertexElementFlag_Normal)
	{
		const DXGI_FORMAT format = DXGI_FORMAT_R32G32B32_FLOAT;

		if (inputElementFlags & InputElementFlag_Normal)
			inputElements.push_back(DXInputElementDesc("NORMAL", 0, format, 0, byteOffset));

		byteOffset += GetBytesPerElement(format);
	}

	if (vertexElementFlags & VertexElementFlag_Color)
	{
		const DXGI_FORMAT format = DXGI_FORMAT_B8G8R8A8_UNORM;

		if (inputElementFlags & InputElementFlag_Color)
			inputElements.push_back(DXInputElementDesc("COLOR", 0, format, 0, byteOffset));

		byteOffset += GetBytesPerElement(format);
	}

	if (vertexElementFlags & VertexElementFlag_Tangent)
	{
		const DXGI_FORMAT format = DXGI_FORMAT_R32G32B32_FLOAT;

		if (inputElementFlags & InputElementFlag_Tangent)
			inputElements.push_back(DXInputElementDesc("TANGENT", 0, format, 0, byteOffset));

		byteOffset += GetBytesPerElement(format);
	}

	if (vertexElementFlags & VertexElementFlag_BiTangent)
	{
		const DXGI_FORMAT format = DXGI_FORMAT_R32G32B32_FLOAT;

		if (inputElementFlags & InputElementFlag_BiTangent)
			inputElements.push_back(DXInputElementDesc("BITANGENT", 0, format, 0, byteOffset));

		byteOffset += GetBytesPerElement(format);
	}

	if (vertexElementFlags & VertexElementFlag_TexCoords)
	{
		const DXGI_FORMAT format = DXGI_FORMAT_R32G32_FLOAT;

		if (inputElementFlags & InputElementFlag_TexCoords)
			inputElements.push_back(DXInputElementDesc("TEXCOORD", 0, format, 0, byteOffset));

		byteOffset += GetBytesPerElement(format);
	}
}
