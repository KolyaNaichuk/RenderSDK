#pragma once

#include "D3DWrapper/GraphicsResource.h"

struct RenderEnv;
struct Viewport;
struct Vector3f;
class CommandList;
class RootSignature;
class StateObject;
class VertexData;
class IndexData;

class RayTracingPass
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
		VertexData* m_pVertexData = nullptr;
		IndexData* m_pIndexData = nullptr;
		Buffer* m_pAppDataBuffer = nullptr;
		u32 m_NumRaysX = 0;
		u32 m_NumRaysY = 0;
	};

	struct RenderParams
	{
		RenderEnv* m_pRenderEnv = nullptr;
		CommandList* m_pCommandList = nullptr;
		u32 m_NumRaysX = 0;
		u32 m_NumRaysY = 0;
	};

	RayTracingPass(InitParams* pParams);
	~RayTracingPass();

	void Record(RenderParams* pParams);
	const ResourceStates* GetOutputResourceStates() const { return &m_OutputResourceStates; }
	ColorTexture* GetRayTracedResult() { return m_pRayTracedResult; }

private:
	void InitTextures(InitParams* pParams);
	void InitGeometryBuffers(InitParams* pParams);
	void InitAccelerationStructures(InitParams* pParams);
	void InitRootSignatures(InitParams* pParams);
	void InitStateObject(InitParams* pParams);
	void InitDescriptorHeap(InitParams* pParams);
	void InitShaderTables(InitParams* pParams);
	
private:
	DescriptorHandle m_SRVHeapStart;
	std::vector<ResourceTransitionBarrier> m_ResourceBarriers;
	ResourceStates m_OutputResourceStates;

	ColorTexture* m_pRayTracedResult = nullptr;
	Buffer* m_pVertexBuffer = nullptr;
	Buffer* m_pIndexBuffer = nullptr;
	Buffer* m_pInstanceBuffer = nullptr;
	Buffer* m_pBLASBuffer = nullptr;
	Buffer* m_pTLASBuffer = nullptr;
	Buffer* m_pRayGenShaderTable = nullptr;
	Buffer* m_pMissShaderTable = nullptr;
	Buffer* m_pHitGroupTable = nullptr;
	RootSignature* m_pRayGenLocalRootSignature = nullptr;
	RootSignature* m_pEmptyLocalRootSignature = nullptr;
	StateObject* m_pStateObject = nullptr;
};