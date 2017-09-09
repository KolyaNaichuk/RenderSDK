#pragma once

#include "D3DWrapper/Common.h"

struct AxisAlignedBox;
struct OrientedBox;
struct Material;
struct Vector2f;
struct Vector3f;
struct Vector4f;
struct Matrix4f;

class VertexData
{
public:
	VertexData(u32 numVertices, const Vector3f* pPositions, const Vector3f* pNormals = nullptr,
		const Vector2f* pTexCoords = nullptr, const Vector4f* pColors = nullptr, const Vector3f* pTangents = nullptr);
	~VertexData();

	enum FormatFlags
	{
		FormatFlag_Position = 1 << 0,
		FormatFlag_Normal = 1 << 1,
		FormatFlag_Color = 1 << 2,
		FormatFlag_Tangent = 1 << 3,
		FormatFlag_TexCoords = 1 << 4
	};

	VertexData(const VertexData&) = delete;
	VertexData& operator= (const VertexData&) = delete;

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

	IndexData(const IndexData&) = delete;
	IndexData& operator= (const IndexData&) = delete;

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

class Mesh
{
public:
	Mesh(VertexData* pVertexData, IndexData* pIndexData, u32 numInstances, Matrix4f* pInstanceWorldMatrices,
		u32 materialIndex, D3D12_PRIMITIVE_TOPOLOGY_TYPE primitiveTopologyType, D3D12_PRIMITIVE_TOPOLOGY primitiveTopology);
	
	~Mesh();

	VertexData* GetVertexData() { return m_pVertexData; }
	const VertexData* GetVertexData() const { return m_pVertexData; }
	
	IndexData* GetIndexData() { return m_pIndexData; };
	const IndexData* GetIndexData() const { return m_pIndexData; };

	u32 GetNumInstances() const { return m_NumInstances; }

	Matrix4f* GetInstanceWorldMatrices() { return m_pInstanceWorldMatrices; }
	const Matrix4f* GetInstanceWorldMatrices() const { return m_pInstanceWorldMatrices; }

	AxisAlignedBox* GetInstanceWorldAABBs() { return m_pInstanceWorldAABBs; }
	const AxisAlignedBox* GetInstanceWorldAABBs() const { return m_pInstanceWorldAABBs; }
	
	OrientedBox* GetInstanceWorldOBBs() { return m_pInstanceWorldOBBs; }
	const OrientedBox* GetInstanceWorldOBBs() const { return m_pInstanceWorldOBBs; }

	void RecalcInstanceWorldBounds();
	
	D3D12_PRIMITIVE_TOPOLOGY_TYPE GetPrimitiveTopologyType() const { return m_PrimitiveTopologyType; }
	D3D12_PRIMITIVE_TOPOLOGY GetPrimitiveTopology() const { return m_PrimitiveTopology; }
	
	u32 GetMaterialIndex() const { return m_pMaterialIndex; }
	
private:
	VertexData* m_pVertexData;
	IndexData* m_pIndexData;

	u32 m_NumInstances;
	Matrix4f* m_pInstanceWorldMatrices;
	AxisAlignedBox* m_pInstanceWorldAABBs;
	OrientedBox* m_pInstanceWorldOBBs;

	u32 m_pMaterialIndex;
	D3D12_PRIMITIVE_TOPOLOGY_TYPE m_PrimitiveTopologyType;
	D3D12_PRIMITIVE_TOPOLOGY m_PrimitiveTopology;
};
