#pragma once

#include "Math/Vector2.h"
#include "Math/Vector3.h"

namespace OBJFile
{
	const static std::wstring kDefaultObjectName = L"default";
	const static std::wstring kDefaultMaterialName = L"default";
	const static std::wstring kDefaultGroupName = L"default";
	const static u32 kUnknownIndex = (u32)-1;

	struct VertexIndex
	{
		VertexIndex(u32 positionIndex, u32 normalIndex = kUnknownIndex, u32 texCoordIndex = kUnknownIndex)
			: m_PositionIndex(positionIndex)
			, m_NormalIndex(normalIndex)
			, m_TexCoordIndex(texCoordIndex)
		{}
		u32 m_PositionIndex;
		u32 m_NormalIndex;
		u32 m_TexCoordIndex;
	};

	struct Material
	{
		Material(const std::wstring& name)
			: m_Name(name)
		{}
		std::wstring m_Name;

		Vector3f m_AmbientColor;
		std::wstring m_AmbientMapName;

		Vector3f m_DiffuseColor;
		std::wstring m_DiffuseMapName;

		Vector3f m_SpecularColor;
		std::wstring m_SpecularMapName;

		f32 m_SpecularPower;
		std::wstring m_SpecularPowerMapName;
		
		Vector3f m_EmissiveColor;
		std::wstring m_EmissiveMapName;
		
		f32 m_Opacity;
		std::wstring m_OpacityMapName;

		f32 m_IndexOfRefraction;
	};

	struct Mesh
	{
		Mesh(u32 materialIndex)
			: m_MaterialIndex(materialIndex)
		{}
		u32 m_MaterialIndex;
		std::vector<VertexIndex> m_VertexIndices;
	};
	
	struct Object
	{
		Object(const std::wstring& name)
			: m_Name(name)
		{}
		std::wstring m_Name;
		std::vector<u32> m_MeshIndices;
	};
}

class OBJFileLoader
{
public:
	bool Load(const wchar_t* pOBJFilePath);

private:
	bool LoadOBJFile(const wchar_t* pFilePath);
	bool LoadMaterialFile(const wchar_t* pFilePath);
	void GenerateMeshBatchData();
	void Clear();
	
private:
	std::vector<Vector3f> m_Positions;
	std::vector<Vector2f> m_TexCoords;
	std::vector<Vector3f> m_Normals;

	std::wstring m_MaterialFileName;
	std::vector<OBJFile::Material> m_Materials;
	u32 m_CurrentMaterialIndex;

	std::vector<OBJFile::Object> m_Objects;
	OBJFile::Object* m_pCurrentObject;

	std::vector<OBJFile::Mesh> m_Meshes;
	OBJFile::Mesh* m_pCurrentMesh;

	std::wstring m_CurrentGroupName;
};
