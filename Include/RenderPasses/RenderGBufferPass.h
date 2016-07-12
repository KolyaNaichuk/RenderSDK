#pragma once

#include "Math/Vector4.h"

class D3DRootSignature;
class D3DPipelineState;
class D3DCommandSignature;
class D3DCommandList;
class D3DCommandAllocator;
class D3DBuffer;
class MeshBatch;

struct D3DViewport;
struct D3DRenderEnv;
struct D3DResourceList;

class RenderGBufferPass
{
public:
	struct InitParams
	{
		D3DRenderEnv* m_pRenderEnv;
		DXGI_FORMAT m_NormalRTVFormat;
		DXGI_FORMAT m_DiffuseRTVFormat;
		DXGI_FORMAT m_SpecularRTVFormat;
		DXGI_FORMAT m_DSVFormat;
		MeshBatch* m_pMeshBatch;
	};
	struct RenderParams
	{
		D3DRenderEnv* m_pRenderEnv;
		D3DCommandList* m_pCommandList;
		D3DCommandAllocator* m_pCommandAllocator;
		D3DResourceList* m_pResources;
		D3DViewport* m_pViewport;
		MeshBatch* m_pMeshBatch;
		D3DBuffer* m_pDrawMeshCommandBuffer;
		D3DBuffer* m_pNumDrawMeshesBuffer;
	};

	RenderGBufferPass(InitParams* pParams);
	~RenderGBufferPass();

	void Record(RenderParams* pParams);

private:
	D3DRootSignature* m_pRootSignature;
	D3DPipelineState* m_pPipelineState;
	D3DCommandSignature* m_pCommandSignature;
};
