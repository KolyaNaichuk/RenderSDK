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
	DescriptorHandle GetTextureHeapStart() { return m_TextureHeapStart; }

	Buffer* GetMeshTypePerMaterialIDBuffer() { return m_pMeshTypePerMaterialIDBuffer; }
	Buffer* GetResourceInfoIndexPerMaterialIDBuffer() { return m_pResourceInfoIndexPerMaterialIDBuffer; }
	Buffer* GetResourceInfoBuffer() { return m_pResourceInfoBuffer; }
	
private:
	void InitMeshTypePerMaterialIDBuffer(RenderEnv* pRenderEnv, u16 numMaterials);
	void InitResourceInfoIndexPerMaterialIDBuffer(RenderEnv* pRenderEnv, u16 numMaterials);
	void InitResourceInfoBuffer(RenderEnv* pRenderEnv, u16 numMaterials);
	void InitTextures(RenderEnv* pRenderEnv, u16 numMaterials, Material** ppMaterials);

private:
	Buffer* m_pMeshTypePerMaterialIDBuffer;
	Buffer* m_pResourceInfoIndexPerMaterialIDBuffer;
	Buffer* m_pResourceInfoBuffer;

	std::vector<ColorTexture*> m_Textures;
	DescriptorHandle m_TextureHeapStart;
};
