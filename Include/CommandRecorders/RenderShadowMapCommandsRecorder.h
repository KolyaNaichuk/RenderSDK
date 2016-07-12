#pragma once

#include "Common/Common.h"

class D3DRootSignature;
class D3DPipelineState;
class D3DCommandList;
class D3DCommandAllocator;
class D3DCommandSignature;
class D3DBuffer;
struct D3DRenderEnv;
struct D3DResourceList;

class RenderShadowMapCommandsRecorder
{
public:
	struct InitParams
	{
		D3DRenderEnv* m_pRenderEnv;
		bool m_EnablePointLights;
		u32 m_MaxNumPointLightsPerShadowCaster;
		bool m_EnableSpotLights;
		u32 m_MaxNumSpotLightsPerShadowCaster;
	};

	struct RenderPassParams
	{
		D3DRenderEnv* m_pRenderEnv;
		D3DCommandList* m_pCommandList;
		D3DCommandAllocator* m_pCommandAllocator;
		D3DResourceList* m_pResources;
		D3DBuffer* m_pIndirectArgumentBuffer;
		D3DBuffer* m_pNumShadowCastingPointLightsBuffer;
		D3DBuffer* m_pNumDrawPointLightShadowCastersBuffer;
		D3DBuffer* m_pNumShadowCastingSpotLightsBuffer;
		D3DBuffer* m_pNumDrawSpotLightShadowCastersBuffer;
	};

	RenderShadowMapCommandsRecorder(InitParams* pParams);
	~RenderShadowMapCommandsRecorder();

	void Record(RenderPassParams* pParams);

private:
	D3DRootSignature* m_pRootSignature;
	D3DPipelineState* m_pPipelineState;
	D3DCommandSignature* m_pCommandSignature;

	bool m_EnablePointLights;
	bool m_EnableSpotLights;
};
