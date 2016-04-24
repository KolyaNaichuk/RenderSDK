#pragma once

#include "Common/Common.h"

class DXCommandList;
class DXCommandAllocator;
class DXRootSignature;
class DXPipelineState;
class DXColorTexture;
class DXDepthTexture;
class DXBuffer;
struct DXRenderEnvironment;

struct VisualizeVoxelGridInitParams
{
	DXRenderEnvironment* m_pEnv;
	DXGI_FORMAT m_RTVFormat;
};

struct VisualizeVoxelGridRecordParams
{
	DXRenderEnvironment* m_pEnv;
	DXCommandList* m_pCommandList;
	DXCommandAllocator* m_pCommandAllocator;
	DXColorTexture* m_pRenderTarget;
	DXDepthTexture* m_pDepthTexture;
	DXBuffer* m_pGridBuffer;
	DXBuffer* m_pGridConfigBuffer;
	DXBuffer* m_pCameraTransformBuffer;
};

class VisualizeVoxelGridRecorder
{
public:
	VisualizeVoxelGridRecorder(VisualizeVoxelGridInitParams* pParams);
	~VisualizeVoxelGridRecorder();

	void Record(VisualizeVoxelGridRecordParams* pParams);

private:
	DXRootSignature* m_pRootSignature;
	DXPipelineState* m_pPipelineState;
};