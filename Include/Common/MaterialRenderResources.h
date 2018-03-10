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
	MaterialRenderResources(RenderEnv* pRenderEnv, u16 numMaterials, Material** ppMaterials, bool forceSRGB);
	~MaterialRenderResources();
	
	u16 GetNumTextures() const { return (u16)m_Textures.size(); }
	ColorTexture** GetTextures() { return m_Textures.data(); }
		
	Buffer* GetMeshTypePerMaterialIDBuffer() { return m_pMeshTypePerMaterialIDBuffer; }
	Buffer* GetFirstResourceIndexPerMaterialIDBuffer() { return m_pFirstResourceIndexPerMaterialIDBuffer; }
	
private:
	void InitMeshTypePerMaterialIDBuffer(RenderEnv* pRenderEnv, u16 numMaterials);
	void InitFirstResourceIndexPerMaterialIDBuffer(RenderEnv* pRenderEnv, u16 numMaterials);
	void InitTextures(RenderEnv* pRenderEnv, u16 numMaterials, Material** ppMaterials, bool forceSRGB);

private:
	Buffer* m_pMeshTypePerMaterialIDBuffer;
	Buffer* m_pFirstResourceIndexPerMaterialIDBuffer;	
	std::vector<ColorTexture*> m_Textures;
};
