#pragma once

#include "D3DWrapper/Common.h"

class GraphicsDevice;
class RootSignature;
class PipelineState;
class CommandList;
class CommandSignature;
class Buffer;
class MeshBatch;

struct Viewport;
struct RenderEnv;
struct BindingResourceList;

class CreateVoxelGridPass
{
public:
	struct InitParams
	{
		RenderEnv* m_pRenderEnv;
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

	CreateVoxelGridPass(InitParams* pParams);
	~CreateVoxelGridPass();

	void Record(RenderParams* pParams);

private:
	RootSignature* m_pRootSignature;
	PipelineState* m_pPipelineState;
	CommandSignature* m_pCommandSignature;
};