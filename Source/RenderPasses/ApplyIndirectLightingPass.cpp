#include "RenderPasses/ApplyIndirectLightingPass.h"
#include "D3DWrapper/CommandList.h"
#include "D3DWrapper/PipelineState.h"
#include "D3DWrapper/RootSignature.h"
#include "D3DWrapper/DescriptorHeap.h"
#include "D3DWrapper/GraphicsResource.h"
#include "Math/Math.h"

ApplyIndirectLightingPass::ApplyIndirectLightingPass(InitPrams* pParams)
	: m_pRootSignature(nullptr)
	, m_pPipelineState(nullptr)
{
	Shader pixelShader(L"Shaders//ApplyIndirectLightingPS.hlsl", "Main", "ps_4_0");
	assert(false && "Needs impl");
}

ApplyIndirectLightingPass::~ApplyIndirectLightingPass()
{
	SafeDelete(m_pPipelineState);
	SafeDelete(m_pRootSignature);
}

void ApplyIndirectLightingPass::Record(RenderParams* pParams)
{
	assert(false && "Needs impl");
}
