#include "RenderPasses/VisualizePropagatedIntensityPass.h"
#include "D3DWrapper/RootSignature.h"
#include "D3DWrapper/PipelineState.h"
#include "D3DWrapper/RenderEnv.h"
#include "D3DWrapper/GraphicsResource.h"
#include "D3DWrapper/CommandList.h"

VisualizePropagatedIntensityPass::VisualizePropagatedIntensityPass(InitParams* pParams)
	: m_pRootSignature(nullptr)
	, m_pPipelineState(nullptr)
{
}

VisualizePropagatedIntensityPass::~VisualizePropagatedIntensityPass()
{
	SafeDelete(m_pPipelineState);
	SafeDelete(m_pRootSignature);
}

void VisualizePropagatedIntensityPass::Record(RenderParams* pParams)
{
}
