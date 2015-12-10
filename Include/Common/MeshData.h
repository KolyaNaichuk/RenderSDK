#pragma once

#include "Common/Common.h"

struct Vector2f;
struct Vector3f;
struct Vector4f;
struct BoundingBox;

struct SubMeshData
{
	u32 mMaterialId;
	u32 mTopologyType;
	u32 mVertexStart;
	u32 mNumVertices;
	u32 mIndexStart;
	u32 mNumIndices;
};

enum FaceNormalWeight
{
	FaceNormalWeight_Equal,
	FaceNormalWeight_ByArea,
	FaceNormalWeight_ByAngle
};

class MeshData
{
public:
	MeshData();
	~MeshData();
	
	u32 GetNumFaces() const;
	u32 GetNumIndices() const;
	u32 GetNumVertices() const;

	void SetVertexData(u32 numVertices, const Vector3f* pPositions, const Vector4f* pColors = nullptr,
		const Vector3f* pNormals = nullptr, const Vector2f* pTexCoords = nullptr,
		const Vector3f* pTangents = nullptr, const Vector3f* pBiTangents = nullptr);
	
	void SetIndexData(u32 numIndices, const u16* pIndices);
	void SetIndexData(u32 numIndices, const u32* pIndices);
	
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

	Vector3f* GetBiTangents();
	const Vector3f* GetBiTangents() const;
			
	u16* Get16BitIndices();
	const u16* Get16BitIndices() const;
	
	u32* Get32BitIndices();
	const u32* Get32BitIndices() const;

	void SetSubMeshData(u32 numSubMeshes, const SubMeshData* pSubMeshes);
	u32 GetNumSubMeshes() const;
	const SubMeshData* GetSubMeshes() const;
	
	void ComputeBoundingBox();
	const BoundingBox* GetBoundingBox() const;

	void ComputeNormals(FaceNormalWeight faceNormalWeight = FaceNormalWeight_Equal);
	
	void Clear();
	
private:
	u32 m_NumIndices;
	u16* m_pIndices16Bit;
	u32* m_pIndices32Bit;

	u32 m_NumVertices;
	Vector3f* m_pPositions;
	Vector3f* m_pNormals;
	Vector2f* m_pTexCoords;
	Vector4f* m_pColors;
	Vector3f* m_pTangents;
	Vector3f* m_pBiTangents;

	u32 m_NumSubMeshes;
	SubMeshData* m_pSubMeshes;

	BoundingBox* m_pBoundingBox;
};