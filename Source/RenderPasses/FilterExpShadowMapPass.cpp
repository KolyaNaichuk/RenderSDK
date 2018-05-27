#include "RenderPasses/FilterExpShadowMapPass.h"

FilterExpShadowMapPass::FilterExpShadowMapPass(InitParams* pParams)
	: m_Name(pParams->m_pName)
{
	InitResources(pParams);
	InitRootSignatures(pParams);
	InitPipelineStates(pParams);
}

FilterExpShadowMapPass::~FilterExpShadowMapPass()
{
	assert(false);
}

void FilterExpShadowMapPass::Record(RenderParams* pParams)
{
	assert(false);
}

void FilterExpShadowMapPass::InitResources(InitParams* pParams)
{
	assert(false);
}

void FilterExpShadowMapPass::InitRootSignatures(InitParams* pParams)
{
	assert(false);
}

void FilterExpShadowMapPass::InitPipelineStates(InitParams* pParams)
{
	assert(false);
}
