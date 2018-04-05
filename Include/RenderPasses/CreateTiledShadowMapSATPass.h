#pragma once

#include "D3DWrapper/GraphicsResource.h"

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
		D3D12_RESOURCE_STATES m_TiledShadowMapState;
		D3D12_RESOURCE_STATES m_TiledShadowMapSATState;
	};

	struct InitParams
	{
		const char* m_pName;
		RenderEnv* m_pRenderEnv;
		ResourceStates m_InputResourceStates;
		ColorTexture* m_pTiledShadowMap;
		u32 m_MinTileSize;
		u32 m_MaxTileSize;
	};

	struct RenderParams
	{
		RenderEnv* m_pRenderEnv;
		CommandList* m_pCommandList;
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
		PipelineStatePermutation(u32 tileSize, PipelineState* pPipelineState)
			: m_TileSize(tileSize)
			, m_pPipelineState(pPipelineState)
		{}
		u32 m_TileSize;
		PipelineState* m_pPipelineState;
	};

	std::string m_Name;
	RootSignature* m_pRootSignature = nullptr;
	std::vector<PipelineStatePermutation> m_PipelineStatePermutations;
	CommandSignature* m_pCommandSignature = nullptr;
	ResourceStates m_OutputResourceStates;

	DescriptorHandle m_SRVHeapStartRow;
	std::vector<ResourceTransitionBarrier> m_ResourceBarriersRow;
	ColorTexture* m_pTiledShadowMapSATRow = nullptr;

	DescriptorHandle m_SRVHeapStartColumn;
	std::vector<ResourceTransitionBarrier> m_ResourceBarriersColumn;
	ColorTexture* m_pTiledShadowMapSATColumn = nullptr;

	Buffer* m_pArgumentBuffer = nullptr;
	Buffer* m_pUploadArgumentBuffer = nullptr;
	void* m_pUploadArgumentBufferMem = nullptr;
};