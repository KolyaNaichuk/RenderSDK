#pragma once

#include "D3DWrapper/GraphicsResource.h"

struct RenderEnv;
class RootSignature;
class PipelineState;
class CommandList;

class CalcShadingRectanglesPass
{
public:
	struct ResourceStates
	{
		D3D12_RESOURCE_STATES m_MaterialIDTextureState;
		D3D12_RESOURCE_STATES m_MeshTypePerMaterialIDBufferState;
		D3D12_RESOURCE_STATES m_ShadingRectangleMinPointBufferState;
		D3D12_RESOURCE_STATES m_ShadingRectangleMaxPointBufferState;
	};

	struct InitParams
	{
		RenderEnv* m_pRenderEnv;
		ResourceStates m_InputResourceStates;
		ColorTexture* m_pMaterialIDTexture;
		Buffer* m_pMeshTypePerMaterialIDBuffer;
		u32 m_NumMeshTypes;
	};

	struct RenderParams
	{
		RenderEnv* m_pRenderEnv;
		CommandList* m_pCommandList;
		Buffer* m_pAppDataBuffer;
	};

	CalcShadingRectanglesPass(InitParams* pParams);
	~CalcShadingRectanglesPass();

	void Record(RenderParams* pParams);
	const ResourceStates* GetOutputResourceStates() const { return &m_OutputResourceStates; }
	Buffer* GetShadingRectangleMinPointBuffer() { return m_pShadingRectangleMinPointBuffer; }
	Buffer* GetShadingRectangleMaxPointBuffer() { return m_pShadingRectangleMaxPointBuffer; }

private:
	void InitResources(InitParams* pParams);
	void InitRootSignature(InitParams* pParams);
	void InitPipelineState(InitParams* pParams);
	void AddResourceBarrierIfRequired(GraphicsResource* pResource, D3D12_RESOURCE_STATES currState, D3D12_RESOURCE_STATES requiredState);

private:
	RootSignature* m_pRootSignature;
	PipelineState* m_pPipelineState;
	DescriptorHandle m_SRVHeapStart;
	std::vector<ResourceTransitionBarrier> m_ResourceBarriers;
	ResourceStates m_OutputResourceStates;
	Buffer* m_pShadingRectangleMinPointBuffer;
	Buffer* m_pShadingRectangleMaxPointBuffer;
	u32 m_NumThreadGroupsX;
	u32 m_NumThreadGroupsY;
};