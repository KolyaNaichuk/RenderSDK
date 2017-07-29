#include "RenderPasses/InjectVirtualPointLightsPass.h"
#include "D3DWrapper/CommandList.h"
#include "D3DWrapper/PipelineState.h"
#include "D3DWrapper/RootSignature.h"
#include "D3DWrapper/DescriptorHeap.h"
#include "D3DWrapper/GraphicsResource.h"
#include "D3DWrapper/RenderEnv.h"
#include "Math/Math.h"

enum RootParams
{
	kSRVRootParam = 0,
	kNumRootParams
};

InjectVirtualPointLightsPass::InjectVirtualPointLightsPass(InitParams* pParams)
	: m_pRootSignature(nullptr)
	, m_pPipelineState(nullptr)
{
	RenderEnv* pRenderEnv = pParams->m_pRenderEnv;

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
	
	std::vector<D3D12_DESCRIPTOR_RANGE> srvDescriptorRanges;
	srvDescriptorRanges.push_back(CBVDescriptorRange(1, 0));
	srvDescriptorRanges.push_back(SRVDescriptorRange(1, 0));

	if (pParams->m_EnablePointLights)
		srvDescriptorRanges.push_back(SRVDescriptorRange(4, 1));
	
	srvDescriptorRanges.push_back(UAVDescriptorRange(6, 0));

	D3D12_ROOT_PARAMETER rootParams[kNumRootParams];
	rootParams[kSRVRootParam] = RootDescriptorTableParameter(srvDescriptorRanges.size(), srvDescriptorRanges.data(), D3D12_SHADER_VISIBILITY_ALL);

	RootSignatureDesc rootSignatureDesc(kNumRootParams, rootParams);
	m_pRootSignature = new RootSignature(pRenderEnv->m_pDevice, &rootSignatureDesc, L"InjectVirtualPointLightsPass::m_pRootSignature");

	ComputePipelineStateDesc pipelineStateDesc;
	pipelineStateDesc.SetRootSignature(m_pRootSignature);
	pipelineStateDesc.SetComputeShader(&computeShader);

	m_pPipelineState = new PipelineState(pRenderEnv->m_pDevice, &pipelineStateDesc, L"InjectVirtualPointLightsPass::m_pPipelineState");
}

InjectVirtualPointLightsPass::~InjectVirtualPointLightsPass()
{
	SafeDelete(m_pPipelineState);
	SafeDelete(m_pRootSignature);
}

void InjectVirtualPointLightsPass::Record(RenderParams* pParams)
{
	assert(false && "Kolya. Fix Me");
	/*
	RenderEnv* pRenderEnv = pParams->m_pRenderEnv;
	CommandList* pCommandList = pParams->m_pCommandList;
	ResourceList* pResources = pParams->m_pResources;

	pCommandList->Begin(m_pPipelineState);
	pCommandList->SetRequiredResourceStates(&pResources->m_RequiredResourceStates);
	pCommandList->SetDescriptorHeaps(pRenderEnv->m_pShaderVisibleSRVHeap);
	pCommandList->SetComputeRootSignature(m_pRootSignature);
	pCommandList->SetComputeRootDescriptorTable(kSRVRootParam, pResources->m_SRVHeapStart);
	pCommandList->Dispatch(m_NumThreadGroupsX, m_NumThreadGroupsY, m_NumThreadGroupsZ);
	pCommandList->End();
	*/
}
