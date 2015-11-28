#include "CommandRecorders/ScaleTextureRecorder.h"
#include "DX/DXRootSignature.h"
#include "DX/DXPipelineState.h"

ScaleTextureRecorder::ScaleTextureRecorder(DXDevice* pDevice, ScaleFactor scaleFactor)
	: m_pRootSignature(nullptr)
	, m_pPipelineState(nullptr)
{
}

ScaleTextureRecorder::~ScaleTextureRecorder()
{
	delete m_pRootSignature;
	delete m_pPipelineState;
}