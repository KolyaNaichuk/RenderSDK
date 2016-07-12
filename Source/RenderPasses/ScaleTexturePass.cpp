#include "RenderPasses/ScaleTexturePass.h"
#include "D3DWrapper/D3DRootSignature.h"
#include "D3DWrapper/D3DPipelineState.h"

ScaleTexturePass::ScaleTexturePass(D3DDevice* pDevice, ScaleFactor scaleFactor)
	: m_pRootSignature(nullptr)
	, m_pPipelineState(nullptr)
{
}

ScaleTexturePass::~ScaleTexturePass()
{
	delete m_pRootSignature;
	delete m_pPipelineState;
}