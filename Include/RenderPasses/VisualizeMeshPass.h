#pragma once

#include "D3DWrapper/Common.h"

class CommandList;
class CommandAllocator;
class RootSignature;
class PipelineState;
class MeshBatch;
struct RenderEnv;
struct BindingResourceList;
struct Viewport;

enum MeshDataElement
{
	MeshDataElement_Normal = 1,
	MeshDataElement_Color,
	MeshDataElement_TexCoords
};

class VisualizeMeshPass
{
public:
	struct InitParams
	{
		RenderEnv* m_pRenderEnv;
		DXGI_FORMAT m_RTVFormat;
		DXGI_FORMAT m_DSVFormat;
		MeshDataElement m_MeshDataElement;
		MeshBatch* m_pMeshBatch;
	};
	struct RenderParams
	{
		RenderEnv* m_pRenderEnv;
		CommandList* m_pCommandList;
		CommandAllocator* m_pCommandAllocator;
		BindingResourceList* m_pResources;
		Viewport* m_pViewport;
		MeshBatch* m_pMeshBatch;
	};

	VisualizeMeshPass(InitParams* pParams);
	~VisualizeMeshPass();

	void Record(RenderParams* pParams);

private:
	RootSignature* m_pRootSignature;
	PipelineState* m_pPipelineState;
};
