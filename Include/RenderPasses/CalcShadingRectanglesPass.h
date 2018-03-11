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
		D3D12_RESOURCE_STATES m_GBuffer3State;
		D3D12_RESOURCE_STATES m_MeshTypePerMaterialIDBufferState;
		D3D12_RESOURCE_STATES m_ShadingRectangleMinPointBufferState;
		D3D12_RESOURCE_STATES m_ShadingRectangleMaxPointBufferState;
	};

	struct InitParams
	{
		const char* m_pName;
		RenderEnv* m_pRenderEnv;
		ResourceStates m_InputResourceStates;
		ColorTexture* m_pGBuffer3;
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
	std::string m_Name;
	RootSignature* m_pRootSignature = nullptr;
	PipelineState* m_pPipelineState = nullptr;
	DescriptorHandle m_SRVHeapStart;
	std::vector<ResourceTransitionBarrier> m_ResourceBarriers;
	ResourceStates m_OutputResourceStates;
	Buffer* m_pShadingRectangleMinPointBuffer = nullptr;
	Buffer* m_pShadingRectangleMaxPointBuffer = nullptr;
	u32 m_NumThreadGroupsX = 0;
	u32 m_NumThreadGroupsY = 0;
};