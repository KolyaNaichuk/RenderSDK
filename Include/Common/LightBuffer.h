#pragma once

#include "Common/Common.h"

class DXCommandList;
class DXBuffer;
struct DXRenderEnvironment;

class PointLight;
class SpotLight;

class LightBuffer
{
public:
	LightBuffer(DXRenderEnvironment* pRenderEnv, u32 numPointLights, PointLight** ppPointLights);
	LightBuffer(DXRenderEnvironment* pRenderEnv, u32 numSpotLights, SpotLight** ppSpotLights);

	~LightBuffer();

	void RecordDataForUpload(DXCommandList* pCommandList);
	void RemoveDataForUpload();

	u32 GetNumLights() const { return m_NumLights; }

	DXBuffer* GetLightBoundsBuffer() { return m_pLightBoundsBuffer; }
	DXBuffer* GetLightPropsBuffer() { return m_pLightPropsBuffer; }
	DXBuffer* GetLightFrustumBuffer() { return m_pLightFrustumBuffer; }

private:
	u32 m_NumLights;
	
	DXBuffer* m_pUploadLightBoundsBuffer;
	DXBuffer* m_pUploadLightPropsBuffer;
	DXBuffer* m_pUploadLightFrustumBuffer;

	DXBuffer* m_pLightBoundsBuffer;
	DXBuffer* m_pLightPropsBuffer;
	DXBuffer* m_pLightFrustumBuffer;
};