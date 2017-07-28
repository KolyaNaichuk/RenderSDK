#pragma once

#include "D3DWrapper/Common.h"

class Buffer;
class CommandList;
class RootSignature;
class PipelineState;
class GraphicsResource;

struct RenderEnv;
struct ResourceList;

class FrustumMeshCullingPass
{
public:
	struct InitParams
	{
		RenderEnv* m_pRenderEnv;
		Buffer* m_pInstanceAABBBuffer;
		Buffer* m_pMeshInstanceRangeBuffer;
		u32 m_TotalNumMeshes;
		u32 m_TotalNumInstances;
		u32 m_MaxNumInstancesPerMesh;
	};

	struct RenderParams
	{
		RenderEnv* m_pRenderEnv;
		CommandList* m_pCommandList;
		Buffer* m_pCullingDataBuffer;
	};

	FrustumMeshCullingPass(InitParams* pParams);
	~FrustumMeshCullingPass();

	void Record(RenderParams* pParams);

private:
	void InitResources(InitParams* pParams);
	void InitRootSignature(InitParams* pParams);
	void InitPipelineState(InitParams* pParams);

private:
	RootSignature* m_pRootSignature;
	PipelineState* m_pPipelineState;	
	ResourceList* m_pResourceList;
	Buffer* m_pNumVisibleMeshesBuffer;
	Buffer* m_pVisibleInstanceRangeBuffer;
	Buffer* m_pVisibleInstanceIndexBuffer;
	Buffer* m_pDrawVisibleInstanceCommandBuffer;
	u32 m_TotalNumMeshes;
};