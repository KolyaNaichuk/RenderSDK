#pragma once

#include "Common/Common.h"

struct Vector2f;
struct Vector3f;

struct SubMeshData
{
	u32 mMaterialIndex;
	u32 mFaceStart;
	u32 mNumFaces;
	u32 mVertexStart;
	u32 mNumVertices;
};

class MeshData
{
public:
	MeshData();
	~MeshData();

	u32 GetNumFaces() const;
	u32 GetNumIndices() const;
	u32 GetNumVertices() const;

	void SetVertexData(u32 numVertices, const Vector3f* pPositions, const Vector3f* pNormals, const Vector2f* pTexCoords);
		
	const Vector3f* GetPositions() const;
	const Vector3f* GetNormals() const;
	const Vector2f* GetTexCoords() const;
	
	void SetIndexData(u32 numIndices, const u16* pIndices);
	const u16* Get16BitIndices() const;

	void SetIndexData(u32 numIndices, const u32* pIndices);
	const u32* Get32BitIndices() const;

	void SetSubMeshData(u32 numSubMeshes, const SubMeshData* pSubMeshes);
	u32 GetNumSubMeshes() const;
	const SubMeshData* GetSubMeshes() const;
		
	void Clear();
	
private:
	u32 m_NumIndices;
	u16* m_pIndices16Bit;
	u32* m_pIndices32Bit;

	u32 m_NumVertices;
	Vector3f* m_pPositions;
	Vector3f* m_pNormals;
	Vector2f* m_pTexCoords;

	u32 m_NumSubMeshes;
	SubMeshData* m_pSubMeshes;
};