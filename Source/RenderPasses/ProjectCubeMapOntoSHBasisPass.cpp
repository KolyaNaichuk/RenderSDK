#include "RenderPasses/ProjectCubeMapOntoSHBasisPass.h"
#include "D3DWrapper/PipelineState.h"
#include "D3DWrapper/GraphicsResource.h"

ProjectCubeMapOntoSHBasisPass::ProjectCubeMapOntoSHBasisPass(InitParams* pParams)
	: m_Name(pParams)
{
	InitResources(pParams);
	InitRootSignature(pParams);
	InitPipelineState(pParams);
}

ProjectCubeMapOntoSHBasisPass::~ProjectCubeMapOntoSHBasisPass()
{
	SafeDelete(m_pPipelineState);
	SafeDelete(m_pRootSignature);
}

void ProjectCubeMapOntoSHBasisPass::Record(RenderParams* pParams)
{
	assert(false);
}

void ProjectCubeMapOntoSHBasisPass::InitResources(InitParams* pParams)
{
	assert(false);
}

void ProjectCubeMapOntoSHBasisPass::InitRootSignature(InitParams* pParams)
{
	assert(m_pRootSignature == nullptr);
	assert(false);
}

void ProjectCubeMapOntoSHBasisPass::InitPipelineState(InitParams* pParams)
{
	assert(m_pPipelineState == nullptr);
	assert(m_pRootSignature != nullptr);
	assert(false);
}
