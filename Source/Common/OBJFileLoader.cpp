#include "Common/OBJFileLoader.h"
#include "Common/FileUtilities.h"
#include "Common/MeshBatchData.h"
#include "Common/MeshData.h"
#include "Common/MeshDataUtilities.h"
#include "Math/Hash.h"
#include <cwctype>
#include <limits>
#include <experimental/filesystem>

static const u16 MAX_STRING_LENGTH = 256;

namespace OBJFile
{
	std::wstring AnsiToWideString(const char* pAnsiString)
	{
		int ansiStringLength = std::strlen(pAnsiString);
		assert(ansiStringLength > 0);

		int wideStringLength = MultiByteToWideChar(CP_ACP, 0, pAnsiString, ansiStringLength, nullptr, 0);
		std::wstring wideString(wideStringLength, L'\0');
		int numWrittenCharacters = MultiByteToWideChar(CP_ACP, 0, pAnsiString, ansiStringLength, &wideString[0], wideStringLength);
		assert(numWrittenCharacters == wideStringLength);

		return wideString;
	}

	std::vector<MeshTriangle> Triangulate(const MeshFace& meshFace)
	{
		assert(meshFace.size() > 2);

		std::vector<MeshTriangle> meshTriangles;
		for (u32 index = 2; index < meshFace.size(); ++index)
			meshTriangles.emplace_back(MeshTriangle{meshFace[0], meshFace[index - 1], meshFace[index]});
		
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
		std::experimental::filesystem::path materialFilePath(pOBJFilePath);
		materialFilePath.replace_filename(m_MaterialFileName);
		
		bool loadedMaterialFile = LoadMaterialFile(materialFilePath.c_str());
		if (!loadedMaterialFile)
			return nullptr;
	}

	return GenerateMeshBatchData(use32BitIndices, convertMeshDataFlags);
}

bool OBJFileLoader::LoadOBJFile(const wchar_t* pFilePath, bool use32BitIndices, u8 convertMeshDataFlags)
{
	std::vector<char> byteStringFileData;
	if (!LoadDataFromFile(pFilePath, FileMode::Text, byteStringFileData))
		return false;

	std::wstring wideStringFileData = OBJFile::AnsiToWideString(&byteStringFileData[0]);

	wchar_t* pLineContext = nullptr;
	wchar_t* pLine = wcstok_s(&wideStringFileData[0], L"\n", &pLineContext);

	while (pLine != nullptr)
	{
		wchar_t* pTokenContext = nullptr;
		wchar_t* pToken = wcstok_s(pLine, L" \t\r", &pTokenContext);

		if (pToken != nullptr)
		{
			if (_wcsicmp(pToken, L"v") == 0)
			{
				f32 x = f32(_wtof(wcstok_s(nullptr, L" \t", &pTokenContext)));
				f32 y = f32(_wtof(wcstok_s(nullptr, L" \t", &pTokenContext)));
				f32 z = f32(_wtof(wcstok_s(nullptr, L" \t", &pTokenContext)));

				m_Positions.emplace_back(x, y, z);
			}
			else if (_wcsicmp(pToken, L"vt") == 0)
			{
				f32 u = f32(_wtof(wcstok_s(nullptr, L" \t", &pTokenContext)));
				f32 v = f32(_wtof(wcstok_s(nullptr, L" \t", &pTokenContext)));

				m_TexCoords.emplace_back(u, v);
			}
			else if (_wcsicmp(pToken, L"vn") == 0)
			{
				f32 x = f32(_wtof(wcstok_s(nullptr, L" \t", &pTokenContext)));
				f32 y = f32(_wtof(wcstok_s(nullptr, L" \t", &pTokenContext)));
				f32 z = f32(_wtof(wcstok_s(nullptr, L" \t", &pTokenContext)));

				m_Normals.emplace_back(x, y, z);
			}
			else if (_wcsicmp(pToken, L"f") == 0)
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

				OBJFile::MeshFace meshFace;
				meshFace.reserve(3);

				while (wchar_t* pFaceElementToken = wcstok_s(nullptr, L" \t\r", &pTokenContext))
				{
					i32 positionIndex, texCoordIndex, normalIndex;
					int numConvertedValues = swscanf_s(pFaceElementToken, L"%d/%d/%d", &positionIndex, &texCoordIndex, &normalIndex);
					assert(numConvertedValues == 3);

					assert(positionIndex > 0);
					--positionIndex;

					assert(texCoordIndex > 0);
					--texCoordIndex;

					assert(normalIndex > 0);
					--normalIndex;

					meshFace.emplace_back(positionIndex, normalIndex, texCoordIndex);
				}

				auto meshTriangles = Triangulate(meshFace);
				m_pCurrentMesh->m_Triangles.insert(m_pCurrentMesh->m_Triangles.end(), meshTriangles.cbegin(), meshTriangles.cend());
			}
			else if (_wcsicmp(pToken, L"#") == 0)
			{
				// Ignore comment
			}
			else if (_wcsicmp(pToken, L"s") == 0)
			{
				// Ignore smoothing group
			}
			else if (_wcsicmp(pToken, L"usemtl") == 0)
			{
				wchar_t* pMaterialToken = wcstok_s(nullptr, L" \t", &pTokenContext);
				assert(pMaterialToken != nullptr);
							
				wchar_t materialName[MAX_STRING_LENGTH];
				::ZeroMemory(materialName, sizeof(materialName));
				int numConvertedValues = swscanf_s(pMaterialToken, L"%s", &materialName[0], MAX_STRING_LENGTH);
				assert(numConvertedValues == 1);

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
			else if (_wcsicmp(pToken, L"g") == 0)
			{
				wchar_t* pGroupToken = wcstok_s(nullptr, L" \t", &pTokenContext);
				assert(pGroupToken != nullptr);

				wchar_t groupName[MAX_STRING_LENGTH];
				::ZeroMemory(groupName, sizeof(groupName));
				int numConvertedValues = swscanf_s(pGroupToken, L"%s", &groupName[0], MAX_STRING_LENGTH);
				assert(numConvertedValues == 1);

				if (m_CurrentGroupName != groupName)
				{
					m_Objects.emplace_back(groupName);
					m_pCurrentObject = &m_Objects.back();
					m_pCurrentMesh = nullptr;

					m_CurrentGroupName = groupName;
				}
			}
			else if (_wcsicmp(pToken, L"o") == 0)
			{
				wchar_t* pObjectToken = wcstok_s(nullptr, L" \t", &pTokenContext);
				assert(pObjectToken != nullptr);

				wchar_t objectName[MAX_STRING_LENGTH];
				::ZeroMemory(objectName, sizeof(objectName));
				int numConvertedValues = swscanf_s(pObjectToken, L"%s", &objectName[0], MAX_STRING_LENGTH);
				assert(numConvertedValues == 1);

				if ((m_pCurrentObject == nullptr) || (m_pCurrentObject->m_Name != objectName))
				{
					m_Objects.emplace_back(objectName);
					m_pCurrentObject = &m_Objects.back();
					m_pCurrentMesh = nullptr;
				}
			}
			else if (_wcsicmp(pToken, L"mtllib") == 0)
			{
				wchar_t* pFileNameToken = wcstok_s(nullptr, L" \t", &pTokenContext);
				assert(pFileNameToken != nullptr);

				wchar_t fileName[MAX_STRING_LENGTH];
				::ZeroMemory(fileName, sizeof(fileName));
				int numConvertedValues = swscanf_s(pFileNameToken, L"%s", &fileName[0], MAX_STRING_LENGTH);
				assert(numConvertedValues == 1);

				m_MaterialFileName = fileName;
			}
		}
		pLine = wcstok_s(nullptr, L"\n", &pLineContext);
	}
	return true;
}

bool OBJFileLoader::LoadMaterialFile(const wchar_t* pFilePath)
{
	std::vector<char> byteStringFileData;
	if (!LoadDataFromFile(pFilePath, FileMode::Text, byteStringFileData))
		return false;

	std::wstring wideStringFileData = OBJFile::AnsiToWideString(&byteStringFileData[0]);

	wchar_t* pLineContext = nullptr;
	wchar_t* pLine = wcstok_s(&wideStringFileData[0], L"\n", &pLineContext);

	while (pLine != nullptr)
	{
		wchar_t* pTokenContext = nullptr;
		wchar_t* pToken = wcstok_s(pLine, L" \t\r", &pTokenContext);

		if (pToken != nullptr)
		{
			if (_wcsicmp(pToken, L"#") == 0)
			{
				// Ignore comment
			}
			else if (_wcsicmp(pToken, L"newmtl") == 0)
			{
				wchar_t* pMaterialToken = wcstok_s(nullptr, L" \t", &pTokenContext);
				assert(pMaterialToken != nullptr);

				wchar_t materialName[MAX_STRING_LENGTH];
				::ZeroMemory(materialName, sizeof(materialName));
				int numConvertedValues = swscanf_s(pMaterialToken, L"%s", &materialName[0], MAX_STRING_LENGTH);
				assert(numConvertedValues == 1);

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
			else if (_wcsicmp(pToken, L"Ka spectral") == 0)
			{
				assert(false && "No support for the ambient reflectivity using a spectral curve");
			}
			else if (_wcsicmp(pToken, L"Ka xyz") == 0)
			{
				assert(false && "No support for the ambient reflectivity using CIEXYZ values");
			}
			else if (_wcsicmp(pToken, L"Ka") == 0)
			{
				m_Materials[m_CurrentMaterialIndex].m_AmbientColor.m_X = f32(_wtof(wcstok_s(nullptr, L" \t", &pTokenContext)));
				m_Materials[m_CurrentMaterialIndex].m_AmbientColor.m_Y = f32(_wtof(wcstok_s(nullptr, L" \t", &pTokenContext)));
				m_Materials[m_CurrentMaterialIndex].m_AmbientColor.m_Z = f32(_wtof(wcstok_s(nullptr, L" \t", &pTokenContext)));
			}
			else if (_wcsicmp(pToken, L"map_Ka") == 0)
			{
				wchar_t* pMapNameToken = wcstok_s(nullptr, L" \t", &pTokenContext);
				assert(pMapNameToken != nullptr);

				wchar_t mapName[MAX_STRING_LENGTH];
				::ZeroMemory(mapName, sizeof(mapName));
				int numConvertedValues = swscanf_s(pMapNameToken, L"%s", &mapName[0], MAX_STRING_LENGTH);
				assert(numConvertedValues == 1);

				m_Materials[m_CurrentMaterialIndex].m_AmbientMapName = mapName;
			}
			else if (_wcsicmp(pToken, L"Kd spectral") == 0)
			{
				assert(false && "No support for the diffuse reflectivity using a spectral curve");
			}
			else if (_wcsicmp(pToken, L"Kd xyz") == 0)
			{
				assert(false && "No support for the diffuse reflectivity using CIEXYZ values");
			}
			else if (_wcsicmp(pToken, L"Kd") == 0)
			{
				m_Materials[m_CurrentMaterialIndex].m_DiffuseColor.m_X = f32(_wtof(wcstok_s(nullptr, L" \t", &pTokenContext)));
				m_Materials[m_CurrentMaterialIndex].m_DiffuseColor.m_Y = f32(_wtof(wcstok_s(nullptr, L" \t", &pTokenContext)));
				m_Materials[m_CurrentMaterialIndex].m_DiffuseColor.m_Z = f32(_wtof(wcstok_s(nullptr, L" \t", &pTokenContext)));
			}
			else if (_wcsicmp(pToken, L"map_Kd") == 0)
			{
				wchar_t* pMapNameToken = wcstok_s(nullptr, L" \t", &pTokenContext);
				assert(pMapNameToken != nullptr);

				wchar_t mapName[MAX_STRING_LENGTH];
				::ZeroMemory(mapName, sizeof(mapName));
				int numConvertedValues = swscanf_s(pMapNameToken, L"%s", &mapName[0], MAX_STRING_LENGTH);
				assert(numConvertedValues == 1);

				m_Materials[m_CurrentMaterialIndex].m_DiffuseMapName = mapName;
			}
			else if (_wcsicmp(pToken, L"Ks spectral") == 0)
			{
				assert(false && "No support for the specular reflectivity using a spectral curve");
			}
			else if (_wcsicmp(pToken, L"Ks xyz") == 0)
			{
				assert(false && "No support for the specular reflectivity using CIEXYZ values");
			}
			else if (_wcsicmp(pToken, L"Ks") == 0)
			{
				m_Materials[m_CurrentMaterialIndex].m_SpecularColor.m_X = f32(_wtof(wcstok_s(nullptr, L" \t", &pTokenContext)));
				m_Materials[m_CurrentMaterialIndex].m_SpecularColor.m_Y = f32(_wtof(wcstok_s(nullptr, L" \t", &pTokenContext)));
				m_Materials[m_CurrentMaterialIndex].m_SpecularColor.m_Z = f32(_wtof(wcstok_s(nullptr, L" \t", &pTokenContext)));
			}
			else if (_wcsicmp(pToken, L"map_Ks") == 0)
			{
				wchar_t* pMapNameToken = wcstok_s(nullptr, L" \t", &pTokenContext);
				assert(pMapNameToken != nullptr);

				wchar_t mapName[MAX_STRING_LENGTH];
				::ZeroMemory(mapName, sizeof(mapName));
				int numConvertedValues = swscanf_s(pMapNameToken, L"%s", &mapName[0], MAX_STRING_LENGTH);
				assert(numConvertedValues == 1);

				m_Materials[m_CurrentMaterialIndex].m_SpecularMapName = mapName;
			}
			else if (_wcsicmp(pToken, L"Ke spectral") == 0)
			{
				assert(false && "No support for the emissive reflectivity using a spectral curve");
			}
			else if (_wcsicmp(pToken, L"Ke xyz") == 0)
			{
				assert(false && "No support for the emissive reflectivity using CIEXYZ values");
			}
			else if (_wcsicmp(pToken, L"Ke") == 0)
			{
				m_Materials[m_CurrentMaterialIndex].m_EmissiveColor.m_X = f32(_wtof(wcstok_s(nullptr, L" \t", &pTokenContext)));
				m_Materials[m_CurrentMaterialIndex].m_EmissiveColor.m_Y = f32(_wtof(wcstok_s(nullptr, L" \t", &pTokenContext)));
				m_Materials[m_CurrentMaterialIndex].m_EmissiveColor.m_Z = f32(_wtof(wcstok_s(nullptr, L" \t", &pTokenContext)));
			}
			else if (_wcsicmp(pToken, L"map_Ke") == 0)
			{
				wchar_t* pMapNameToken = wcstok_s(nullptr, L" \t", &pTokenContext);
				assert(pMapNameToken != nullptr);

				wchar_t mapName[MAX_STRING_LENGTH];
				::ZeroMemory(mapName, sizeof(mapName));
				int numConvertedValues = swscanf_s(pMapNameToken, L"%s", &mapName[0], MAX_STRING_LENGTH);
				assert(numConvertedValues == 1);

				m_Materials[m_CurrentMaterialIndex].m_EmissiveMapName = mapName;
			}
			else if (_wcsicmp(pToken, L"Ns") == 0)
			{
				m_Materials[m_CurrentMaterialIndex].m_SpecularPower = f32(_wtof(wcstok_s(nullptr, L" \t", &pTokenContext)));
			}
			else if (_wcsicmp(pToken, L"map_Ns") == 0)
			{
				wchar_t* pMapNameToken = wcstok_s(nullptr, L" \t", &pTokenContext);
				assert(pMapNameToken != nullptr);

				wchar_t mapName[MAX_STRING_LENGTH];
				::ZeroMemory(mapName, sizeof(mapName));
				int numConvertedValues = swscanf_s(pMapNameToken, L"%s", &mapName[0], MAX_STRING_LENGTH);
				assert(numConvertedValues == 1);

				m_Materials[m_CurrentMaterialIndex].m_SpecularPowerMapName = mapName;
			}
			else if (_wcsicmp(pToken, L"d") == 0)
			{
				m_Materials[m_CurrentMaterialIndex].m_Opacity = f32(_wtof(wcstok_s(nullptr, L" \t", &pTokenContext)));
			}
			else if (_wcsicmp(pToken, L"map_d") == 0)
			{
				wchar_t* pMapNameToken = wcstok_s(nullptr, L" \t", &pTokenContext);
				assert(pMapNameToken != nullptr);

				wchar_t mapName[MAX_STRING_LENGTH];
				::ZeroMemory(mapName, sizeof(mapName));
				int numConvertedValues = swscanf_s(pMapNameToken, L"%s", &mapName[0], MAX_STRING_LENGTH);
				assert(numConvertedValues == 1);

				m_Materials[m_CurrentMaterialIndex].m_OpacityMapName = mapName;
			}
			else if (_wcsicmp(pToken, L"Ni") == 0)
			{
				m_Materials[m_CurrentMaterialIndex].m_IndexOfRefraction = f32(_wtof(wcstok_s(nullptr, L" \t", &pTokenContext)));
			}
		}
		pLine = wcstok_s(nullptr, L"\n", &pLineContext);
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
