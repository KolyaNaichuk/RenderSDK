#include "RenderPasses/CreateExpShadowMapPass.h"
#include "D3DWrapper/CommandList.h"
#include "D3DWrapper/PipelineState.h"
#include "D3DWrapper/RootSignature.h"

CreateExpShadowMapPass::CreateExpShadowMapPass(InitParams* pParams)
	: m_Name(pParams->m_pName)
{
	InitResources(pParams);
	InitRootSignature(pParams);
	InitPipelineState(pParams);
}

CreateExpShadowMapPass::~CreateExpShadowMapPass()
{
	SafeDelete(m_pPipelineState);
	SafeDelete(m_pRootSignature);
}

void CreateExpShadowMapPass::Record(RenderParams* pParams)
{
	assert(false);
}

void CreateExpShadowMapPass::InitResources(InitParams* pParams)
{
	assert(false);
}

void CreateExpShadowMapPass::InitRootSignature(InitParams* pParams)
{
	assert(false);
}

void CreateExpShadowMapPass::InitPipelineState(InitParams* pParams)
{
	assert(false);
}

void CreateExpShadowMapPass::AddResourceBarrierIfRequired(GraphicsResource* pResource, D3D12_RESOURCE_STATES currState, D3D12_RESOURCE_STATES requiredState)
{
	if (currState != requiredState)
		m_ResourceBarriers.emplace_back(pResource, currState, requiredState);
}
