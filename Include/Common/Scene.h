#pragma once

#include "Common/Light.h"
#include "Common/MeshBatchData.h"

class Scene
{
public:
	~Scene();

	void AddMeshBatch(MeshBatchData* pMeshBatch);
	std::size_t GetNumMeshBatches() const;
	MeshBatchData** GetMeshBatches();
	
	void AddPointLight(PointLight* pPointLight);
	std::size_t GetNumPointLights() const;
	PointLight** GetPointLights();

	void AddSpotLight(SpotLight* pSpotLight);
	std::size_t GetNumSpotLights() const;
	SpotLight** GetSpotLights();

	const DirectionalLight* GetDirectionalLight() const;
	void SetDirectionalLight(DirectionalLight* pDirectionalLight);
	
private:
	std::vector<MeshBatchData*> m_MeshBatches;
	std::vector<PointLight*> m_PointLights;
	std::vector<SpotLight*> m_SpotLights;
	DirectionalLight* m_pDirectionalLight;
};