#include "RenderPasses/ScaleTexturePass.h"
#include "D3DWrapper/RootSignature.h"
#include "D3DWrapper/PipelineState.h"

ScaleTexturePass::ScaleTexturePass(GraphicsDevice* pDevice, ScaleFactor scaleFactor)
	: m_pRootSignature(nullptr)
	, m_pPipelineState(nullptr)
{
}

ScaleTexturePass::~ScaleTexturePass()
{
	SafeDelete(m_pRootSignature);
	SafeDelete(m_pPipelineState);
}