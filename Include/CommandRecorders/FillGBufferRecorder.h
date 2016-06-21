#pragma once

#include "Math/Vector4.h"

class DXRootSignature;
class DXPipelineState;
class DXCommandSignature;
class DXCommandList;
class DXCommandAllocator;
class DXBuffer;
class MeshBatch;

struct DXViewport;
struct DXRenderEnvironment;
struct DXBindingResourceList;

class FillGBufferRecorder
{
public:
	struct InitParams
	{
		DXRenderEnvironment* m_pRenderEnv;
		DXGI_FORMAT m_NormalRTVFormat;
		DXGI_FORMAT m_DiffuseRTVFormat;
		DXGI_FORMAT m_SpecularRTVFormat;
		DXGI_FORMAT m_DSVFormat;
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
		DXBuffer* m_pDrawCommandBuffer;
		DXBuffer* m_pNumCommandsBuffer;
	};

	FillGBufferRecorder(InitParams* pParams);
	~FillGBufferRecorder();

	void Record(RenderPassParams* pParams);

private:
	DXRootSignature* m_pRootSignature;
	DXPipelineState* m_pPipelineState;
	DXCommandSignature* m_pCommandSignature;
};
