#pragma once

#include "Common/Common.h"

struct Vector2f;
struct Vector3f;
struct Vector4f;

enum FaceNormalWeight
{
	FaceNormalWeight_Equal,
	FaceNormalWeight_ByArea,
	FaceNormalWeight_ByAngle
};

class VertexData
{
public:
	VertexData(u32 numVertices, const Vector3f* pPositions, const Vector4f* pColors = nullptr,
		const Vector3f* pNormals = nullptr, const Vector2f* pTexCoords = nullptr, const Vector3f* pTangents = nullptr);	
	~VertexData();

	enum FormatFlags
	{
		FormatFlag_Position = 1 << 0,
		FormatFlag_Normal = 1 << 1,
		FormatFlag_Color = 1 << 2,
		FormatFlag_Tangent = 1 << 3,
		FormatFlag_TexCoords = 1 << 4
	};

	u8 GetFormatFlags() const { return m_FormatFlags; }
	u32 GetNumVertices() const { return m_NumVertices; }

	Vector3f* GetPositions();
	const Vector3f* GetPositions() const;

	Vector3f* GetNormals();
	const Vector3f* GetNormals() const;

	Vector2f* GetTexCoords();
	const Vector2f* GetTexCoords() const;
	
	Vector4f* GetColors();
	const Vector4f* GetColors() const;
	
	Vector3f* GetTangents();
	const Vector3f* GetTangents() const;

private:
	u32 m_NumVertices;
	u8 m_FormatFlags;
	Vector3f* m_pPositions;
	Vector3f* m_pNormals;
	Vector2f* m_pTexCoords;
	Vector4f* m_pColors;
	Vector3f* m_pTangents;
};

class IndexData
{	
public:
	IndexData(u32 numIndices, const u16* p16BitIndices);
	IndexData(u32 numIndices, const u32* p32BitIndices);
	~IndexData();

	u32 GetNumIndices() const { return m_NumIndices; }
	DXGI_FORMAT GetFormat() const { return m_Format; }

	u16* Get16BitIndices();
	const u16* Get16BitIndices() const;

	u32* Get32BitIndices();
	const u32* Get32BitIndices() const;
	
private:
	DXGI_FORMAT m_Format;
	u32 m_NumIndices;
	u16* m_p16BitIndices;
	u32* m_p32BitIndices;
};

class MeshData
{
public:
	MeshData(VertexData* pVertexData, IndexData* pIndexData, D3D12_PRIMITIVE_TOPOLOGY_TYPE primitiveTopologyType, D3D12_PRIMITIVE_TOPOLOGY primitiveTopology, u32 materialIndex);
	~MeshData();

	VertexData* GetVertexData() { return m_pVertexData; }
	const VertexData* GetVertexData() const { return m_pVertexData; }
	
	IndexData* GetIndexData() { return m_pIndexData; };
	const IndexData* GetIndexData() const { return m_pIndexData; };

	D3D12_PRIMITIVE_TOPOLOGY_TYPE GetPrimitiveTopologyType() const { return m_PrimitiveTopologyType; }
	D3D12_PRIMITIVE_TOPOLOGY GetPrimitiveTopology() const { return m_PrimitiveTopology; }
	
	u32 GetMaterialIndex() const { return m_MaterialIndex; }
	
private:
	VertexData* m_pVertexData;
	IndexData* m_pIndexData;
	D3D12_PRIMITIVE_TOPOLOGY_TYPE m_PrimitiveTopologyType;
	D3D12_PRIMITIVE_TOPOLOGY m_PrimitiveTopology;
	u32 m_MaterialIndex;
};
