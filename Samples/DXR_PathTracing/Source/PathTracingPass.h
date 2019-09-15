#pragma once

#include "D3DWrapper/GraphicsResource.h"

struct RenderEnv;
class CommandList;
class RootSignature;
class StateObject;
class MeshBatch;

class PathTracingPass
{
public:
	struct ResourceStates
	{
		D3D12_RESOURCE_STATES m_RayTracedResultState;
	};

	struct InitParams
	{
		RenderEnv* m_pRenderEnv = nullptr;
		ResourceStates m_InputResourceStates;
		MeshBatch* m_pMeshBatch = nullptr;
		u32 m_NumRaysX = 0;
		u32 m_NumRaysY = 0;
	};

	struct RenderParams
	{
		RenderEnv* m_pRenderEnv = nullptr;
		CommandList* m_pCommandList = nullptr;
		Buffer* m_pAppDataBuffer = nullptr;
		u32 m_NumRaysX = 0;
		u32 m_NumRaysY = 0;
	};

	PathTracingPass(InitParams* pParams);
	~PathTracingPass();

	void Record(RenderParams* pParams);
	const ResourceStates* GetOutputResourceStates() const { return &m_OutputResourceStates; }
	ColorTexture* GetPathTracedResult() { return m_pPathTracedResult; }

private:
	void InitTextures(InitParams* pParams);
	void InitGeometryBuffers(InitParams* pParams);
	void InitAccelerationStructures(InitParams* pParams);
	void InitRootSignatures(InitParams* pParams);
	void InitStateObject(InitParams* pParams);
	void InitDescriptorHeap(InitParams* pParams);
	void InitShaderTables(InitParams* pParams);
	
private:
	DescriptorHandle m_GlobalSRVHeapStart;
	std::vector<ResourceTransitionBarrier> m_ResourceBarriers;
	ResourceStates m_OutputResourceStates;

	ColorTexture* m_pPathTracedResult = nullptr;
	Buffer* m_pVertexBuffer = nullptr;
	Buffer* m_pIndexBuffer = nullptr;
	Buffer* m_pInstanceBuffer = nullptr;
	Buffer* m_pBLASBuffer = nullptr;
	Buffer* m_pTLASBuffer = nullptr;
	Buffer* m_pRayGenShaderTable = nullptr;
	Buffer* m_pMissShaderTable = nullptr;
	Buffer* m_pHitGroupTable = nullptr;
	RootSignature* m_pGlobalRootSignature = nullptr;
	RootSignature* m_pEmptyLocalRootSignature = nullptr;
	StateObject* m_pStateObject = nullptr;
};