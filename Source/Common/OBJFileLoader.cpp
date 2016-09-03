#include "Common/OBJFileLoader.h"

namespace
{
	const static std::wstring kDefaultObjectName = L"default";
	const static std::wstring kDefaultMaterialName = L"default";
	const static std::wstring kDefaultGroupName = L"default";
	const static u32 kUnknownMaterialIndex = (u32)-1;

	void Triangulate(const std::vector<u32>& faceVertexIndices, std::vector<u32>& triangleVertexIndices);
}

bool OBJFileLoader::Load(const wchar_t* pOBJFilePath)
{
	bool success = LoadOBJFile(pOBJFilePath);
	if (success)
	{
		if (!m_MaterialFileName.empty())
		{
			assert(false);
			std::wstring materialFilePath;
			success = LoadMaterialFile(materialFilePath.c_str());
		}
	}
	return success;
}

bool OBJFileLoader::LoadOBJFile(const wchar_t* pFilePath)
{
	Clear();

	assert(pFilePath != nullptr);
	std::wifstream fileStream(pFilePath);
	if (!fileStream)
		return false;
	
	for (std::wstring line; std::getline(fileStream, line); )
	{
		if (line.empty())
		{
			// Skip
		}
		else if (line.compare(0, 6, L"mtllib"))
		{
			std::wstringstream stringStream(line.substr(6));
			stringStream >> m_MaterialFileName;
		}
		else if (line.compare(0, 6, L"usemtl"))
		{
			std::wstringstream stringStream(line.substr(6));
			std::wstring materialName;
			stringStream >> materialName;

			if (m_Materials[m_CurrentMaterialIndex].m_Name != materialName)
			{
				bool isNewMaterial = true;
				for (u32 index = 0; index < m_Materials.size(); ++index)
				{
					if (m_Materials[index].m_Name == materialName)
					{
						m_CurrentMaterialIndex = index;
						isNewMaterial = false;
						break;
					}
				}
				if (isNewMaterial)
				{
					m_CurrentMaterialIndex = m_Materials.size();
					m_Materials.emplace_back(materialName);
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

			if ((m_pCurrentObject == nullptr) || (m_pCurrentObject->m_Name != objectName))
			{
				m_Objects.emplace_back(objectName);
				m_pCurrentObject = &m_Objects.back();
				m_pCurrentMesh = nullptr;
			}
		}
		else if (line.compare(0, 2, L"g ") == 0)
		{
			std::wstringstream stringStream(line.substr(2));
			std::wstring groupName;
			stringStream >> groupName;

			if (m_CurrentGroupName != groupName)
			{
				m_Objects.emplace_back(groupName);
				m_pCurrentObject = &m_Objects.back();
				m_pCurrentMesh = nullptr;
				
				m_CurrentGroupName = groupName;
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
			m_Positions.emplace_back(x, y, z);
		}
		else if (line.compare(0, 2, L"vt") == 0)
		{
			std::wstringstream stringStream(line.substr(2));

			f32 u, v;
			stringStream >> u >> v;
			m_TexCoords.emplace_back(u, v);
		}
		else if (line.compare(0, 2, L"vn") == 0)
		{
			std::wstringstream stringStream(line.substr(2));

			f32 x, y, z;
			stringStream >> x >> y >> z;
			m_Normals.emplace_back(x, y, z);
		}
		else if (line.compare(0, 2, L"f ") == 0)
		{
			std::wstringstream stringStream(line.substr(2));
			
			bool createNewMesh = false;
			if (m_pCurrentMesh == nullptr)
			{
				if (m_pCurrentObject == nullptr)
				{
					m_Objects.emplace_back(kDefaultObjectName);
					m_pCurrentObject = &m_Objects.back();
				}
				createNewMesh = true;
			}
			else
				createNewMesh = m_pCurrentMesh->m_MaterialIndex != m_CurrentMaterialIndex;
			
			if (createNewMesh)
			{
				u32 currentMeshIndex = m_Meshes.size();
				m_Meshes.emplace_back(m_CurrentMaterialIndex);
				m_pCurrentMesh = &m_Meshes.back();

				m_pCurrentObject->m_MeshIndices.emplace_back(currentMeshIndex);
			}
						
			const i32 numPositions = m_Positions.size();
			const i32 numTexCoords = m_TexCoords.size();
			const i32 numNormals = m_Normals.size();
			
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
						
			Triangulate(facePositionIndices, m_pCurrentMesh->m_PositionIndices);
			
			if (!faceTexCoordIndices.empty())
				Triangulate(faceTexCoordIndices, m_pCurrentMesh->m_TexCoordIndices);

			if (!faceNormalIndices.empty())
				Triangulate(faceNormalIndices, m_pCurrentMesh->m_NormalIndices);
		}
	}
	return true;
}

bool OBJFileLoader::LoadMaterialFile(const wchar_t* pFilePath)
{
	assert(pFilePath != nullptr);
	std::wifstream fileStream(pFilePath);
	if (!fileStream)
		return false;

	for (std::wstring line; std::getline(fileStream, line); )
	{
		if (line.empty())
		{
			// Skip
		}
		else if (line.compare(0, 1, L"#") == 0)
		{
			// Comment
		}
		else if (line.compare(0, 6, L"newmtl"))
		{
			std::wstringstream stringStream(line.substr(6));
			std::wstring materialName;
			stringStream >> materialName;

			m_CurrentMaterialIndex = kUnknownMaterialIndex;
			for (u32 index = 0; index < m_Materials.size(); ++index)
			{
				if (m_Materials[index].m_Name == materialName)
				{
					m_CurrentMaterialIndex = index;
					break;
				}
			}
		}
		else if (line.compare(0, 11, L"Ka spectral"))
		{
			assert(m_CurrentMaterialIndex != kUnknownMaterialIndex);
			assert(false && "No support for the ambient reflectivity using a spectral curve");
		}
		else if (line.compare(0, 6, L"Ka xyz"))
		{
			assert(m_CurrentMaterialIndex != kUnknownMaterialIndex);
			assert(false && "No support for the ambient reflectivity using CIEXYZ values");
		}
		else if (line.compare(0, 2, L"Ka"))
		{
			std::wstringstream stringStream(line.substr(2));

			assert(m_CurrentMaterialIndex != kUnknownMaterialIndex);
			Vector3f& ambientColor = m_Materials[m_CurrentMaterialIndex].m_AmbientColor;
			
			stringStream >> ambientColor.m_X;
			if (stringStream)
			{
				stringStream >> ambientColor.m_Y;
				stringStream >> ambientColor.m_Z;
			}
			else
			{
				ambientColor.m_Y = ambientColor.m_X;
				ambientColor.m_Z = ambientColor.m_X;
			}
		}
		else if (line.compare(0, 6, L"map_Ka"))
		{
			std::wstringstream stringStream(line.substr(6));
			
			assert(m_CurrentMaterialIndex != kUnknownMaterialIndex);
			stringStream >> m_Materials[m_CurrentMaterialIndex].m_AmbientMapName;
		}
		else if (line.compare(0, 11, L"Kd spectral"))
		{
			assert(m_CurrentMaterialIndex != kUnknownMaterialIndex);
			assert(false && "No support for the diffuse reflectivity using a spectral curve");
		}
		else if (line.compare(0, 6, L"Kd xyz"))
		{
			assert(m_CurrentMaterialIndex != kUnknownMaterialIndex);
			assert(false && "No support for the diffuse reflectivity using CIEXYZ values");
		}
		else if (line.compare(0, 2, L"Kd"))
		{
			std::wstringstream stringStream(line.substr(2));

			assert(m_CurrentMaterialIndex != kUnknownMaterialIndex);
			Vector3f& diffuseColor = m_Materials[m_CurrentMaterialIndex].m_DiffuseColor;

			stringStream >> diffuseColor.m_X;
			if (stringStream)
			{
				stringStream >> diffuseColor.m_Y;
				stringStream >> diffuseColor.m_Z;
			}
			else
			{
				diffuseColor.m_Y = diffuseColor.m_X;
				diffuseColor.m_Z = diffuseColor.m_X;
			}
		}
		else if (line.compare(0, 6, L"map_Kd"))
		{
			std::wstringstream stringStream(line.substr(6));

			assert(m_CurrentMaterialIndex != kUnknownMaterialIndex);
			stringStream >> m_Materials[m_CurrentMaterialIndex].m_DiffuseMapName;
		}
		else if (line.compare(0, 11, L"Ks spectral"))
		{
			assert(m_CurrentMaterialIndex != kUnknownMaterialIndex);
			assert(false && "No support for the specular reflectivity using a spectral curve");
		}
		else if (line.compare(0, 6, L"Ks xyz"))
		{
			assert(m_CurrentMaterialIndex != kUnknownMaterialIndex);
			assert(false && "No support for the specular reflectivity using CIEXYZ values");
		}
		else if (line.compare(0, 2, L"Ks"))
		{
			std::wstringstream stringStream(line.substr(2));

			assert(m_CurrentMaterialIndex != kUnknownMaterialIndex);
			Vector3f& specularColor = m_Materials[m_CurrentMaterialIndex].m_SpecularColor;

			stringStream >> specularColor.m_X;
			if (stringStream)
			{
				stringStream >> specularColor.m_Y;
				stringStream >> specularColor.m_Z;
			}
			else
			{
				specularColor.m_Y = specularColor.m_X;
				specularColor.m_Z = specularColor.m_X;
			}
		}
		else if (line.compare(0, 6, L"map_Ks"))
		{
			std::wstringstream stringStream(line.substr(6));

			assert(m_CurrentMaterialIndex != kUnknownMaterialIndex);
			stringStream >> m_Materials[m_CurrentMaterialIndex].m_SpecularMapName;
		}
		else if (line.compare(0, 2, L"Ns"))
		{
			std::wstringstream stringStream(line.substr(2));

			assert(m_CurrentMaterialIndex != kUnknownMaterialIndex);
			stringStream >> m_Materials[m_CurrentMaterialIndex].m_SpecularPower;
		}
		else if (line.compare(0, 6, L"map_Ns"))
		{
			std::wstringstream stringStream(line.substr(6));

			assert(m_CurrentMaterialIndex != kUnknownMaterialIndex);
			stringStream >> m_Materials[m_CurrentMaterialIndex].m_SpecularPowerMapName;
		}
		else if (line.compare(0, 11, L"Ke spectral"))
		{
			assert(m_CurrentMaterialIndex != kUnknownMaterialIndex);
			assert(false && "No support for the emissive reflectivity using a spectral curve");
		}
		else if (line.compare(0, 6, L"Ke xyz"))
		{
			assert(m_CurrentMaterialIndex != kUnknownMaterialIndex);
			assert(false && "No support for the emissive reflectivity using CIEXYZ values");
		}
		else if (line.compare(0, 2, L"Ke"))
		{
			std::wstringstream stringStream(line.substr(2));

			assert(m_CurrentMaterialIndex != kUnknownMaterialIndex);
			Vector3f& emissiveColor = m_Materials[m_CurrentMaterialIndex].m_EmissiveColor;

			stringStream >> emissiveColor.m_X;
			if (stringStream)
			{
				stringStream >> emissiveColor.m_Y;
				stringStream >> emissiveColor.m_Z;
			}
			else
			{
				emissiveColor.m_Y = emissiveColor.m_X;
				emissiveColor.m_Z = emissiveColor.m_X;
			}
		}
		else if (line.compare(0, 6, L"map_Ke"))
		{
			std::wstringstream stringStream(line.substr(6));

			assert(m_CurrentMaterialIndex != kUnknownMaterialIndex);
			stringStream >> m_Materials[m_CurrentMaterialIndex].m_EmissiveMapName;
		}
		else if (line.compare(0, 2, L"Ni"))
		{
			std::wstringstream stringStream(line.substr(2));

			assert(m_CurrentMaterialIndex != kUnknownMaterialIndex);
			stringStream >> m_Materials[m_CurrentMaterialIndex].m_IndexOfRefraction;
		}
		else if (line.compare(0, 2, L"d "))
		{
			std::wstringstream stringStream(line.substr(2));
			
			assert(m_CurrentMaterialIndex != kUnknownMaterialIndex);
			stringStream >> m_Materials[m_CurrentMaterialIndex].m_Opacity;
		}
		else if (line.compare(0, 5, L"map_d"))
		{
			std::wstringstream stringStream(line.substr(5));

			assert(m_CurrentMaterialIndex != kUnknownMaterialIndex);
			stringStream >> m_Materials[m_CurrentMaterialIndex].m_OpacityMapName;
		}
	}	
	return true;
}

void OBJFileLoader::Clear()
{
	m_Positions.clear();
	m_TexCoords.clear();
	m_Normals.clear();

	m_MaterialFileName.clear();
	m_Materials.clear();
	m_CurrentMaterialIndex = 0;
	m_Materials.emplace_back(kDefaultMaterialName);
	assert(false && "Set up default material");
	
	m_Objects.clear();
	m_pCurrentObject = nullptr;
	
	m_Meshes.clear();
	m_pCurrentMesh = nullptr;

	m_CurrentGroupName = kDefaultGroupName;
}

namespace
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
