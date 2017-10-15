#pragma once

#include "Common/Light.h"
#include "Common/Material.h"
#include "Common/MeshBatch.h"
#include "Math/AxisAlignedBox.h"

class Scene
{
public:
	Scene();
	~Scene();

	const AxisAlignedBox& GetWorldBounds() const;

	void AddMeshBatch(MeshBatch* pMeshBatch);
	std::size_t GetNumMeshBatches() const;
	MeshBatch** GetMeshBatches();
		
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
	AxisAlignedBox m_WorldBounds;
	std::vector<MeshBatch*> m_MeshBatches;
	std::vector<Material*> m_Materials;
	DirectionalLight* m_pDirectionalLight;
	std::vector<PointLight*> m_PointLights;
	std::vector<SpotLight*> m_SpotLights;

};