#pragma once

#include "Common/Common.h"

class CommandList;
class Buffer;
class PointLight;
class SpotLight;

struct RenderEnv;

class LightRenderResources
{
public:
	LightRenderResources(RenderEnv* pRenderEnv, u32 numPointLights, PointLight** ppPointLights);
	LightRenderResources(RenderEnv* pRenderEnv, u32 numSpotLights, SpotLight** ppSpotLights);
	
	~LightRenderResources();
	
	u32 GetNumLights() const { return m_NumLights; }

	Buffer* GetLightBoundsBuffer() { return m_pLightBoundsBuffer; }
	Buffer* GetLightPropsBuffer() { return m_pLightPropsBuffer; }
	Buffer* GetLightFrustumBuffer() { return m_pLightFrustumBuffer; }
	Buffer* GetLightViewProjMatrixBuffer() { return m_pLightViewProjMatrixBuffer; }

private:
	u32 m_NumLights;
	Buffer* m_pLightBoundsBuffer;
	Buffer* m_pLightPropsBuffer;
	Buffer* m_pLightFrustumBuffer;
	Buffer* m_pLightViewProjMatrixBuffer;
};