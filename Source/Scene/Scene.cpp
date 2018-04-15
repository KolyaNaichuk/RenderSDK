#include "Scene/Scene.h"

Scene::Scene()
	: m_WorldBounds(Vector3f::ZERO, Vector3f::ZERO)
	, m_pCamera(nullptr)
	, m_pDirectionalLight(nullptr)
{
}

Scene::~Scene()
{	
	SafeDelete(m_pCamera);
	SafeDelete(m_pDirectionalLight);

	for (decltype(m_MeshBatches.size()) index = 0; index < m_MeshBatches.size(); ++index)
		SafeDelete(m_MeshBatches[index]);

	for (decltype(m_Materials.size()) index = 0; index < m_Materials.size(); ++index)
		SafeDelete(m_Materials[index]);
	
	for (decltype(m_PointLights.size()) index = 0; index < m_PointLights.size(); ++index)
		SafeDelete(m_PointLights[index]);
	
	for (decltype(m_SpotLights.size()) index = 0; index < m_SpotLights.size(); ++index)
		SafeDelete(m_SpotLights[index]);
}

const AxisAlignedBox& Scene::GetWorldBounds() const
{
	return m_WorldBounds;
}

Camera* Scene::GetCamera()
{
	return m_pCamera;
}

void Scene::SetCamera(Camera* pCamera)
{
	if (m_pCamera != pCamera)
	{
		SafeDelete(m_pCamera);
		m_pCamera = pCamera;
	}
}

void Scene::AddMeshBatch(MeshBatch* pMeshBatch)
{
	const u32 numMeshInstances = pMeshBatch->GetNumMeshInstances();
	if (numMeshInstances > 0)
	{
		const AxisAlignedBox* instanceWorldAABBs = pMeshBatch->GetMeshInstanceWorldAABBs();

		u32 instanceIndex = 0;
		if (m_MeshBatches.empty())
			m_WorldBounds = instanceWorldAABBs[instanceIndex++];
		
		while (instanceIndex < numMeshInstances)
			m_WorldBounds = AxisAlignedBox(m_WorldBounds, instanceWorldAABBs[instanceIndex++]);

		m_MeshBatches.emplace_back(pMeshBatch);
	}
}

std::size_t Scene::GetNumMeshBatches() const
{
	return m_MeshBatches.size();
}

MeshBatch** Scene::GetMeshBatches()
{
	return m_MeshBatches.data();
}

void Scene::AddMaterial(Material* pMaterial)
{
	m_Materials.emplace_back(pMaterial);
}

std::size_t Scene::GetNumMaterials() const
{
	return m_Materials.size();
}

Material** Scene::GetMaterials()
{
	return m_Materials.data();
}

void Scene::AddPointLight(PointLight* pPointLight)
{
	m_PointLights.emplace_back(pPointLight);
}

std::size_t Scene::GetNumPointLights() const
{
	return m_PointLights.size();
}

PointLight** Scene::GetPointLights()
{
	return m_PointLights.data();
}

void Scene::AddSpotLight(SpotLight* pSpotLight)
{
	m_SpotLights.emplace_back(pSpotLight);
}

std::size_t Scene::GetNumSpotLights() const
{
	return m_SpotLights.size();
}

SpotLight** Scene::GetSpotLights()
{
	return m_SpotLights.data();
}

DirectionalLight* Scene::GetDirectionalLight()
{
	return m_pDirectionalLight;
}

void Scene::SetDirectionalLight(DirectionalLight* pDirectionalLight)
{
	if (m_pDirectionalLight != pDirectionalLight)
	{
		SafeDelete(m_pDirectionalLight);
		m_pDirectionalLight = pDirectionalLight;
	}
}