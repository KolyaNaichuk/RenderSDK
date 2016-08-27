#include "Common/OBJFileLoader.h"
#include "Math/Vector2.h"
#include "Math/Vector3.h"

namespace OBJFile
{
	struct Face
	{
		enum Attributes
		{
			Attributes_PositionIndices = 1 << 0,
			Attributes_TexCoordIndices = 1 << 1,
			Attributes_NormalIndices = 1 << 2
		};
		enum { kNumIndices = 3 };

		Face()
			: m_Attributes(0)
		{}		
		u32 m_PositionIndices[kNumIndices];
		u32 m_TexCoordIndices[kNumIndices];
		u32 m_NormalIndices[kNumIndices];
		u8 m_Attributes;
	};

	struct Material
	{
		Material(const std::wstring& name)
			: m_Name(name)
		{}
		std::wstring m_Name;
	};

	struct Mesh
	{
		Mesh(u32 materialIndex)
			: m_MaterialIndex(materialIndex)
		{}
		u32 m_MaterialIndex;
		std::vector<Face> m_Faces;
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

bool OBJFileLoader::Load(const wchar_t* pFilePath)
{
	assert(pFilePath != nullptr);
	std::wifstream fileStream(pFilePath);
	if (!fileStream)
		return false;

	std::vector<Vector3f> positions;
	std::vector<Vector2f> texCoords;
	std::vector<Vector3f> normals;
	
	std::wstring materialLibName;

	const static std::wstring kDefaultObjectName = L"Default Object";
	const static std::wstring kDefaultMaterialName = L"Default Material";

	u32 currentMaterialIndex = 0;
	std::vector<OBJFile::Material> materials;
	materials.emplace_back(kDefaultMaterialName);
	assert(false && "Set up default material");
		
	std::vector<OBJFile::Object> objects;
	OBJFile::Object* pCurrentObject = nullptr;

	std::vector<OBJFile::Mesh> meshes;
	OBJFile::Mesh* pCurrentMesh = nullptr;

	for (std::wstring line; std::getline(fileStream, line); )
	{
		if (line.empty())
		{
			// Skip
		}
		else if (line.compare(0, 6, L"mtllib"))
		{
			std::wstringstream stringStream(line.substr(6));
			stringStream >> materialLibName;
		}
		else if (line.compare(0, 6, L"usemtl"))
		{
			std::wstringstream stringStream(line.substr(6));
			std::wstring materialName;
			stringStream >> materialName;

			if (materials[currentMaterialIndex].m_Name != materialName)
			{
				bool isNewMaterial = true;
				for (u32 index = 0; index < materials.size(); ++index)
				{
					if (materials[index].m_Name == materialName)
					{
						currentMaterialIndex = index;
						isNewMaterial = false;
						break;
					}
				}
				if (isNewMaterial)
				{
					currentMaterialIndex = materials.size();
					materials.emplace_back(materialName);
				}
			}
		}
		else if (line.compare(0, 1, L"#") == 0)
		{
			// Comment
		}
		else if (line.compare(0, 2, L"o ") == 0)
		{
			std::wstringstream stringStream(line.substr(2));
			std::wstring objectName;
			stringStream >> objectName;

			pCurrentObject = nullptr;
			for (OBJFile::Object& object : objects)
			{
				if (object.m_Name == objectName)
				{
					pCurrentObject = &object;
					break;
				}
			}
			if (pCurrentObject == nullptr)
			{
				objects.emplace_back(objectName);
				pCurrentObject = &objects.back();
			}
			pCurrentMesh = nullptr;
		}
		else if (line.compare(0, 2, L"g ") == 0)
		{
			// Group name
			assert(false);
		}
		else if (line.compare(0, 2, L"s ") == 0)
		{
			// Smoothing group
			assert(false);
		}
		else if (line.compare(0, 2, L"v ") == 0)
		{
			std::wstringstream stringStream(line.substr(2));
			
			f32 x, y, z;
			stringStream >> x >> y >> z;
			positions.emplace_back(x, y, z);
		}
		else if (line.compare(0, 2, L"vt") == 0)
		{
			std::wstringstream stringStream(line.substr(2));

			f32 u, v;
			stringStream >> u >> v;
			texCoords.emplace_back(u, v);
		}
		else if (line.compare(0, 2, L"vn") == 0)
		{
			std::wstringstream stringStream(line.substr(2));

			f32 x, y, z;
			stringStream >> x >> y >> z;
			normals.emplace_back(x, y, z);
		}
		else if (line.compare(0, 2, L"f ") == 0)
		{
			std::wstringstream stringStream(line.substr(2));
			
			bool createNewMesh = false;
			if (pCurrentMesh == nullptr)
			{
				if (pCurrentObject == nullptr)
				{
					objects.emplace_back(kDefaultObjectName);
					pCurrentObject = &objects.back();
				}
				createNewMesh = true;
			}
			else
				createNewMesh = pCurrentMesh->m_MaterialIndex != currentMaterialIndex;
			
			if (createNewMesh)
			{
				u32 currentMeshIndex = meshes.size();
				meshes.emplace_back(currentMaterialIndex);
				pCurrentMesh = &meshes.back();

				pCurrentObject->m_MeshIndices.emplace_back(currentMeshIndex);
			}
			
			pCurrentMesh->m_Faces.emplace_back();
			OBJFile::Face* pCurrentFace = &pCurrentMesh->m_Faces.back();

			const i32 numPositions = positions.size();
			const i32 numTexCoords = texCoords.size();
			const i32 numNormals = normals.size();
						
			u8 index = 0;						
			for (; stringStream; ++index)
			{
				i32 posIndex;
				stringStream >> posIndex;
				
				if (posIndex > 0)
					pCurrentFace->m_PositionIndices[index] = posIndex - 1;
				else if (posIndex < 0)
					pCurrentFace->m_PositionIndices[index] = posIndex + numPositions;
				else
					assert(false);

				pCurrentFace->m_Attributes |= OBJFile::Face::Attributes_PositionIndices;
				
				if (stringStream.peek() == L'/')
				{
					stringStream.ignore(1);
					if (stringStream.peek() != L'/')
					{
						i32 texCoordIndex;
						stringStream >> texCoordIndex;

						if (texCoordIndex > 0)
							pCurrentFace->m_TexCoordIndices[index] = texCoordIndex - 1;
						else if (texCoordIndex < 0)
							pCurrentFace->m_TexCoordIndices[index] = texCoordIndex + numTexCoords;
						else
							assert(false);

						pCurrentFace->m_Attributes |= OBJFile::Face::Attributes_TexCoordIndices;
					}
					if (stringStream.peek() == L'/')
					{						
						stringStream.ignore(1);
						
						i32 normalIndex;
						stringStream >> normalIndex;
						
						if (normalIndex > 0)
							pCurrentFace->m_NormalIndices[index] = normalIndex - 1;
						else if (normalIndex < 0)
							pCurrentFace->m_NormalIndices[index] = normalIndex + numNormals;
						else
							assert(false);

						pCurrentFace->m_Attributes |= OBJFile::Face::Attributes_NormalIndices;
					}
				}
			}
			assert((pCurrentFace->m_Attributes & OBJFile::Face::Attributes_PositionIndices) != 0);
			assert((index + 1) == OBJFile::Face::kNumIndices);
		}
	}
	return true;
}
