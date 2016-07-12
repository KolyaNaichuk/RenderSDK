#pragma once

#include "Common/Common.h"

class D3DCommandList;
class D3DCommandAllocator;
class D3DRootSignature;
class D3DPipelineState;
class MeshBatch;
struct D3DRenderEnv;
struct D3DResourceList;
struct D3DViewport;

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
		D3DRenderEnv* m_pRenderEnv;
		DXGI_FORMAT m_RTVFormat;
		DXGI_FORMAT m_DSVFormat;
		MeshDataElement m_MeshDataElement;
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
	};

	VisualizeMeshPass(InitParams* pParams);
	~VisualizeMeshPass();

	void Record(RenderParams* pParams);

private:
	D3DRootSignature* m_pRootSignature;
	D3DPipelineState* m_pPipelineState;
};
