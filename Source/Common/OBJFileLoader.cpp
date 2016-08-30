#include "Common/OBJFileLoader.h"
#include "Math/Vector2.h"
#include "Math/Vector3.h"

namespace OBJFile
{
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
		std::vector<u32> m_PositionIndices;
		std::vector<u32> m_TexCoordIndices;
		std::vector<u32> m_NormalIndices;
	};
	
	struct Object
	{
		Object(const std::wstring& name)
			: m_Name(name)
		{}
		std::wstring m_Name;
		std::vector<u32> m_MeshIndices;
	};

	void Triangulate(const std::vector<u32>& faceVertexIndices, std::vector<u32>& triangleVertexIndices);
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

	const static std::wstring kDefaultObjectName = L"default";
	const static std::wstring kDefaultMaterialName = L"default";
	const static std::wstring kDefaultGroupName = L"default";

	u32 currentMaterialIndex = 0;
	std::vector<OBJFile::Material> materials;
	materials.emplace_back(kDefaultMaterialName);
	assert(false && "Set up default material");
		
	std::vector<OBJFile::Object> objects;
	OBJFile::Object* pCurrentObject = nullptr;

	std::vector<OBJFile::Mesh> meshes;
	OBJFile::Mesh* pCurrentMesh = nullptr;

	std::wstring currentGroupName = kDefaultGroupName;

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

			if ((pCurrentObject == nullptr) || (pCurrentObject->m_Name != objectName))
			{
				objects.emplace_back(objectName);
				pCurrentObject = &objects.back();
				pCurrentMesh = nullptr;
			}
		}
		else if (line.compare(0, 2, L"g ") == 0)
		{
			std::wstringstream stringStream(line.substr(2));
			std::wstring groupName;
			stringStream >> groupName;

			if (currentGroupName != groupName)
			{
				objects.emplace_back(groupName);
				pCurrentObject = &objects.back();
				pCurrentMesh = nullptr;
				
				currentGroupName = groupName;
			}
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
						
			const i32 numPositions = positions.size();
			const i32 numTexCoords = texCoords.size();
			const i32 numNormals = normals.size();
			
			std::vector<u32> facePositionIndices;
			std::vector<u32> faceTexCoordIndices;
			std::vector<u32> faceNormalIndices;

			while (stringStream)
			{
				i32 posIndex;
				stringStream >> posIndex;
				
				if (posIndex > 0)
					facePositionIndices.emplace_back(posIndex - 1);
				else if (posIndex < 0)
					facePositionIndices.emplace_back(posIndex + numPositions);
				else
					assert(false);
								
				if (stringStream.peek() == L'/')
				{
					stringStream.ignore(1);
					if (stringStream.peek() != L'/')
					{
						i32 texCoordIndex;
						stringStream >> texCoordIndex;

						if (texCoordIndex > 0)
							faceTexCoordIndices.emplace_back(texCoordIndex - 1);
						else if (texCoordIndex < 0)
							faceTexCoordIndices.emplace_back(texCoordIndex + numTexCoords);
						else
							assert(false);
					}
					if (stringStream.peek() == L'/')
					{						
						stringStream.ignore(1);
						
						i32 normalIndex;
						stringStream >> normalIndex;
						
						if (normalIndex > 0)
							faceNormalIndices.emplace_back(normalIndex - 1);
						else if (normalIndex < 0)
							faceNormalIndices.emplace_back(normalIndex + numNormals);
						else
							assert(false);
					}
				}
			}
						
			OBJFile::Triangulate(facePositionIndices, pCurrentMesh->m_PositionIndices);
			
			if (!faceTexCoordIndices.empty())
				OBJFile::Triangulate(faceTexCoordIndices, pCurrentMesh->m_TexCoordIndices);

			if (!faceNormalIndices.empty())
				OBJFile::Triangulate(faceNormalIndices, pCurrentMesh->m_NormalIndices);
		}
	}
	return true;
}

namespace OBJFile
{
	void Triangulate(const std::vector<u32>& faceVertexIndices, std::vector<u32>& triangleVertexIndices)
	{
		assert(faceVertexIndices.size() > 2);
		for (u32 index = 1; index < faceVertexIndices.size() - 1; ++index)
		{
			triangleVertexIndices.emplace_back(faceVertexIndices[0]);
			triangleVertexIndices.emplace_back(faceVertexIndices[index + 1]);
			triangleVertexIndices.emplace_back(faceVertexIndices[index]);
		}
		assert((triangleVertexIndices.size() % 3) == 0);
	}
}
