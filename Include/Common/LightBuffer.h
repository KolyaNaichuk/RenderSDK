#pragma once

#include "Common/Common.h"

class CommandList;
class Buffer;
struct RenderEnv;

class PointLight;
class SpotLight;

class LightBuffer
{
public:
	LightBuffer(RenderEnv* pRenderEnv, u32 numPointLights, PointLight** ppPointLights);
	LightBuffer(RenderEnv* pRenderEnv, u32 numSpotLights, SpotLight** ppSpotLights);

	~LightBuffer();

	void RecordDataForUpload(CommandList* pCommandList);
	void RemoveDataForUpload();

	u32 GetNumLights() const { return m_NumLights; }

	Buffer* GetLightBoundsBuffer() { return m_pLightBoundsBuffer; }
	Buffer* GetLightPropsBuffer() { return m_pLightPropsBuffer; }
	Buffer* GetLightFrustumBuffer() { return m_pLightFrustumBuffer; }
	Buffer* GetLightViewProjMatrixBuffer() { return m_pLightViewProjMatrixBuffer; }

private:
	u32 m_NumLights;
	
	Buffer* m_pUploadLightBoundsBuffer;
	Buffer* m_pUploadLightPropsBuffer;
	Buffer* m_pUploadLightFrustumBuffer;
	Buffer* m_pUploadLightViewProjMatrixBuffer;

	Buffer* m_pLightBoundsBuffer;
	Buffer* m_pLightPropsBuffer;
	Buffer* m_pLightFrustumBuffer;
	Buffer* m_pLightViewProjMatrixBuffer;
};