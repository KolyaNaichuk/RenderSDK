#include "CommandRecorders/FrustumCullingRecorder.h"
#include "DX/DXRootSignature.h"
#include "DX/DXPipelineState.h"

FrustumCullingRecorder::FrustumCullingRecorder(InitParams* pParams)
	: m_pRootSignature(nullptr)
	, m_pPipelineState(nullptr)
{
}

FrustumCullingRecorder::~FrustumCullingRecorder()
{
	SafeDelete(m_pPipelineState);
	SafeDelete(m_pRootSignature);
}

void FrustumCullingRecorder::Record(RenderPassParams* pParams)
{
}
