#pragma once

#include "Math/Vector4.h"

class DXRootSignature;
class DXPipelineState;
class DXCommandList;
class DXCommandAllocator;
class MeshBatch;
struct DXRenderEnvironment;

class FillGBufferRecorder
{
public:
	struct InitParams
	{
		DXRenderEnvironment* m_pEnv;
		DXGI_FORMAT m_DiffuseRTVFormat;
		DXGI_FORMAT m_NormalRTVFormat;
		DXGI_FORMAT m_SpecularRTVFormat;
		DXGI_FORMAT m_DSVFormat;
		MeshBatch* m_pMeshBatch;
	};
	struct RenderPassParams
	{
		DXRenderEnvironment* m_pEnv;
		DXCommandList* m_pCommandList;
		DXCommandAllocator* m_pCommandAllocator;
		MeshBatch* m_pMeshBatch;
	};

	FillGBufferRecorder(InitParams* pParams);
	~FillGBufferRecorder();

	void Record(RenderPassParams* pParams);

private:
	DXRootSignature* m_pRootSignature;
	DXPipelineState* m_pPipelineState;
};
