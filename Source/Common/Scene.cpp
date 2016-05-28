#include "Common/Scene.h"

Scene::~Scene()
{
	for (std::size_t meshBatchIndex = 0; meshBatchIndex < m_MeshBatches.size(); ++meshBatchIndex)
		SafeDelete(m_MeshBatches[meshBatchIndex]);
	
	for (std::size_t pointLightIndex = 0; pointLightIndex < m_PointLights.size(); ++pointLightIndex)
		SafeDelete(m_PointLights[pointLightIndex]);
	
	for (std::size_t spotLightIndex = 0; spotLightIndex < m_SpotLights.size(); ++spotLightIndex)
		SafeDelete(m_SpotLights[spotLightIndex]);

	SafeDelete(m_pDirectionalLight);
}

void Scene::AddMeshBatch(MeshBatchData* pMeshBatch)
{
	m_MeshBatches.emplace_back(pMeshBatch);
}

std::size_t Scene::GetNumMeshBatches() const
{
	return m_MeshBatches.size();
}

MeshBatchData** Scene::GetMeshBatches()
{
	return m_MeshBatches.data();
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

const DirectionalLight* Scene::GetDirectionalLight() const
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