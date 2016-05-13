#pragma once

#include "Common/Common.h"

class DXDevice;
class DXRootSignature;
class DXPipelineState;
class DXCommandList;
class DXCommandAllocator;
class Mesh;

struct DXViewport;
struct DXRenderEnvironment;
struct DXBindingResourceList;

//#define HAS_TEXCOORD

class CreateVoxelGridRecorder
{
public:
	struct InitParams
	{
		DXRenderEnvironment* m_pEnv;
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

	CreateVoxelGridRecorder(InitParams* pParams);
	~CreateVoxelGridRecorder();

	void Record(RenderPassParams* pParams);

private:
	DXRootSignature* m_pRootSignature;
	DXPipelineState* m_pPipelineState;
};