#include "CommandRecorders/ViewFrustumCullingRecorder.h"
#include "DX/DXRootSignature.h"
#include "DX/DXPipelineState.h"
#include "DX/DXRenderEnvironment.h"
#include "Math/Math.h"

enum RootParams
{
	kSRVRootParam = 0,
	kNumRootParams
};

ViewFrustumCullingRecorder::ViewFrustumCullingRecorder(InitParams* pParams)
	: m_pRootSignature(nullptr)
	, m_pPipelineState(nullptr)
{
	const u16 threadGroupSize = 64;
	m_NumThreadGroupsX = (u16)Ceil((f32)pParams->m_NumMeshes / (f32)threadGroupSize);

	std::string threadGroupSizeStr = std::to_string(threadGroupSize);
	const DXShaderMacro shaderDefines[] =
	{
		DXShaderMacro("THREAD_GROUP_SIZE", threadGroupSizeStr.c_str()),
		DXShaderMacro()
	};
	DXShader computeShader(L"Shaders//ViewFrustumCullingCS.hlsl", "Main", "cs_5_0", shaderDefines);

	D3D12_DESCRIPTOR_RANGE descriptorRanges[] = {DXCBVRange(1, 0), DXSRVRange(2, 0), DXUAVRange(2, 0)};
	D3D12_ROOT_PARAMETER rootParams[kNumRootParams];
	rootParams[kSRVRootParam] = DXRootDescriptorTableParameter(ARRAYSIZE(descriptorRanges), &descriptorRanges[0], D3D12_SHADER_VISIBILITY_ALL);

	DXRootSignatureDesc rootSignatureDesc(kNumRootParams, rootParams);
	m_pRootSignature = new DXRootSignature(pParams->m_pEnv->m_pDevice, &rootSignatureDesc, L"ViewFrustumCullingRecorder::m_pRootSignature");

	DXComputePipelineStateDesc pipelineStateDesc;
	pipelineStateDesc.SetRootSignature(m_pRootSignature);
	pipelineStateDesc.SetComputeShader(&computeShader);

	m_pPipelineState = new DXPipelineState(pParams->m_pEnv->m_pDevice, &pipelineStateDesc, L"ViewFrustumCullingRecorder::m_pPipelineState");
}

ViewFrustumCullingRecorder::~ViewFrustumCullingRecorder()
{
	SafeDelete(m_pPipelineState);
	SafeDelete(m_pRootSignature);
}

void ViewFrustumCullingRecorder::Record(RenderPassParams* pParams)
{
}
