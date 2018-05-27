#include "RenderPasses/RenderSpotLightShadowMapPass.h"
#include "D3DWrapper/CommandList.h"
#include "D3DWrapper/PipelineState.h"
#include "D3DWrapper/RootSignature.h"
#include "D3DWrapper/RenderEnv.h"

RenderSpotLightShadowMapPass::RenderSpotLightShadowMapPass(InitParams* pParams)
	: m_Name(pParams->m_pName)
{
	InitResources(pParams);
	InitRootSignature(pParams);
	InitPipelineState(pParams);
	InitCommandSignature(pParams);
}

RenderSpotLightShadowMapPass::~RenderSpotLightShadowMapPass()
{
	SafeDelete(m_pPipelineState);
	SafeDelete(m_pRootSignature);
}

void RenderSpotLightShadowMapPass::Record(RenderParams* pParams)
{
	assert(false);
}

void RenderSpotLightShadowMapPass::InitResources(InitParams* pParams)
{
	RenderEnv* pRenderEnv = pParams->m_pRenderEnv;

	assert(pParams->m_InputResourceStates.m_RenderCommandBufferState == D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
	assert(pParams->m_InputResourceStates.m_MeshInstanceIndexBufferState == D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	assert(pParams->m_InputResourceStates.m_MeshInstanceWorldMatrixBufferState == D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	assert(pParams->m_InputResourceStates.m_SpotLightViewProjMatrixBufferState == D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	
	m_OutputResourceStates = pParams->m_InputResourceStates;
	assert(false);
}

void RenderSpotLightShadowMapPass::InitRootSignature(InitParams* pParams)
{
	assert(false);
}

void RenderSpotLightShadowMapPass::InitPipelineState(InitParams* pParams)
{
	assert(false);
}

void RenderSpotLightShadowMapPass::InitCommandSignature(InitParams* pParams)
{
	assert(false);
}
