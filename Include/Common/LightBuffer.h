#pragma once

#include "Common/Common.h"

class D3DCommandList;
class D3DBuffer;
struct D3DRenderEnv;

class PointLight;
class SpotLight;

class LightBuffer
{
public:
	LightBuffer(D3DRenderEnv* pRenderEnv, u32 numPointLights, PointLight** ppPointLights);
	LightBuffer(D3DRenderEnv* pRenderEnv, u32 numSpotLights, SpotLight** ppSpotLights);

	~LightBuffer();

	void RecordDataForUpload(D3DCommandList* pCommandList);
	void RemoveDataForUpload();

	u32 GetNumLights() const { return m_NumLights; }

	D3DBuffer* GetLightBoundsBuffer() { return m_pLightBoundsBuffer; }
	D3DBuffer* GetLightPropsBuffer() { return m_pLightPropsBuffer; }
	D3DBuffer* GetLightFrustumBuffer() { return m_pLightFrustumBuffer; }
	D3DBuffer* GetLightViewProjMatrixBuffer() { return m_pLightViewProjMatrixBuffer; }

private:
	u32 m_NumLights;
	
	D3DBuffer* m_pUploadLightBoundsBuffer;
	D3DBuffer* m_pUploadLightPropsBuffer;
	D3DBuffer* m_pUploadLightFrustumBuffer;
	D3DBuffer* m_pUploadLightViewProjMatrixBuffer;

	D3DBuffer* m_pLightBoundsBuffer;
	D3DBuffer* m_pLightPropsBuffer;
	D3DBuffer* m_pLightFrustumBuffer;
	D3DBuffer* m_pLightViewProjMatrixBuffer;
};