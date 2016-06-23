#pragma once

#include "Common/Common.h"

class DXRootSignature;
class DXPipelineState;
class DXCommandList;
class DXCommandAllocator;
class DXCommandSignature;
class DXBuffer;
struct DXRenderEnvironment;
struct DXBindingResourceList;

class CreateRenderShadowMapCommandsRecorder
{
public:
	struct InitParams
	{
		DXRenderEnvironment* m_pRenderEnv;
		bool m_EnablePointLights;
		u32 m_MaxNumPointLightsPerShadowCaster;
		bool m_EnableSpotLights;
		u32 m_MaxNumSpotLightsPerShadowCaster;
	};

	struct RenderPassParams
	{
		DXRenderEnvironment* m_pRenderEnv;
		DXCommandList* m_pCommandList;
		DXCommandAllocator* m_pCommandAllocator;
		DXBindingResourceList* m_pResources;
		DXBuffer* m_pNumMeshes;
		DXBuffer* m_pNumShadowCastingPointLightsBuffer;
		DXBuffer* m_pNumDrawPointLightShadowCastersBuffer;
		DXBuffer* m_pNumShadowCastingSpotLightsBuffer;
		DXBuffer* m_pNumDrawSpotLightShadowCastersBuffer;
	};

	CreateRenderShadowMapCommandsRecorder(InitParams* pParams);
	~CreateRenderShadowMapCommandsRecorder();

	void Record(RenderPassParams* pParams);

private:
	DXRootSignature* m_pRootSignature;
	DXPipelineState* m_pPipelineState;
	DXCommandSignature* m_pCommandSignature;

	bool m_EnablePointLights;
	bool m_EnableSpotLights;
};
