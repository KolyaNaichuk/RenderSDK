#include "Scene/OBJFileLoader.h"
#include "Common/FileUtilities.h"
#include "Common/StringUtilities.h"
#include "Scene/Material.h"
#include "Scene/Mesh.h"
#include "Scene/MeshBatch.h"
#include "Scene/Scene.h"
#include <experimental/filesystem>
#include "assimp/Importer.hpp"
#include "assimp/scene.h"
#include "assimp/postprocess.h"

namespace
{
	void AddAssimpMeshes(Scene* pScene, const aiScene* pAssimpScene, const Matrix4f& worldMatrix, bool use32BitIndices)
	{
		assert(pAssimpScene->HasMeshes());

		const u8 vertexFormat = VertexData::FormatFlag_Position | VertexData::FormatFlag_Normal | VertexData::FormatFlag_TexCoords;
		const DXGI_FORMAT indexFormat = use32BitIndices ? DXGI_FORMAT_R32_UINT : DXGI_FORMAT_R16_UINT;
		const D3D12_PRIMITIVE_TOPOLOGY_TYPE primitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		const D3D12_PRIMITIVE_TOPOLOGY primitiveTopology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

		MeshBatch* pMeshBatch = new MeshBatch(vertexFormat, indexFormat, primitiveTopologyType, primitiveTopology);

		for (decltype(pAssimpScene->mNumMeshes) meshIndex = 0; meshIndex < pAssimpScene->mNumMeshes; ++meshIndex)
		{
			const aiMesh* pAssimpMesh = pAssimpScene->mMeshes[meshIndex];

			assert(pAssimpMesh->HasPositions());
			assert(pAssimpMesh->HasNormals());
			assert(pAssimpMesh->HasTextureCoords(0));
			assert(pAssimpMesh->mPrimitiveTypes == aiPrimitiveType_TRIANGLE);
			
			Vector3f* pPositions = new Vector3f[pAssimpMesh->mNumVertices];
			Vector3f* pNormals = new Vector3f[pAssimpMesh->mNumVertices];
			Vector2f* pTexCoords = new Vector2f[pAssimpMesh->mNumVertices];

			for (decltype(pAssimpMesh->mNumVertices) vertexIndex = 0; vertexIndex < pAssimpMesh->mNumVertices; ++vertexIndex)
			{
				const aiVector3D& position = pAssimpMesh->mVertices[vertexIndex];
				pPositions[vertexIndex] = Vector3f(position.x, position.y, position.z);
			}
			for (decltype(pAssimpMesh->mNumVertices) vertexIndex = 0; vertexIndex < pAssimpMesh->mNumVertices; ++vertexIndex)
			{
				const aiVector3D& normal = pAssimpMesh->mNormals[vertexIndex];
				pNormals[vertexIndex] = Vector3f(normal.x, normal.y, normal.z);
			}
			for (decltype(pAssimpMesh->mNumVertices) vertexIndex = 0; vertexIndex < pAssimpMesh->mNumVertices; ++vertexIndex)
			{
				const aiVector3D& texCoords = pAssimpMesh->mTextureCoords[0][vertexIndex];
				pTexCoords[vertexIndex] = Vector2f(texCoords.x, texCoords.y);
			}

			const u32 numIndices = 3 * pAssimpMesh->mNumFaces;

			u16* p16BitIndices = nullptr;
			u32* p32BitIndices = nullptr;

			if (use32BitIndices)
			{
				p32BitIndices = new u32[numIndices];
				for (decltype(pAssimpMesh->mNumFaces) faceIndex = 0; faceIndex < pAssimpMesh->mNumFaces; ++faceIndex)
				{
					const aiFace& face = pAssimpMesh->mFaces[faceIndex];
					assert(face.mNumIndices == 3);

					p32BitIndices[3 * faceIndex + 0] = face.mIndices[0];
					p32BitIndices[3 * faceIndex + 1] = face.mIndices[1];
					p32BitIndices[3 * faceIndex + 2] = face.mIndices[2];
				}
			}
			else
			{
				p16BitIndices = new u16[numIndices];
				const u32 u16Max = std::numeric_limits<u16>::max();

				for (decltype(pAssimpMesh->mNumFaces) faceIndex = 0; faceIndex < pAssimpMesh->mNumFaces; ++faceIndex)
				{
					const aiFace& face = pAssimpMesh->mFaces[faceIndex];
					assert(face.mNumIndices == 3);
					assert(face.mIndices[0] <= u16Max);
					assert(face.mIndices[1] <= u16Max);
					assert(face.mIndices[2] <= u16Max);

					p16BitIndices[3 * faceIndex + 0] = u16(face.mIndices[0]);
					p16BitIndices[3 * faceIndex + 1] = u16(face.mIndices[1]);
					p16BitIndices[3 * faceIndex + 2] = u16(face.mIndices[2]);
				}
			}

			VertexData* pVertexData = new VertexData(pAssimpMesh->mNumVertices, pPositions, pNormals, pTexCoords);
			
			IndexData* pIndexData = nullptr;
			if (use32BitIndices)
				pIndexData = new IndexData(numIndices, p32BitIndices);
			else
				pIndexData = new IndexData(numIndices, p16BitIndices);

			const u8 numInstances = 1;
			Matrix4f* pInstanceWorldMatrices = new Matrix4f[numInstances];
			pInstanceWorldMatrices[0] = worldMatrix;

			Mesh mesh(pVertexData, pIndexData, numInstances, pInstanceWorldMatrices,
				pAssimpMesh->mMaterialIndex, primitiveTopologyType, primitiveTopology);

			pMeshBatch->AddMesh(&mesh);
		}
		pScene->AddMeshBatch(pMeshBatch);
	}

	void AddAssimpMaterials(Scene* pScene, const aiScene* pAssimpScene, const std::experimental::filesystem::path& materialDirectoryPath)
	{
		assert(pAssimpScene->HasMaterials());

		aiString assimpName;
		aiString assimpMapPath;

		for (decltype(pAssimpScene->mNumMaterials) materialIndex = 0; materialIndex < pAssimpScene->mNumMaterials; ++materialIndex)
		{
			const aiMaterial* pAssimpMaterial = pAssimpScene->mMaterials[materialIndex];
			
			pAssimpMaterial->Get(AI_MATKEY_NAME, assimpName);
			Material* pMaterial = new Material(AnsiToWideString(assimpName.C_Str()));

			if (pAssimpMaterial->GetTexture(aiTextureType_DIFFUSE, 0, &assimpMapPath) == aiReturn_SUCCESS)
				pMaterial->m_FilePaths[Material::BaseColorTextureIndex] = materialDirectoryPath / std::experimental::filesystem::path(AnsiToWideString(assimpMapPath.C_Str()));
			else
				assert(false);

			if (pAssimpMaterial->GetTexture(aiTextureType_AMBIENT, 0, &assimpMapPath) == aiReturn_SUCCESS)
				pMaterial->m_FilePaths[Material::MetallicTextureIndex] = materialDirectoryPath / std::experimental::filesystem::path(AnsiToWideString(assimpMapPath.C_Str()));
			else
				assert(false);

			if (pAssimpMaterial->GetTexture(aiTextureType_SHININESS, 0, &assimpMapPath) == aiReturn_SUCCESS)
				pMaterial->m_FilePaths[Material::RougnessTextureIndex] = materialDirectoryPath / std::experimental::filesystem::path(AnsiToWideString(assimpMapPath.C_Str()));
			else
				assert(false);
			
			pScene->AddMaterial(pMaterial);
		}
	}
}

Scene* LoadSceneFromOBJFile(const wchar_t* pFilePath, const Matrix4f& worldMatrix, bool use32BitIndices)
{
	Assimp::Importer importer;
	
	const u32 importFlags = aiProcess_CalcTangentSpace |
		aiProcess_GenNormals |
		aiProcess_Triangulate |
		aiProcess_SortByPType |
		aiProcess_MakeLeftHanded |
		aiProcess_FlipUVs |
		aiProcess_FlipWindingOrder |
		aiProcess_JoinIdenticalVertices |
		aiProcess_OptimizeMeshes |
		aiProcess_RemoveRedundantMaterials;
		
	const aiScene* pAssimpScene = importer.ReadFile(WideToAnsiString(pFilePath), importFlags);
	if (pAssimpScene == nullptr)
	{
		OutputDebugStringA(importer.GetErrorString());
		return nullptr;
	}

	Scene* pScene = new Scene();
	AddAssimpMeshes(pScene, pAssimpScene, worldMatrix, use32BitIndices);
			
	std::experimental::filesystem::path materialDirectoryPath(pFilePath);
	materialDirectoryPath.remove_filename();

	AddAssimpMaterials(pScene, pAssimpScene, materialDirectoryPath);
	
	return pScene;
}