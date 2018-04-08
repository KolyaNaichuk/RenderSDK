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
		ResourceStates m_InputResourceStates;
		ColorTexture* m_pTiledShadowMap;
		u32 m_MinTileSize;
		u32 m_MaxTileSize;
		LightType m_LightType;
		u32 m_MaxNumLights;
	};

	struct RenderParams
	{
		RenderEnv* m_pRenderEnv;
		CommandList* m_pCommandList;
		void* m_pLightData;
	};

	CreateTiledShadowMapSATPass(InitParams* pParams);
	~CreateTiledShadowMapSATPass();

	void Record(RenderParams* pParams);
	const ResourceStates* GetOutputResourceStates() const { return &m_OutputResourceStates; }
	const ColorTexture* GetTiledShadowMapSAT() { return m_pTiledShadowMapSATColumn; }

private:
	void InitResources(InitParams* pParams);
	void InitRootSignature(InitParams* pParams);
	void InitPipelineStates(InitParams* pParams);
	void InitCommandSignature(InitParams* pParams);

private:
	struct PipelineStatePermutation
	{
		u32 m_TileSize;
		PipelineState* m_pPipelineState;
		u32 m_NumThreadGroupsX;
		u32 m_NumThreadGroupsY;
	};

	struct ExecuteIndirectParams
	{
		PipelineState* m_pPipelineState;
		UINT m_NumCommands;
		UINT64 m_FirstCommandOffset;
	};

	std::string m_Name;
	RootSignature* m_pRootSignature = nullptr;
	std::vector<PipelineStatePermutation> m_PipelineStatePermutations;
	CommandSignature* m_pCommandSignature = nullptr;
	ResourceStates m_OutputResourceStates;
	
	std::vector<ResourceTransitionBarrier> m_UploadArgumentsResourceBarriers;
	Buffer* m_pArgumentBuffer = nullptr;
	Buffer* m_pUploadArgumentBuffer = nullptr;
	void* m_pUploadArgumentBufferMem = nullptr;

	DescriptorHandle m_SRVHeapStartRow;
	std::vector<ResourceTransitionBarrier> m_ResourceBarriersRow;
	ColorTexture* m_pTiledShadowMapSATRow = nullptr;

	DescriptorHandle m_SRVHeapStartColumn;
	std::vector<ResourceTransitionBarrier> m_ResourceBarriersColumn;
	ColorTexture* m_pTiledShadowMapSATColumn = nullptr;
};