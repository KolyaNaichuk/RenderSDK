#pragma once

#include "Common/Mesh.h"

class DXCommandList;
class DXCommandAllocator;
class DXRootSignature;
class DXPipelineState;
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
		DXRenderEnvironment* m_pEnv;
		DXGI_FORMAT m_RTVFormat;
		DXGI_FORMAT m_DSVFormat;
		MeshDataElement m_MeshDataElement;
		u8 m_VertexElementFlags;
	};
	struct RenderPassParams
	{
		DXRenderEnvironment* m_pEnv;
		DXCommandList* m_pCommandList;
		DXCommandAllocator* m_pCommandAllocator;
		DXBindingResourceList* m_pResources;
		DXViewport* m_pViewport;
		Mesh* m_pMesh;
	};

	VisualizeMeshRecorder(InitParams* pParams);
	~VisualizeMeshRecorder();

	void Record(RenderPassParams* pParams);

private:
	DXRootSignature* m_pRootSignature;
	DXPipelineState* m_pPipelineState;
};
