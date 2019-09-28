#include "Scene/SceneLoader.h"
#include "Common/FileUtilities.h"
#include "Common/StringUtilities.h"
#include "D3DWrapper/Common.h"
#include "Math/Transform.h"
#include "Scene/Light.h"
#include "Scene/Scene.h"
#include "Scene/Material.h"
#include "Scene/Mesh.h"
#include "Scene/MeshBatch.h"
#include "Scene/Scene.h"
#include "assimp/Importer.hpp"
#include "assimp/scene.h"
#include "assimp/postprocess.h"

namespace
{
	const Vector3f ToVector3f(const aiVector3D& assimpVec);
	const Vector3f ToVector3f(const aiColor3D& assimpColor);
	const Vector2f ToVector2f(const aiVector3D& assimpVec);

	void AddAssimpMeshes(Scene* pScene, const aiScene* pAssimpScene, const Matrix4f& worldMatrix, bool use32BitIndices);
	void AddAssimpMaterials(Scene* pScene, const aiScene* pAssimpScene, const std::filesystem::path& materialDirectoryPath);

	Scene* LoadSceneFromFile(const wchar_t* pFilePath, const Matrix4f& worldMatrix, bool use32BitIndices = false);
}

Scene* SceneLoader::LoadCrytekSponza()
{
#ifdef ENABLE_EXTERNAL_TOOL_DEBUGGING
	const wchar_t* pFilePath = L"..\\..\\..\\Resources\\CrytekSponza\\sponza.obj";
#else
	const wchar_t* pFilePath = L"..\\..\\Resources\\CrytekSponza\\sponza.obj";
#endif
	Matrix4f matrix1 = CreateTranslationMatrix(60.5189209f, -651.495361f, -38.6905518f);
	Matrix4f matrix2 = CreateScalingMatrix(0.01f);
	Matrix4f matrix3 = CreateRotationYMatrix(PI_DIV_2);
	Matrix4f matrix4 = CreateTranslationMatrix(0.0f, 7.8f, 18.7f);
	Matrix4f worldMatrix = matrix1 * matrix2 * matrix3 * matrix4;

	Scene* pScene = LoadSceneFromFile(pFilePath, worldMatrix);

	const AxisAlignedBox& worldBounds = pScene->GetWorldBounds();
	// {-11.4411659, 0.020621, 0.095729}
	const Vector3f minPoint = worldBounds.m_Center - worldBounds.m_Radius;
	// {11.4411659, 15.5793781, 37.304271}
	const Vector3f maxPoint = worldBounds.m_Center + worldBounds.m_Radius;

	Camera* pCamera = new Camera(
		Vector3f(0.0f, 2.8f, 9.32f)/*worldPosition*/,
		BasisAxes()/*worldOrientation*/,
		PI_DIV_4/*fovYInRadians*/,
		1.0f/*aspectRatio*/,
		0.1f/*nearClipDist*/,
		60.0f/*farClipDist*/,
		Vector3f(0.01f)/*moveSpeed*/,
		Vector3f(0.001f)/*rotationSpeed*/
	);
	pScene->SetCamera(pCamera);

#if 1
	SpotLight* pSpotLight1 = new SpotLight(
		Vector3f(0.0f, 5.5f, 16.5f)/*worldPosition*/,
		BasisAxes(Vector3f::RIGHT, Vector3f::FORWARD, Vector3f::DOWN)/*worldOrientation*/,
		Vector3f(20.0f, 20.0f, 20.0f)/*radiantPower*/,
		7.5f/*range*/,
		ToRadians(60.0f)/*innerConeAngleInRadians*/,
		ToRadians(90.0f)/*outerConeAngleInRadians*/,
		0.1f/*shadowNearPlane*/,
		80.0f/*expShadowMapConstant*/
	);
	pScene->AddSpotLight(pSpotLight1);
#endif

#if 1
	SpotLight* pSpotLight2 = new SpotLight(
		Vector3f(0.0f, 7.0f, 23.5f)/*worldPosition*/,
		BasisAxes(Vector3f::RIGHT, Vector3f::FORWARD, Vector3f::DOWN)/*worldOrientation*/,
		Vector3f(20.0f, 20.0f, 20.0f)/*radiantPower*/,
		10.0f/*range*/,
		ToRadians(50.0f)/*innerConeAngleInRadians*/,
		ToRadians(70.0f)/*outerConeAngleInRadians*/,
		0.1f/*shadowNearPlane*/,
		80.0f/*expShadowMapConstant*/
	);
	pScene->AddSpotLight(pSpotLight2);
#endif

#if 1
	SpotLight* pSpotLight3 = new SpotLight(
		Vector3f(0.0f, 5.5f, 31.5f)/*worldPosition*/,
		BasisAxes(Vector3f::RIGHT, Vector3f::FORWARD, Vector3f::DOWN)/*worldOrientation*/,
		Vector3f(20.0f, 20.0f, 20.0f)/*radiantPower*/,
		7.5f/*range*/,
		ToRadians(60.0f)/*innerConeAngleInRadians*/,
		ToRadians(90.0f)/*outerConeAngleInRadians*/,
		0.1f/*shadowNearPlane*/,
		80.0f/*expShadowMapConstant*/
	);
	pScene->AddSpotLight(pSpotLight3);
#endif

#if 1
	SpotLight* pSpotLight4 = new SpotLight(
		Vector3f(0.0f, 7.5f, 23.9312f)/*worldPosition*/,
		BasisAxes()/*worldOrientation*/,
		Vector3f(20.0f, 20.0f, 20.0f)/*radiantPower*/,
		12.0f/*range*/,
		ToRadians(60.0f)/*innerConeAngleInRadians*/,
		ToRadians(75.0f)/*outerConeAngleInRadians*/,
		0.1f/*shadowNearPlane*/,
		80.0f/*expShadowMapConstant*/
	);
	pScene->AddSpotLight(pSpotLight4);
#endif

#if 1
	SpotLight* pSpotLight5 = new SpotLight(
		Vector3f(0.0f, 7.5f, 23.9312f)/*worldPosition*/,
		BasisAxes(Vector3f::BACK, Vector3f::UP, Vector3f::RIGHT)/*worldOrientation*/,
		Vector3f(20.0f, 20.0f, 20.0f)/*radiantPower*/,
		13.0f/*range*/,
		ToRadians(60.0f)/*innerConeAngleInRadians*/,
		ToRadians(90.0f)/*outerConeAngleInRadians*/,
		0.1f/*shadowNearPlane*/,
		80.0f/*expShadowMapConstant*/
	);
	pScene->AddSpotLight(pSpotLight5);
#endif

#if 1
	SpotLight* pSpotLight6 = new SpotLight(
		Vector3f(0.0f, 7.5f, 23.9312f)/*worldPosition*/,
		BasisAxes(Vector3f::FORWARD, Vector3f::UP, Vector3f::LEFT)/*worldOrientation*/,
		Vector3f(20.0f, 20.0f, 20.0f)/*radiantPower*/,
		13.0f/*range*/,
		ToRadians(60.0f)/*innerConeAngleInRadians*/,
		ToRadians(90.0f)/*outerConeAngleInRadians*/,
		0.1f/*shadowNearPlane*/,
		80.0f/*expShadowMapConstant*/
	);
	pScene->AddSpotLight(pSpotLight6);
#endif

	return pScene;
}

Scene* SceneLoader::LoadLivingRoom()
{
#ifdef ENABLE_EXTERNAL_TOOL_DEBUGGING
	const wchar_t* pFilePath = L"..\\..\\..\\Resources\\Living Room\\living_room.obj";
#else
	const wchar_t* pFilePath = L"..\\..\\Resources\\Living Room\\living_room.obj";
#endif

	Matrix4f worldMatrix = Matrix4f::IDENTITY;
	Scene* pScene = LoadSceneFromFile(pFilePath, worldMatrix, true);

	const AxisAlignedBox& worldBounds = pScene->GetWorldBounds();
	const Vector3f minPoint = worldBounds.m_Center - worldBounds.m_Radius;
	const Vector3f maxPoint = worldBounds.m_Center + worldBounds.m_Radius;

	return pScene;
}

namespace
{
	const Vector3f ToVector3f(const aiVector3D& assimpVec)
	{
		return Vector3f(assimpVec.x, assimpVec.y, assimpVec.z);
	}
	
	const Vector3f ToVector3f(const aiColor3D& assimpColor)
	{
		return Vector3f(assimpColor.r, assimpColor.g, assimpColor.b);
	}
	
	const Vector2f ToVector2f(const aiVector3D& assimpVec)
	{
		return Vector2f(assimpVec.x, assimpVec.y);
	}

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
				pPositions[vertexIndex] = ToVector3f(pAssimpMesh->mVertices[vertexIndex]);
			
			for (decltype(pAssimpMesh->mNumVertices) vertexIndex = 0; vertexIndex < pAssimpMesh->mNumVertices; ++vertexIndex)
				pNormals[vertexIndex] = ToVector3f(pAssimpMesh->mNormals[vertexIndex]);
			
			for (decltype(pAssimpMesh->mNumVertices) vertexIndex = 0; vertexIndex < pAssimpMesh->mNumVertices; ++vertexIndex)
				pTexCoords[vertexIndex] = ToVector2f(pAssimpMesh->mTextureCoords[0][vertexIndex]);
			
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

	void AddAssimpMaterials(Scene* pScene, const aiScene* pAssimpScene, const std::filesystem::path& materialDirectoryPath)
	{
		assert(pAssimpScene->HasMaterials());

		aiString assimpName;
		aiString assimpMapPath;

		for (decltype(pAssimpScene->mNumMaterials) materialIndex = 0; materialIndex < pAssimpScene->mNumMaterials; ++materialIndex)
		{
			const aiMaterial* pAssimpMaterial = pAssimpScene->mMaterials[materialIndex];

			aiReturn result = pAssimpMaterial->Get(AI_MATKEY_NAME, assimpName);
			assert(result == aiReturn_SUCCESS);
			Material* pMaterial = new Material(AnsiToWideString(assimpName.C_Str()));

			result = pAssimpMaterial->Get(AI_MATKEY_TEXTURE(aiTextureType_DIFFUSE, 0), assimpMapPath);
			assert(result == aiReturn_SUCCESS);
			pMaterial->m_FilePaths[Material::BaseColorTextureIndex] = materialDirectoryPath / assimpMapPath.C_Str();
			
			result = pAssimpMaterial->Get(AI_MATKEY_TEXTURE(aiTextureType_AMBIENT, 0), assimpMapPath);
			assert(result == aiReturn_SUCCESS);
			pMaterial->m_FilePaths[Material::MetalnessTextureIndex] = materialDirectoryPath / assimpMapPath.C_Str();
			
			result = pAssimpMaterial->Get(AI_MATKEY_TEXTURE(aiTextureType_SHININESS, 0), assimpMapPath);
			assert(result == aiReturn_SUCCESS);
			pMaterial->m_FilePaths[Material::RougnessTextureIndex] = materialDirectoryPath / assimpMapPath.C_Str();
						
			pScene->AddMaterial(pMaterial);
		}
	}

	Scene* LoadSceneFromFile(const wchar_t* pFilePath, const Matrix4f& worldMatrix, bool use32BitIndices)
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

		std::filesystem::path materialDirectoryPath(pFilePath);
		materialDirectoryPath.remove_filename();

		AddAssimpMaterials(pScene, pAssimpScene, materialDirectoryPath);

		return pScene;
	}
}
