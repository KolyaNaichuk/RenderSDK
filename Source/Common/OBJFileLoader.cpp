#include "Common/OBJFileLoader.h"
#include "Common/MeshBatchData.h"
#include "Common/MeshData.h"
#include "Common/MeshDataUtilities.h"
#include "Math/Hash.h"
#include <cwctype>

namespace OBJFile
{
	std::vector<MeshTriangle> Triangulate(const MeshPolygon& meshPolygon)
	{
		assert(meshPolygon.size() > 2);

		std::vector<MeshTriangle> meshTriangles;
		for (u32 index = 2; index < meshPolygon.size(); ++index)
			meshTriangles.emplace_back(MeshTriangle{meshPolygon[0], meshPolygon[index - 1], meshPolygon[index]});
		
		return meshTriangles;
	}
}

std::shared_ptr<MeshBatchData> OBJFileLoader::Load(const wchar_t* pOBJFilePath, bool use32BitIndices, u8 convertMeshDataFlags)
{
	Clear();

	bool loadedOBJFile = LoadOBJFile(pOBJFilePath, use32BitIndices, convertMeshDataFlags);
	if (!loadedOBJFile)
		return nullptr;

	if (!m_MaterialFileName.empty())
	{
		assert(false && "Missing complete implementation");
		std::wstring materialFilePath;

		bool loadedMaterialFile = LoadMaterialFile(materialFilePath.c_str());
		if (!loadedMaterialFile)
			return nullptr;
	}

	return GenerateMeshBatchData(use32BitIndices, convertMeshDataFlags);
}

bool OBJFileLoader::LoadOBJFile(const wchar_t* pFilePath, bool use32BitIndices, u8 convertMeshDataFlags)
{
	assert(pFilePath != nullptr);
	std::wifstream fileStream(pFilePath);
	if (!fileStream)
		return false;
	
	for (std::wstring statement; true; )
	{
		fileStream >> statement;
		if (!fileStream)
			break;

		if (statement == L"#")
		{
			// Ignore comment
		}
		else if (statement == L"mtllib")
		{
			fileStream >> m_MaterialFileName;
		}		
		else if (statement == L"usemtl")
		{
			std::wstring materialName;
			fileStream >> materialName;

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
		else if (statement == L"o")
		{
			std::wstring objectName;
			fileStream >> objectName;

			if ((m_pCurrentObject == nullptr) || (m_pCurrentObject->m_Name != objectName))
			{
				m_Objects.emplace_back(objectName);
				m_pCurrentObject = &m_Objects.back();
				m_pCurrentMesh = nullptr;
			}
		}
		else if (statement == L"g")
		{
			std::wstring groupName;
			fileStream >> groupName;

			if (m_CurrentGroupName != groupName)
			{
				m_Objects.emplace_back(groupName);
				m_pCurrentObject = &m_Objects.back();
				m_pCurrentMesh = nullptr;
				
				m_CurrentGroupName = groupName;
			}
		}
		else if (statement == L"s")
		{
			// Ignore smoothing group
		}
		else if (statement == L"v")
		{
			Vector3f position;
			fileStream >> position.m_X >> position.m_Y >> position.m_Z;
			
			m_Positions.emplace_back(position);
		}
		else if (statement == L"vt")
		{
			Vector2f texCoord;
			fileStream >> texCoord.m_X >> texCoord.m_Y;

			m_TexCoords.emplace_back(texCoord);
		}
		else if (statement == L"vn")
		{
			Vector3f normal;
			fileStream >> normal.m_X >> normal.m_Y >> normal.m_Z;

			m_Normals.emplace_back(Normalize(normal));
		}
		else if (statement == L"f")
		{
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
			while (true)
			{
				i32 positionIndex;
				fileStream >> positionIndex;
				
				if (positionIndex > 0)
					meshPolygon.emplace_back(positionIndex - 1);
				else if (positionIndex < 0)
					meshPolygon.emplace_back(positionIndex + numPositions);
				else
					assert(false);
				OBJFile::FaceElement& currentFaceElement = meshPolygon.back();
				
				if (fileStream.peek() == L'/')
				{
					fileStream.ignore(1);
					if (fileStream.peek() != L'/')
					{
						i32 texCoordIndex;
						fileStream >> texCoordIndex;

						if (texCoordIndex > 0)
							currentFaceElement.m_TexCoordIndex = texCoordIndex - 1;
						else if (texCoordIndex < 0)
							currentFaceElement.m_TexCoordIndex = texCoordIndex + numTexCoords;
						else
							assert(false);
					}
					if (fileStream.peek() == L'/')
					{
						fileStream.ignore(1);
						
						i32 normalIndex;
						fileStream >> normalIndex;
						
						if (normalIndex > 0)
							currentFaceElement.m_NormalIndex = normalIndex - 1;
						else if (normalIndex < 0)
							currentFaceElement.m_NormalIndex = normalIndex + numNormals;
						else
							assert(false);
					}
				}

				bool reachedFaceEnd = false;
				while (true)
				{
					const wchar_t character = fileStream.peek();
					if (!fileStream || (character == L'\n'))
					{
						reachedFaceEnd = true;
						break;
					}
					else if (std::iswdigit(character) != 0)
					{
						break;
					}
					fileStream.ignore(1);
				}
				if (reachedFaceEnd)
				{
					break;
				}
			}

			auto meshTriangles = Triangulate(meshPolygon);
			m_pCurrentMesh->m_Triangles.insert(m_pCurrentMesh->m_Triangles.end(), meshTriangles.cbegin(), meshTriangles.cend());
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

	for (std::wstring statement; true; )
	{
		fileStream >> statement;
		if (!fileStream)
			break;

		if (statement == L"#")
		{
			// Ignore comment
		}
		else if (statement == L"newmtl")
		{
			std::wstring materialName;
			fileStream >> materialName;

			m_CurrentMaterialIndex = OBJFile::kUnknownIndex;
			for (u32 index = 0; index < m_Materials.size(); ++index)
			{
				if (m_Materials[index].m_Name == materialName)
				{
					m_CurrentMaterialIndex = index;
					break;
				}
			}
			assert(m_CurrentMaterialIndex != OBJFile::kUnknownIndex);
		}
		else if (statement == L"Ka spectral")
		{
			assert(false && "No support for the ambient reflectivity using a spectral curve");
		}
		else if (statement == L"Ka xyz")
		{
			assert(false && "No support for the ambient reflectivity using CIEXYZ values");
		}
		else if (statement == L"Ka")
		{
			Vector3f& ambientColor = m_Materials[m_CurrentMaterialIndex].m_AmbientColor;
			fileStream >> ambientColor.m_X >> ambientColor.m_Y >> ambientColor.m_Z;
		}
		else if (statement == L"map_Ka")
		{
			fileStream >> m_Materials[m_CurrentMaterialIndex].m_AmbientMapName;
		}
		else if (statement == L"Kd spectral")
		{
			assert(false && "No support for the diffuse reflectivity using a spectral curve");
		}
		else if (statement == L"Kd xyz")
		{
			assert(false && "No support for the diffuse reflectivity using CIEXYZ values");
		}
		else if (statement == L"Kd")
		{
			Vector3f& diffuseColor = m_Materials[m_CurrentMaterialIndex].m_DiffuseColor;
			fileStream >> diffuseColor.m_X >> diffuseColor.m_Y >> diffuseColor.m_Z;
		}
		else if (statement == L"map_Kd")
		{
			fileStream >> m_Materials[m_CurrentMaterialIndex].m_DiffuseMapName;
		}
		else if (statement == L"Ks spectral")
		{
			assert(false && "No support for the specular reflectivity using a spectral curve");
		}
		else if (statement == L"Ks xyz")
		{
			assert(false && "No support for the specular reflectivity using CIEXYZ values");
		}
		else if (statement == L"Ks")
		{
			Vector3f& specularColor = m_Materials[m_CurrentMaterialIndex].m_SpecularColor;
			fileStream >> specularColor.m_X >> specularColor.m_Y >> specularColor.m_Z;
		}
		else if (statement == L"map_Ks")
		{
			fileStream >> m_Materials[m_CurrentMaterialIndex].m_SpecularMapName;
		}
		else if (statement == L"Ns")
		{
			fileStream >> m_Materials[m_CurrentMaterialIndex].m_SpecularPower;
		}
		else if (statement == L"map_Ns")
		{
			fileStream >> m_Materials[m_CurrentMaterialIndex].m_SpecularPowerMapName;
		}
		else if (statement == L"Ke spectral")
		{
			assert(false && "No support for the emissive reflectivity using a spectral curve");
		}
		else if (statement == L"Ke xyz")
		{
			assert(false && "No support for the emissive reflectivity using CIEXYZ values");
		}
		else if (statement == L"Ke")
		{
			Vector3f& emissiveColor = m_Materials[m_CurrentMaterialIndex].m_EmissiveColor;
			fileStream >> emissiveColor.m_X >> emissiveColor.m_Y >> emissiveColor.m_Z;
		}
		else if (statement == L"map_Ke")
		{
			fileStream >> m_Materials[m_CurrentMaterialIndex].m_EmissiveMapName;
		}
		else if (statement == L"Ni")
		{
			fileStream >> m_Materials[m_CurrentMaterialIndex].m_IndexOfRefraction;
		}
		else if (statement == L"d")
		{
			fileStream >> m_Materials[m_CurrentMaterialIndex].m_Opacity;
		}
		else if (statement == L"map_d")
		{
			fileStream >> m_Materials[m_CurrentMaterialIndex].m_OpacityMapName;
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
	for (const OBJFile::Object& object : m_Objects)
	{
		for (u32 meshIndex : object.m_MeshIndices)
		{
			const OBJFile::Mesh& mesh = m_Meshes[meshIndex];

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

	OBJFile::Material& defaultMaterial = m_Materials.back();
	defaultMaterial.m_DiffuseColor = Vector3f(0.6f, 0.6f, 0.0f);
	defaultMaterial.m_Opacity = 1.0f;
	defaultMaterial.m_IndexOfRefraction = 1.0f;
	defaultMaterial.m_SpecularPower = 0.0f;
		
	m_Objects.clear();
	m_pCurrentObject = nullptr;
	
	m_Meshes.clear();
	m_pCurrentMesh = nullptr;

	m_CurrentGroupName = OBJFile::kDefaultGroupName;
}
