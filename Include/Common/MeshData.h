#pragma once

#include "Common/Common.h"

struct Vector2f;
struct Vector3f;

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

	void Clear();

private:
	u32 m_NumIndices;
	u32 m_NumVertices;

	u16* m_pIndices16Bit;
	u32* m_pIndices32Bit;

	Vector3f* m_pPositions;
	Vector3f* m_pNormals;
	Vector2f* m_pTexCoords;
};