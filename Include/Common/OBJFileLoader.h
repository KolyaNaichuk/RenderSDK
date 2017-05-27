#pragma once

#include "Math/Vector2.h"
#include "Math/Vector3.h"

class VertexData;
class IndexData;
class MeshBatchData;

namespace OBJFile
{
	const static std::wstring kDefaultObjectName = L"default";
	const static std::wstring kDefaultMaterialName = L"default";
	const static std::wstring kDefaultGroupName = L"default";
	const static u32 kUnknownIndex = (u32)-1;

	struct FaceElement
	{
		FaceElement(u32 positionIndex, u32 normalIndex = kUnknownIndex, u32 texCoordIndex = kUnknownIndex)
			: m_PositionIndex(positionIndex)
			, m_NormalIndex(normalIndex)
			, m_TexCoordIndex(texCoordIndex)
		{}
		u32 m_PositionIndex;
		u32 m_NormalIndex;
		u32 m_TexCoordIndex;
	};

	using MeshFace = std::vector<FaceElement>;
	using MeshTriangle = std::array<FaceElement, 3>;
		
	struct Mesh
	{
		Mesh(u32 materialIndex)
			: m_MaterialIndex(materialIndex)
		{}
		u32 m_MaterialIndex;
		std::vector<MeshTriangle> m_Triangles;
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
	std::shared_ptr<MeshBatchData> Load(const wchar_t* pOBJFilePath, bool use32BitIndices, u8 convertMeshDataFlags);

private:
	bool LoadOBJFile(const wchar_t* pFilePath, bool use32BitIndices, u8 convertMeshDataFlags);
	bool LoadMaterialFile(const wchar_t* pFilePath);
	
	u32 FindMaterialIndex(const std::wstring& materialName) const;
	std::shared_ptr<MeshBatchData> GenerateMeshBatchData(bool use32BitIndices, u8 convertMeshDataFlags);
	
	template <typename Index>
	void GenerateVertexAndIndexData(const OBJFile::Mesh& mesh, VertexData** ppVertexData, IndexData** ppIndexData);
		
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
