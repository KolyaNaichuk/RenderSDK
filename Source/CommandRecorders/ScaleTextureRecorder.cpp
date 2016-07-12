#include "CommandRecorders/ScaleTextureRecorder.h"
#include "D3DWrapper/D3DRootSignature.h"
#include "D3DWrapper/D3DPipelineState.h"

ScaleTextureRecorder::ScaleTextureRecorder(D3DDevice* pDevice, ScaleFactor scaleFactor)
	: m_pRootSignature(nullptr)
	, m_pPipelineState(nullptr)
{
}

ScaleTextureRecorder::~ScaleTextureRecorder()
{
	delete m_pRootSignature;
	delete m_pPipelineState;
}