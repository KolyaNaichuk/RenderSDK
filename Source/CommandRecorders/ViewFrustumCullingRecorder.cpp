#include "CommandRecorders/ViewFrustumCullingRecorder.h"
#include "DX/DXRootSignature.h"
#include "DX/DXPipelineState.h"
#include "Math/Math.h"

enum RootParams
{
	kCullingDataCBVRootParam = 0,
	kMeshBoundsBufferSRVRootParam,
	kMeshDataBufferSRVRootParam,
	kCommandBufferUAVRootParam,
	kCommandCountBufferUAVRootParam,
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

	DXCBVRange cullingDataCBVRange(1, 0);
	DXSRVRange meshBoundsBufferSRVRange(1, 0);
	DXSRVRange meshDataBufferSRVRange(1, 1);
	DXUAVRange commandBufferUAVRange(1, 0);
	DXUAVRange commandCountBufferUAVRange(1, 1);

	D3D12_ROOT_PARAMETER rootParams[kNumRootParams];
	rootParams[kCullingDataCBVRootParam] = DXRootDescriptorTableParameter(1, &cullingDataCBVRange, D3D12_SHADER_VISIBILITY_ALL);
	rootParams[kMeshBoundsBufferSRVRootParam] = DXRootDescriptorTableParameter(1, &meshBoundsBufferSRVRange, D3D12_SHADER_VISIBILITY_ALL);
	rootParams[kMeshDataBufferSRVRootParam] = DXRootDescriptorTableParameter(1, &meshDataBufferSRVRange, D3D12_SHADER_VISIBILITY_ALL);
	rootParams[kCommandBufferUAVRootParam] = DXRootDescriptorTableParameter(1, &commandBufferUAVRange, D3D12_SHADER_VISIBILITY_ALL);
	rootParams[kCommandCountBufferUAVRootParam] = DXRootDescriptorTableParameter(1, &commandCountBufferUAVRange, D3D12_SHADER_VISIBILITY_ALL);

	DXRootSignatureDesc rootSignatureDesc(kNumRootParams, rootParams);
	m_pRootSignature = new DXRootSignature(pParams->m_pDevice, &rootSignatureDesc, L"ViewFrustumCullingRecorder::m_pRootSignature");

	DXComputePipelineStateDesc pipelineStateDesc;
	pipelineStateDesc.SetRootSignature(m_pRootSignature);
	pipelineStateDesc.SetComputeShader(&computeShader);

	m_pPipelineState = new DXPipelineState(pParams->m_pDevice, &pipelineStateDesc, L"ViewFrustumCullingRecorder::m_pPipelineState");
}

ViewFrustumCullingRecorder::~ViewFrustumCullingRecorder()
{
	SafeDelete(m_pPipelineState);
	SafeDelete(m_pRootSignature);
}

void ViewFrustumCullingRecorder::Record(RenderPassParams* pParams)
{
}
