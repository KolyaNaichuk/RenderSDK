#include "Common/OBJFileLoader.h"
#include "Common/MeshBatchData.h"
#include "Common/MeshData.h"
#include "Common/MeshDataUtilities.h"
#include "Math/Hash.h"

namespace OBJFile
{
	std::vector<MeshTriangle> Triangulate(const MeshPolygon& meshPolygon)
	{
		assert(meshPolygon.size() > 2);

		std::vector<MeshTriangle> meshTriangles;
		for (u32 index = 1; index < meshPolygon.size() - 1; ++index)
			meshTriangles.emplace_back(MeshTriangle{meshPolygon[0], meshPolygon[index + 1], meshPolygon[index]});
		
		return meshTriangles;
	}
}

bool OBJFileLoader::Load(const wchar_t* pOBJFilePath, bool use32BitIndices, u8 convertMeshDataFlags)
{
	bool success = LoadOBJFile(pOBJFilePath, use32BitIndices, convertMeshDataFlags);
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

bool OBJFileLoader::LoadOBJFile(const wchar_t* pFilePath, bool use32BitIndices, u8 convertMeshDataFlags)
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
			
			Vector3f position;
			stringStream >> position.m_X >> position.m_Y >> position.m_Z;
			
			m_Positions.emplace_back(position);
		}
		else if (line.compare(0, 2, L"vt") == 0)
		{
			std::wstringstream stringStream(line.substr(2));

			Vector2f texCoord;
			stringStream >> texCoord.m_X >> texCoord.m_Y;

			m_TexCoords.emplace_back(texCoord);
		}
		else if (line.compare(0, 2, L"vn") == 0)
		{
			std::wstringstream stringStream(line.substr(2));

			Vector3f normal;
			stringStream >> normal.m_X >> normal.m_Y >> normal.m_Z;

			assert(IsNormalized(normal));
			m_Normals.emplace_back(normal);
		}
		else if (line.compare(0, 2, L"f ") == 0)
		{
			std::wstringstream stringStream(line.substr(2));
			
			bool createNewMesh = false;
			if (m_pCurrentMesh == nullptr)
			{
				if (m_pCurrentObject == nullptr)
				{
					m_Objects.emplace_back(OBJFile::kDefaultObjectName);
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
			
			OBJFile::MeshPolygon meshPolygon;
			while (stringStream)
			{
				i32 positionIndex;
				stringStream >> positionIndex;
				
				if (positionIndex > 0)
					meshPolygon.emplace_back(positionIndex - 1);
				else if (positionIndex < 0)
					meshPolygon.emplace_back(positionIndex + numPositions);
				else
					assert(false);
				OBJFile::FaceElement& currentFaceElement = meshPolygon.back();
				
				if (stringStream.peek() == L'/')
				{
					stringStream.ignore(1);
					if (stringStream.peek() != L'/')
					{
						i32 texCoordIndex;
						stringStream >> texCoordIndex;

						if (texCoordIndex > 0)
							currentFaceElement.m_TexCoordIndex = texCoordIndex - 1;
						else if (texCoordIndex < 0)
							currentFaceElement.m_TexCoordIndex = texCoordIndex + numTexCoords;
						else
							assert(false);
					}
					if (stringStream.peek() == L'/')
					{
						stringStream.ignore(1);
						
						i32 normalIndex;
						stringStream >> normalIndex;
						
						if (normalIndex > 0)
							currentFaceElement.m_NormalIndex = normalIndex - 1;
						else if (normalIndex < 0)
							currentFaceElement.m_NormalIndex = normalIndex + numNormals;
						else
							assert(false);
					}
				}
			}
			m_pCurrentMesh->m_Triangles = Triangulate(meshPolygon);
		}
	}
	GenerateMeshBatchData(use32BitIndices, convertMeshDataFlags);
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

			m_CurrentMaterialIndex = OBJFile::kUnknownIndex;
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
			assert(m_CurrentMaterialIndex != OBJFile::kUnknownIndex);
			assert(false && "No support for the ambient reflectivity using a spectral curve");
		}
		else if (line.compare(0, 6, L"Ka xyz"))
		{
			assert(m_CurrentMaterialIndex != OBJFile::kUnknownIndex);
			assert(false && "No support for the ambient reflectivity using CIEXYZ values");
		}
		else if (line.compare(0, 2, L"Ka"))
		{
			std::wstringstream stringStream(line.substr(2));

			assert(m_CurrentMaterialIndex != OBJFile::kUnknownIndex);
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
			
			assert(m_CurrentMaterialIndex != OBJFile::kUnknownIndex);
			stringStream >> m_Materials[m_CurrentMaterialIndex].m_AmbientMapName;
		}
		else if (line.compare(0, 11, L"Kd spectral"))
		{
			assert(m_CurrentMaterialIndex != OBJFile::kUnknownIndex);
			assert(false && "No support for the diffuse reflectivity using a spectral curve");
		}
		else if (line.compare(0, 6, L"Kd xyz"))
		{
			assert(m_CurrentMaterialIndex != OBJFile::kUnknownIndex);
			assert(false && "No support for the diffuse reflectivity using CIEXYZ values");
		}
		else if (line.compare(0, 2, L"Kd"))
		{
			std::wstringstream stringStream(line.substr(2));

			assert(m_CurrentMaterialIndex != OBJFile::kUnknownIndex);
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

			assert(m_CurrentMaterialIndex != OBJFile::kUnknownIndex);
			stringStream >> m_Materials[m_CurrentMaterialIndex].m_DiffuseMapName;
		}
		else if (line.compare(0, 11, L"Ks spectral"))
		{
			assert(m_CurrentMaterialIndex != OBJFile::kUnknownIndex);
			assert(false && "No support for the specular reflectivity using a spectral curve");
		}
		else if (line.compare(0, 6, L"Ks xyz"))
		{
			assert(m_CurrentMaterialIndex != OBJFile::kUnknownIndex);
			assert(false && "No support for the specular reflectivity using CIEXYZ values");
		}
		else if (line.compare(0, 2, L"Ks"))
		{
			std::wstringstream stringStream(line.substr(2));

			assert(m_CurrentMaterialIndex != OBJFile::kUnknownIndex);
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

			assert(m_CurrentMaterialIndex != OBJFile::kUnknownIndex);
			stringStream >> m_Materials[m_CurrentMaterialIndex].m_SpecularMapName;
		}
		else if (line.compare(0, 2, L"Ns"))
		{
			std::wstringstream stringStream(line.substr(2));

			assert(m_CurrentMaterialIndex != OBJFile::kUnknownIndex);
			stringStream >> m_Materials[m_CurrentMaterialIndex].m_SpecularPower;
		}
		else if (line.compare(0, 6, L"map_Ns"))
		{
			std::wstringstream stringStream(line.substr(6));

			assert(m_CurrentMaterialIndex != OBJFile::kUnknownIndex);
			stringStream >> m_Materials[m_CurrentMaterialIndex].m_SpecularPowerMapName;
		}
		else if (line.compare(0, 11, L"Ke spectral"))
		{
			assert(m_CurrentMaterialIndex != OBJFile::kUnknownIndex);
			assert(false && "No support for the emissive reflectivity using a spectral curve");
		}
		else if (line.compare(0, 6, L"Ke xyz"))
		{
			assert(m_CurrentMaterialIndex != OBJFile::kUnknownIndex);
			assert(false && "No support for the emissive reflectivity using CIEXYZ values");
		}
		else if (line.compare(0, 2, L"Ke"))
		{
			std::wstringstream stringStream(line.substr(2));

			assert(m_CurrentMaterialIndex != OBJFile::kUnknownIndex);
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

			assert(m_CurrentMaterialIndex != OBJFile::kUnknownIndex);
			stringStream >> m_Materials[m_CurrentMaterialIndex].m_EmissiveMapName;
		}
		else if (line.compare(0, 2, L"Ni"))
		{
			std::wstringstream stringStream(line.substr(2));

			assert(m_CurrentMaterialIndex != OBJFile::kUnknownIndex);
			stringStream >> m_Materials[m_CurrentMaterialIndex].m_IndexOfRefraction;
		}
		else if (line.compare(0, 2, L"d "))
		{
			std::wstringstream stringStream(line.substr(2));
			
			assert(m_CurrentMaterialIndex != OBJFile::kUnknownIndex);
			stringStream >> m_Materials[m_CurrentMaterialIndex].m_Opacity;
		}
		else if (line.compare(0, 5, L"map_d"))
		{
			std::wstringstream stringStream(line.substr(5));

			assert(m_CurrentMaterialIndex != OBJFile::kUnknownIndex);
			stringStream >> m_Materials[m_CurrentMaterialIndex].m_OpacityMapName;
		}
	}	
	return true;
}

std::shared_ptr<MeshBatchData> OBJFileLoader::GenerateMeshBatchData(bool use32BitIndices, u8 convertMeshDataFlags)
{
	u8 vertexFormat = 0;
	
	assert(!m_Positions.empty());
	vertexFormat |= VertexData::FormatFlag_Position;
	
	assert(!m_Normals.empty());
	vertexFormat |= VertexData::FormatFlag_Normal;
	
	assert(!m_TexCoords.empty());
	vertexFormat |= VertexData::FormatFlag_TexCoords;

	const DXGI_FORMAT indexFormat = use32BitIndices ? DXGI_FORMAT_R32_UINT : DXGI_FORMAT_R16_UINT;
	const D3D12_PRIMITIVE_TOPOLOGY_TYPE primitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	const D3D12_PRIMITIVE_TOPOLOGY primitiveTopology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

	auto meshBatchData = std::make_shared<MeshBatchData>(vertexFormat, indexFormat, primitiveTopologyType, primitiveTopology);
	for (const auto& mesh : m_Meshes)
	{
		VertexData* pVertexData = nullptr;
		IndexData* pIndexData = nullptr;

		if (use32BitIndices)
			GenerateVertexAndIndexData<u32>(mesh, &pVertexData, &pIndexData);
		else
			GenerateVertexAndIndexData<u16>(mesh, &pVertexData, &pIndexData);
		
		Material* pMaterial = nullptr;
		assert(pMaterial != nullptr);

		MeshData meshData(pVertexData, pIndexData, pMaterial, primitiveTopologyType, primitiveTopology);
		if (convertMeshDataFlags != 0)
			ConvertMeshData(&meshData, convertMeshDataFlags);
		meshData.RecalcAABB();

		meshBatchData->Append(&meshData);
	}

	return meshBatchData;
}

template <typename Index>
void OBJFileLoader::GenerateVertexAndIndexData(const OBJFile::Mesh& mesh, VertexData** ppVertexData, IndexData** ppIndexData)
{
	assert((sizeof(u16) == sizeof(Index)) || (sizeof(u32) == sizeof(Index)));

	struct FaceElementHasher
	{
		std::size_t operator()(const OBJFile::FaceElement& faceElement) const
		{
			std::size_t seed = 0;
			HashCombine(seed, faceElement.m_PositionIndex);
			HashCombine(seed, faceElement.m_NormalIndex);
			HashCombine(seed, faceElement.m_TexCoordIndex);

			return seed;
		};
	};

	struct FaceElementComp
	{
		bool operator()(const OBJFile::FaceElement& faceElement1, const OBJFile::FaceElement& faceElement2) const
		{
			return (faceElement1.m_PositionIndex == faceElement2.m_PositionIndex) &&
				(faceElement1.m_NormalIndex == faceElement2.m_NormalIndex) &&
				(faceElement1.m_TexCoordIndex == faceElement2.m_TexCoordIndex);
		};
	};
		
	std::vector<Index> indices;
	std::vector<Vector3f> positions;
	std::vector<Vector2f> texCoords;
	std::vector<Vector3f> normals;
	
	using FaceElementCache = std::unordered_map<OBJFile::FaceElement, u32, FaceElementHasher, FaceElementComp>;
	FaceElementCache faceElementCache;

	for (const OBJFile::MeshTriangle& triangle : mesh.m_Triangles)
	{
		for (const OBJFile::FaceElement& faceElement : triangle)
		{
			u32 newIndex = faceElementCache.size();
			auto result = faceElementCache.insert({faceElement, newIndex});
			if (result.second)
			{
				positions.emplace_back(m_Positions[faceElement.m_PositionIndex]);
				texCoords.emplace_back(m_TexCoords[faceElement.m_TexCoordIndex]);
				normals.emplace_back(m_Normals[faceElement.m_NormalIndex]);
			}
			else
			{
				newIndex = result.first->second;
			}
			indices.emplace_back((Index)newIndex);
		}
	}

	*ppVertexData = new VertexData(positions.size(), positions.data(), normals.data(), nullptr, texCoords.data());
	*ppIndexData = new IndexData(indices.size(), indices.data());
}

void OBJFileLoader::Clear()
{
	m_Positions.clear();
	m_TexCoords.clear();
	m_Normals.clear();

	m_MaterialFileName.clear();
	m_Materials.clear();
	m_CurrentMaterialIndex = 0;
	m_Materials.emplace_back(OBJFile::kDefaultMaterialName);
	assert(false && "Set up default material");
	
	m_Objects.clear();
	m_pCurrentObject = nullptr;
	
	m_Meshes.clear();
	m_pCurrentMesh = nullptr;

	m_CurrentGroupName = OBJFile::kDefaultGroupName;
}
