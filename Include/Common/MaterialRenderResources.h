#pragma once

#include "Common/Common.h"

struct RenderEnv;
struct Material;
class ColorTexture;

class MaterialRenderResources
{
public:
	MaterialRenderResources(RenderEnv* pRenderEnv, u32 numMaterials, Material** ppMaterials);
	~MaterialRenderResources();
	
	u32 GetNumTextures() const { return m_Textures.size(); }

private:
	std::vector<ColorTexture*> m_Textures;
};