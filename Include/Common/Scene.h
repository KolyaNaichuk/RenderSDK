#pragma once

#include "Common/Light.h"
#include "Common/MeshBatchData.h"

struct Material;

class Scene
{
public:
	Scene();
	~Scene();

	const MeshBatchData* GetMeshBatchData() const;
	void SetMeshBatchData(MeshBatchData* pMeshBatch);
		
	void AddMaterial(Material* pMaterial);
	std::size_t GetNumMaterials() const;
	Material** GetMaterials();

	const DirectionalLight* GetDirectionalLight() const;
	void SetDirectionalLight(DirectionalLight* pDirectionalLight);

	void AddPointLight(PointLight* pPointLight);
	std::size_t GetNumPointLights() const;
	PointLight** GetPointLights();

	void AddSpotLight(SpotLight* pSpotLight);
	std::size_t GetNumSpotLights() const;
	SpotLight** GetSpotLights();
			
private:
	MeshBatchData* m_pMeshBatchData;
	std::vector<Material*> m_Materials;
	DirectionalLight* m_pDirectionalLight;
	std::vector<PointLight*> m_PointLights;
	std::vector<SpotLight*> m_SpotLights;
};