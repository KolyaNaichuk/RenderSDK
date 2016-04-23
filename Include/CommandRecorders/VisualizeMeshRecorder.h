#pragma once

#include "Common/Mesh.h"

class DXCommandList;
class DXCommandAllocator;
class DXRootSignature;
class DXPipelineState;
class DXRenderTarget;
class DXDepthStencilTexture;
class DXBuffer;
struct DXRenderEnvironment;

enum MeshDataElement
{
	MeshDataElement_Normal = 1,
	MeshDataElement_Color,
	MeshDataElement_TexCoords
};

struct VisualizeMeshInitParams
{
	DXRenderEnvironment* m_pEnv;
	DXGI_FORMAT m_RTVFormat;
	DXGI_FORMAT m_DSVFormat;
	MeshDataElement m_MeshDataElement;
	u8 m_VertexElementFlags;
};

struct VisualizeMeshRecordParams
{
	DXRenderEnvironment* m_pEnv;
	DXCommandList* m_pCommandList;
	DXCommandAllocator* m_pCommandAllocator;
	DXRenderTarget* m_pRenderTarget;
	DXDepthStencilTexture* m_pDepthTexture;
	DXBuffer* m_pTransformBuffer;
	Mesh* m_pMesh;
};

class VisualizeMeshRecorder
{
public:
	VisualizeMeshRecorder(VisualizeMeshInitParams* pParams);
	~VisualizeMeshRecorder();

	void Record(VisualizeMeshRecordParams* pParams);

private:
	DXRootSignature* m_pRootSignature;
	DXPipelineState* m_pPipelineState;
};