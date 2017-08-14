#include "RenderPasses/CreateMainDrawCommandsPass.h"
#include "D3DWrapper/CommandList.h"
#include "D3DWrapper/GraphicsDevice.h"
#include "D3DWrapper/GraphicsUtils.h"
#include "D3DWrapper/PipelineState.h"
#include "D3DWrapper/RenderEnv.h"
#include "D3DWrapper/RootSignature.h"

CreateMainDrawCommandsPass::CreateMainDrawCommandsPass(InitParams* pParams)
	: m_pRootSignature(nullptr)
	, m_pPipelineState(nullptr)
{
	InitResources(pParams);
	InitRootSignature(pParams);
	InitPipelineState(pParams);
}

CreateMainDrawCommandsPass::~CreateMainDrawCommandsPass()
{
	SafeDelete(m_pPipelineState);
	SafeDelete(m_pRootSignature);
}

void CreateMainDrawCommandsPass::Record(RenderParams* pParams)
{
}

void CreateMainDrawCommandsPass::InitResources(InitParams* pParams)
{
}

void CreateMainDrawCommandsPass::InitRootSignature(InitParams* pParams)
{
}

void CreateMainDrawCommandsPass::InitPipelineState(InitParams* pParams)
{
}

void CreateMainDrawCommandsPass::CreateResourceBarrierIfRequired(GraphicsResource * pResource, D3D12_RESOURCE_STATES currState, D3D12_RESOURCE_STATES requiredState)
{
	if (currState != requiredState)
		m_ResourceBarriers.emplace_back(pResource, currState, requiredState);
}
