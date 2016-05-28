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
	LightBuffer(DXRenderEnvironment* pEnv, u32 numPointLights, PointLight** ppPointLights);
	LightBuffer(DXRenderEnvironment* pEnv, u32 numSpotLights, SpotLight** ppSpotLights);

	~LightBuffer();

	void RecordDataForUpload(DXCommandList* pCommandList);
	void RemoveDataForUpload();

	u32 GetNumLights() const { return m_NumLights; }

	DXBuffer* GetLightGeometryBuffer() { return m_pLightGeometryBuffer; }
	DXBuffer* GetLightPropsBuffer() { return m_pLightPropsBuffer; }

private:
	u32 m_NumLights;
	
	DXBuffer* m_pUploadLightGeometryBuffer;
	DXBuffer* m_pUploadLightPropsBuffer;

	DXBuffer* m_pLightGeometryBuffer;
	DXBuffer* m_pLightPropsBuffer;
};