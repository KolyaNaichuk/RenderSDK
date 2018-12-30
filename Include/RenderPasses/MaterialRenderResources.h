#pragma once

#include "Common/Common.h"
#include "D3DWrapper/DescriptorHeap.h"

struct RenderEnv;
struct Material;
class ColorTexture;
class Buffer;

class MaterialRenderResources
{
public:
	MaterialRenderResources(RenderEnv* pRenderEnv, u16 numMaterials, Material** ppMaterials);
	~MaterialRenderResources();
	
	u16 GetNumTextures() const { return (u16)m_Textures.size(); }
	ColorTexture** GetTextures() { return m_Textures.data(); }
		
	Buffer* GetMeshTypePerMaterialIDBuffer() { return m_pMeshTypePerMaterialIDBuffer; }
	Buffer* GetMaterialTextureIndicesBuffer() { return m_pMaterialTextureIndicesBuffer; }

private:
	Buffer* m_pMeshTypePerMaterialIDBuffer = nullptr;
	Buffer* m_pMaterialTextureIndicesBuffer = nullptr;	
	std::vector<ColorTexture*> m_Textures;
};
