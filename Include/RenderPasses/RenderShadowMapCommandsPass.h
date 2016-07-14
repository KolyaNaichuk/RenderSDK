#pragma once

#include "D3DWrapper/Common.h"

class RootSignature;
class PipelineState;
class CommandList;
class CommandAllocator;
class CommandSignature;
class Buffer;
struct RenderEnv;
struct BindingResourceList;

class RenderShadowMapCommandsPass
{
public:
	struct InitParams
	{
		RenderEnv* m_pRenderEnv;
		bool m_EnablePointLights;
		u32 m_MaxNumPointLightsPerShadowCaster;
		bool m_EnableSpotLights;
		u32 m_MaxNumSpotLightsPerShadowCaster;
	};

	struct RenderParams
	{
		RenderEnv* m_pRenderEnv;
		CommandList* m_pCommandList;
		CommandAllocator* m_pCommandAllocator;
		BindingResourceList* m_pResources;
		Buffer* m_pIndirectArgumentBuffer;
		Buffer* m_pNumShadowCastingPointLightsBuffer;
		Buffer* m_pNumDrawPointLightShadowCastersBuffer;
		Buffer* m_pNumShadowCastingSpotLightsBuffer;
		Buffer* m_pNumDrawSpotLightShadowCastersBuffer;
	};

	RenderShadowMapCommandsPass(InitParams* pParams);
	~RenderShadowMapCommandsPass();

	void Record(RenderParams* pParams);

private:
	RootSignature* m_pRootSignature;
	PipelineState* m_pPipelineState;
	CommandSignature* m_pCommandSignature;

	bool m_EnablePointLights;
	bool m_EnableSpotLights;
};
