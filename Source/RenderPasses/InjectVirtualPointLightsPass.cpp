#include "RenderPasses/InjectVirtualPointLightsPass.h"
#include "D3DWrapper/CommandList.h"
#include "D3DWrapper/PipelineState.h"
#include "D3DWrapper/RootSignature.h"
#include "D3DWrapper/DescriptorHeap.h"
#include "D3DWrapper/GraphicsResource.h"
#include "Math/Math.h"

InjectVirtualPointLightsPass::InjectVirtualPointLightsPass(InitPrams* pParams)
	: m_pRootSignature(nullptr)
	, m_pPipelineState(nullptr)
{
	const u16 numThreadsPerGroupX = 8;
	const u16 numThreadsPerGroupY = 8;
	const u16 numThreadsPerGroupZ = 8;

	m_NumThreadGroupsX = (u16)Ceil((f32)pParams->m_NumGridCellsX / (f32)numThreadsPerGroupX);
	m_NumThreadGroupsY = (u16)Ceil((f32)pParams->m_NumGridCellsY / (f32)numThreadsPerGroupY);
	m_NumThreadGroupsZ = (u16)Ceil((f32)pParams->m_NumGridCellsZ / (f32)numThreadsPerGroupZ);

	std::string numThreadsPerGroupXStr = std::to_string(numThreadsPerGroupX);
	std::string numThreadsPerGroupYStr = std::to_string(numThreadsPerGroupY);
	std::string numThreadsPerGroupZStr = std::to_string(numThreadsPerGroupZ);
	std::string enablePointLightsStr = std::to_string(pParams->m_EnablePointLights ? 1 : 0);

	const ShaderMacro shaderDefines[] =
	{
		ShaderMacro("NUM_THREADS_X", numThreadsPerGroupXStr.c_str()),
		ShaderMacro("NUM_THREADS_Y", numThreadsPerGroupYStr.c_str()),
		ShaderMacro("NUM_THREADS_Z", numThreadsPerGroupZStr.c_str()),
		ShaderMacro("ENABLE_POINT_LIGHTS", enablePointLightsStr.c_str()),
		ShaderMacro()
	};
	Shader computeShader(L"Shaders//InjectVirtualPointLightsCS.hlsl", "Main", "cs_5_0", shaderDefines);
	assert(false);
}

InjectVirtualPointLightsPass::~InjectVirtualPointLightsPass()
{
	SafeDelete(m_pPipelineState);
	SafeDelete(m_pRootSignature);
}

void InjectVirtualPointLightsPass::Record(RenderParams* pParams)
{
	assert(false);
}
