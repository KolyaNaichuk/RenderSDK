#include "Common/OBJFileLoader.h"
#include "Common/FileUtilities.h"
#include "Common/MeshBatchData.h"
#include "Common/MeshData.h"
#include "Common/MeshDataUtilities.h"
#include "Common/Scene.h"
#include "Math/Hash.h"
#include <cwctype>
#include <limits>
#include <experimental/filesystem>

namespace OBJFile
{
	FaceElement ExtractFaceElement(const wchar_t* pFaceElementToken)
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

		return FaceElement(positionIndex, normalIndex, texCoordIndex);
	}

	f32 ExtractFloat(wchar_t** ppTokenContext)
	{
		return f32(_wtof(wcstok_s(nullptr, L" \t", ppTokenContext)));
	}
		
	std::wstring ExtractString(wchar_t** ppTokenContext)
	{
		static const u16 MAX_STRING_LENGTH = 256;

		wchar_t* pStringToken = wcstok_s(nullptr, L" \t", ppTokenContext);
		assert(pStringToken != nullptr);

		wchar_t stringBuffer[MAX_STRING_LENGTH];
		::ZeroMemory(stringBuffer, sizeof(stringBuffer));
		
		int numConvertedValues = swscanf_s(pStringToken, L"%s", &stringBuffer[0], MAX_STRING_LENGTH);
		assert(numConvertedValues == 1);

		return std::wstring(stringBuffer);
	}
		
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

Scene* OBJFileLoader::Load(const wchar_t* pOBJFilePath, bool use32BitIndices, u8 convertMeshDataFlags)
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

	return PopulateScene(use32BitIndices, convertMeshDataFlags);
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
				f32 x = OBJFile::ExtractFloat(&pTokenContext);
				f32 y = OBJFile::ExtractFloat(&pTokenContext);
				f32 z = OBJFile::ExtractFloat(&pTokenContext);

				m_Positions.emplace_back(x, y, z);
			}
			else if (_wcsicmp(pToken, L"vt") == 0)
			{
				f32 u = OBJFile::ExtractFloat(&pTokenContext);
				f32 v = OBJFile::ExtractFloat(&pTokenContext);

				m_TexCoords.emplace_back(u, v);
			}
			else if (_wcsicmp(pToken, L"vn") == 0)
			{
				f32 x = OBJFile::ExtractFloat(&pTokenContext);
				f32 y = OBJFile::ExtractFloat(&pTokenContext);
				f32 z = OBJFile::ExtractFloat(&pTokenContext);

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
					meshFace.emplace_back(OBJFile::ExtractFaceElement(pFaceElementToken));
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
				std::wstring materialName = OBJFile::ExtractString(&pTokenContext);
				if (m_Materials[m_CurrentMaterialIndex].m_Name != materialName)
				{
					m_CurrentMaterialIndex = FindMaterialIndex(materialName);
					if (m_CurrentMaterialIndex == OBJFile::kUnknownIndex)
					{
						m_CurrentMaterialIndex = m_Materials.size();
						m_Materials.emplace_back(materialName);
					}
				}
			}
			else if (_wcsicmp(pToken, L"g") == 0)
			{
				std::wstring groupName = OBJFile::ExtractString(&pTokenContext);
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
				std::wstring objectName = OBJFile::ExtractString(&pTokenContext);
				if ((m_pCurrentObject == nullptr) || (m_pCurrentObject->m_Name != objectName))
				{
					m_Objects.emplace_back(objectName);
					m_pCurrentObject = &m_Objects.back();
					m_pCurrentMesh = nullptr;
				}
			}
			else if (_wcsicmp(pToken, L"mtllib") == 0)
			{
				m_MaterialFileName = OBJFile::ExtractString(&pTokenContext);
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
				std::wstring materialName = OBJFile::ExtractString(&pTokenContext);

				m_CurrentMaterialIndex = FindMaterialIndex(materialName);
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
				m_Materials[m_CurrentMaterialIndex].m_AmbientColor.m_X = OBJFile::ExtractFloat(&pTokenContext);
				m_Materials[m_CurrentMaterialIndex].m_AmbientColor.m_Y = OBJFile::ExtractFloat(&pTokenContext);
				m_Materials[m_CurrentMaterialIndex].m_AmbientColor.m_Z = OBJFile::ExtractFloat(&pTokenContext);
			}
			else if (_wcsicmp(pToken, L"map_Ka") == 0)
			{
				m_Materials[m_CurrentMaterialIndex].m_AmbientMapName = OBJFile::ExtractString(&pTokenContext);
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
				m_Materials[m_CurrentMaterialIndex].m_DiffuseColor.m_X = OBJFile::ExtractFloat(&pTokenContext);
				m_Materials[m_CurrentMaterialIndex].m_DiffuseColor.m_Y = OBJFile::ExtractFloat(&pTokenContext);
				m_Materials[m_CurrentMaterialIndex].m_DiffuseColor.m_Z = OBJFile::ExtractFloat(&pTokenContext);
			}
			else if (_wcsicmp(pToken, L"map_Kd") == 0)
			{
				m_Materials[m_CurrentMaterialIndex].m_DiffuseMapName = OBJFile::ExtractString(&pTokenContext);
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
				m_Materials[m_CurrentMaterialIndex].m_SpecularColor.m_X = OBJFile::ExtractFloat(&pTokenContext);
				m_Materials[m_CurrentMaterialIndex].m_SpecularColor.m_Y = OBJFile::ExtractFloat(&pTokenContext);
				m_Materials[m_CurrentMaterialIndex].m_SpecularColor.m_Z = OBJFile::ExtractFloat(&pTokenContext);
			}
			else if (_wcsicmp(pToken, L"map_Ks") == 0)
			{
				m_Materials[m_CurrentMaterialIndex].m_SpecularMapName = OBJFile::ExtractString(&pTokenContext);
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
				m_Materials[m_CurrentMaterialIndex].m_EmissiveColor.m_X = OBJFile::ExtractFloat(&pTokenContext);
				m_Materials[m_CurrentMaterialIndex].m_EmissiveColor.m_Y = OBJFile::ExtractFloat(&pTokenContext);
				m_Materials[m_CurrentMaterialIndex].m_EmissiveColor.m_Z = OBJFile::ExtractFloat(&pTokenContext);
			}
			else if (_wcsicmp(pToken, L"map_Ke") == 0)
			{
				m_Materials[m_CurrentMaterialIndex].m_EmissiveMapName = OBJFile::ExtractString(&pTokenContext);
			}
			else if (_wcsicmp(pToken, L"Ns") == 0)
			{
				m_Materials[m_CurrentMaterialIndex].m_SpecularPower = OBJFile::ExtractFloat(&pTokenContext);
			}
			else if (_wcsicmp(pToken, L"map_Ns") == 0)
			{
				m_Materials[m_CurrentMaterialIndex].m_SpecularPowerMapName = OBJFile::ExtractString(&pTokenContext);
			}
			else if (_wcsicmp(pToken, L"d") == 0)
			{
				m_Materials[m_CurrentMaterialIndex].m_Opacity = OBJFile::ExtractFloat(&pTokenContext);
			}
			else if (_wcsicmp(pToken, L"map_d") == 0)
			{
				m_Materials[m_CurrentMaterialIndex].m_OpacityMapName = OBJFile::ExtractString(&pTokenContext);
			}
			else if (_wcsicmp(pToken, L"Ni") == 0)
			{
				m_Materials[m_CurrentMaterialIndex].m_IndexOfRefraction = OBJFile::ExtractFloat(&pTokenContext);
			}
		}
		pLine = wcstok_s(nullptr, L"\n", &pLineContext);
	}
	return true;
}

u32 OBJFileLoader::FindMaterialIndex(const std::wstring& materialName) const
{
	for (u32 index = 0; index < m_Materials.size(); ++index)
	{
		if (m_Materials[index].m_Name == materialName)
			return index;
	}
	return OBJFile::kUnknownIndex;
}

Scene* OBJFileLoader::PopulateScene(bool use32BitIndices, u8 convertMeshDataFlags)
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
		
	Scene* pScene = new Scene();
	
	MeshBatchData* pMeshBatchData = new MeshBatchData(vertexFormat, indexFormat, primitiveTopologyType, primitiveTopology);
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
						
			MeshData meshData(pVertexData, pIndexData, mesh.m_MaterialIndex, primitiveTopologyType, primitiveTopology);
			if (convertMeshDataFlags != 0)
				ConvertMeshData(&meshData, convertMeshDataFlags);
			meshData.RecalcAABB();

			pMeshBatchData->Append(&meshData);
		}
	}
	pScene->SetMeshBatchData(pMeshBatchData);

	std::vector<std::string> materialMasks;
	for (const OBJFile::Material& material : m_Materials)
	{
		Material* pMaterial = new Material(Vector4f(material.m_AmbientColor.m_X, material.m_AmbientColor.m_Y, material.m_AmbientColor.m_Z, 1.0f),
			Vector4f(material.m_DiffuseColor.m_X, material.m_DiffuseColor.m_Y, material.m_DiffuseColor.m_Z, 1.0f),
			Vector4f(material.m_SpecularColor.m_X, material.m_SpecularColor.m_Y, material.m_SpecularColor.m_Z, 1.0f),
			material.m_SpecularPower,
			Vector4f(material.m_EmissiveColor.m_X, material.m_EmissiveColor.m_Y, material.m_EmissiveColor.m_Z, 1.0f));

		pMaterial->m_AmbientMapName = material.m_AmbientMapName;
		pMaterial->m_DiffuseMapName = material.m_DiffuseMapName;
		pMaterial->m_SpecularMapName = material.m_SpecularMapName;
		pMaterial->m_SpecularPowerMapName = material.m_SpecularPowerMapName;
		pMaterial->m_EmissiveMapName = material.m_EmissiveMapName;

		pScene->AddMaterial(pMaterial);

		std::string mask(6, '\n');
		mask[0] = pMaterial->m_AmbientMapName.empty() ? '0' : '1';
		mask[1] = pMaterial->m_DiffuseMapName.empty() ? '0' : '1';
		mask[2] = pMaterial->m_SpecularMapName.empty() ? '0' : '1';
		mask[3] = pMaterial->m_SpecularPowerMapName.empty() ? '0' : '1';
		mask[4] = pMaterial->m_EmissiveMapName.empty() ? '0' : '1';

		if (mask == "00000\n")
			OutputDebugStringA("");

		materialMasks.emplace_back(std::move(mask));
	}

	std::sort(materialMasks.begin(), materialMasks.end());
	for (const auto& mask : materialMasks)
		OutputDebugStringA(mask.c_str());
	
	return pScene;
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
