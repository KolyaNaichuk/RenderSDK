#include "CommandRecorders/RenderTiledShadowMapRecorder.h"
#include "Common/MeshBatch.h"
#include "DX/DXPipelineState.h"
#include "DX/DXRootSignature.h"
#include "DX/DXCommandSignature.h"
#include "DX/DXCommandList.h"
#include "DX/DXResource.h"
#include "DX/DXUtils.h"
#include "DX/DXRenderEnvironment.h"

RenderTiledShadowMapRecorder::RenderTiledShadowMapRecorder(InitParams* pParams)
	: m_pPipelineState(nullptr)
	, m_pRootSignature(nullptr)
	, m_pCommandSignature(nullptr)
{
}

RenderTiledShadowMapRecorder::~RenderTiledShadowMapRecorder()
{
	SafeDelete(m_pCommandSignature);
	SafeDelete(m_pRootSignature);
	SafeDelete(m_pPipelineState);
}

void RenderTiledShadowMapRecorder::Record(RenderPassParams* pParams)
{
	assert(false);
}