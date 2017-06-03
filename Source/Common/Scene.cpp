#include "Common/Scene.h"

Scene::Scene()
	: m_pMeshBatchData(nullptr)
	, m_pDirectionalLight(nullptr)
{
}

Scene::~Scene()
{
	SafeDelete(m_pMeshBatchData);
	
	for (std::size_t index = 0; index < m_Materials.size(); ++index)
		SafeDelete(m_Materials[index]);
	
	for (std::size_t index = 0; index < m_PointLights.size(); ++index)
		SafeDelete(m_PointLights[index]);
	
	for (std::size_t index = 0; index < m_SpotLights.size(); ++index)
		SafeDelete(m_SpotLights[index]);

	SafeDelete(m_pDirectionalLight);
}

const MeshBatchData* Scene::GetMeshBatchData() const
{
	return m_pMeshBatchData;
}

void Scene::SetMeshBatchData(MeshBatchData* pMeshBatch)
{
	if (m_pMeshBatchData != pMeshBatch)
	{
		SafeDelete(m_pMeshBatchData);
		m_pMeshBatchData = pMeshBatch;
	}
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