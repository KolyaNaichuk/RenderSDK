#pragma once

#include "D3DWrapper/GraphicsResource.h"
#include "Scene/Light.h"

struct RenderEnv;
class CommandList;
class CommandSignature;
class RootSignature;
class PipelineState;

class CreateTiledShadowMapSATPass
{
public:
	struct ResourceStates
	{
		D3D12_RESOURCE_STATES m_TiledVarianceShadowMapState;
		D3D12_RESOURCE_STATES m_TiledVarianceShadowMapSATState;
	};

	struct InitParams
	{
		const char* m_pName;
		RenderEnv* m_pRenderEnv;
		u32 m_RenderLatency;
		ResourceStates m_InputResourceStates;
		ColorTexture* m_pTiledShadowMap;
		LightType m_LightType;
	};

	struct RenderParams
	{
		RenderEnv* m_pRenderEnv;
		u32 m_CurrentFrameIndex;
		CommandList* m_pCommandList;
		void** m_ppLightsData;
		u32 m_NumLights;
	};

	CreateTiledShadowMapSATPass(InitParams* pParams);
	~CreateTiledShadowMapSATPass();

	void Record(RenderParams* pParams);
	const ResourceStates* GetOutputResourceStates() const { return &m_OutputResourceStates; }
	const ColorTexture* GetTiledShadowMapSAT() { return m_pTiledShadowMapSAT; }

private:
	void InitResources(InitParams* pParams);
	void InitRootSignature(InitParams* pParams);
	void InitPipelineStates(InitParams* pParams);

private:
	const std::string m_Name;
	const LightType m_LightType;

	RootSignature* m_pRootSignature = nullptr;
	PipelineState* m_pPipelineState = nullptr;
	ResourceStates m_OutputResourceStates;
	
	std::vector<Buffer*> m_UploadTileOffsetBuffers;
	std::vector<void*> m_UploadTileOffsetBuffersMem;
	
	DescriptorHandle m_SRVHeapStart;
	std::vector<ResourceTransitionBarrier> m_ResourceBarriers;
	ColorTexture* m_pTiledShadowMapSAT = nullptr;
};