#pragma once

#include "Common/Common.h"

class DXCommandList;
class DXCommandAllocator;
class DXRootSignature;
class DXPipelineState;
class MeshBatch;
struct DXRenderEnvironment;
struct DXBindingResourceList;
struct DXViewport;

enum MeshDataElement
{
	MeshDataElement_Normal = 1,
	MeshDataElement_Color,
	MeshDataElement_TexCoords
};

class VisualizeMeshRecorder
{
public:
	struct InitParams
	{
		DXRenderEnvironment* m_pRenderEnv;
		DXGI_FORMAT m_RTVFormat;
		DXGI_FORMAT m_DSVFormat;
		MeshDataElement m_MeshDataElement;
		MeshBatch* m_pMeshBatch;
	};
	struct RenderPassParams
	{
		DXRenderEnvironment* m_pRenderEnv;
		DXCommandList* m_pCommandList;
		DXCommandAllocator* m_pCommandAllocator;
		DXBindingResourceList* m_pResources;
		DXViewport* m_pViewport;
		MeshBatch* m_pMeshBatch;
	};

	VisualizeMeshRecorder(InitParams* pParams);
	~VisualizeMeshRecorder();

	void Record(RenderPassParams* pParams);

private:
	DXRootSignature* m_pRootSignature;
	DXPipelineState* m_pPipelineState;
};
