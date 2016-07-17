#pragma once

#include "D3DWrapper/Common.h"

class RootSignature;
class PipelineState;
class CommandSignature;
class CommandList;
class Buffer;
class MeshBatch;

struct Viewport;
struct RenderEnv;
struct BindingResourceList;

class RenderGBufferPass
{
public:
	struct InitParams
	{
		RenderEnv* m_pRenderEnv;
		DXGI_FORMAT m_NormalRTVFormat;
		DXGI_FORMAT m_DiffuseRTVFormat;
		DXGI_FORMAT m_SpecularRTVFormat;
		DXGI_FORMAT m_DSVFormat;
		MeshBatch* m_pMeshBatch;
	};
	struct RenderParams
	{
		RenderEnv* m_pRenderEnv;
		CommandList* m_pCommandList;
		BindingResourceList* m_pResources;
		Viewport* m_pViewport;
		MeshBatch* m_pMeshBatch;
		Buffer* m_pDrawMeshCommandBuffer;
		Buffer* m_pNumDrawMeshesBuffer;
	};

	RenderGBufferPass(InitParams* pParams);
	~RenderGBufferPass();

	void Record(RenderParams* pParams);

private:
	RootSignature* m_pRootSignature;
	PipelineState* m_pPipelineState;
	CommandSignature* m_pCommandSignature;
};
