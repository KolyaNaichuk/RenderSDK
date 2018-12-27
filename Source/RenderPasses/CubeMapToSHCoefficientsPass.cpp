#include "RenderPasses/CubeMapToSHCoefficientsPass.h"
#include "D3DWrapper/GraphicsResource.h"
#include "D3DWrapper/PipelineState.h"
#include "D3DWrapper/RenderEnv.h"
#include "D3DWrapper/RootSignature.h"
#include "Profiler/GPUProfiler.h"

namespace
{
	enum IntegrateRootParams
	{
		kIntegrateRootSRVTableParam = 0,
		kIntegrateNumRootParams
	};
	
	enum MergeRootParams
	{
		kMergeRootSRVTableParam = 0,
		kMergeNumRootParams
	};
}

CubeMapToSHCoefficientsPass::CubeMapToSHCoefficientsPass(InitParams* pParams)
	: m_Name(pParams->m_pName)
{
	InitResources(pParams);
	
	InitIntegrateRootSignature(pParams);
	InitIntegratePipelineState(pParams);

	InitMergeRootSignature(pParams);
	InitMergePipelineState(pParams);
}

CubeMapToSHCoefficientsPass::~CubeMapToSHCoefficientsPass()
{
	for (u32 SHIndex = 0; SHIndex < kNumSHCoefficients; ++SHIndex)
		SafeDelete(m_IntegratePipelineStates[SHIndex]);
	SafeDelete(m_pIntegrateRootSignature);

	SafeDelete(m_pMergePipelineState);
	SafeDelete(m_pMergeRootSignature);

	SafeDelete(m_pSumPerRowBuffer);
}

void CubeMapToSHCoefficientsPass::Record(RenderParams* pParams)
{
	assert(pParams->m_pCubeMap->GetDepthOrArraySize() == kNumCubeMapFaces);
	assert(m_CubeMapFaceSize == pParams->m_pCubeMap->GetWidth());
	assert(m_CubeMapFaceSize == pParams->m_pCubeMap->GetHeight());

	RenderEnv* pRenderEnv = pParams->m_pRenderEnv;
	CommandList* pCommandList = pParams->m_pCommandList;
	GPUProfiler* pGPUProfiler = pRenderEnv->m_pGPUProfiler;

	pCommandList->Begin();
#ifdef ENABLE_PROFILING
	u32 profileIndex = pGPUProfiler->StartProfile(pCommandList, m_Name.c_str());
#endif // ENABLE_PROFILING

	pCommandList->SetDescriptorHeaps(pRenderEnv->m_pShaderVisibleSRVHeap);
	
	{
		pCommandList->SetComputeRootSignature(m_pIntegrateRootSignature);

		assert(false && "pCommandList->ResourceBarrier");
		assert(false && "pCommandList->SetComputeRootDescriptorTable");
		
		for (u32 SHIndex = 0; SHIndex < kNumSHCoefficients; ++SHIndex)
		{
			pCommandList->SetPipelineState(m_IntegratePipelineStates[SHIndex]);
			pCommandList->Dispatch(1, m_CubeMapFaceSize, kNumCubeMapFaces);
		}
	}
	{
		ResourceTransitionBarrier resourceBarrier(m_pSumPerRowBuffer,
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
			D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

		pCommandList->SetComputeRootSignature(m_pMergeRootSignature);
		pCommandList->SetPipelineState(m_pMergePipelineState);
		pCommandList->ResourceBarrier(1, &resourceBarrier);
		assert(false && "pCommandList->SetComputeRootDescriptorTable");

		pCommandList->Dispatch(kNumSHCoefficients, 1, 1);
	}

#ifdef ENABLE_PROFILING
	pGPUProfiler->EndProfile(pCommandList, profileIndex);
#endif // ENABLE_PROFILING
	pCommandList->End();
}

void CubeMapToSHCoefficientsPass::InitResources(InitParams* pParams)
{
	RenderEnv* pRenderEnv = pParams->m_pRenderEnv;

	assert(pParams->m_CubeMapFaceSize > 0);
	m_CubeMapFaceSize = pParams->m_CubeMapFaceSize;

	assert(m_pSumPerRowBuffer == nullptr);
	FormattedBufferDesc sumPerRowBufferDesc(kNumSHCoefficients * kNumCubeMapFaces * m_CubeMapFaceSize, DXGI_FORMAT_R32G32B32_FLOAT, true, true);
	m_pSumPerRowBuffer = new Buffer(pRenderEnv, pRenderEnv->m_pDefaultHeapProps, &sumPerRowBufferDesc,
		D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, L"CubeMapToSHCoefficientsPass::m_pSumPerRowBuffer");
}

void CubeMapToSHCoefficientsPass::InitIntegrateRootSignature(InitParams* pParams)
{
	assert(m_pIntegrateRootSignature == nullptr);
	D3D12_ROOT_PARAMETER rootParams[kIntegrateNumRootParams];

	D3D12_DESCRIPTOR_RANGE descriptorRanges[] = {SRVDescriptorRange(1, 0), UAVDescriptorRange(1, 0)};
	rootParams[kIntegrateRootSRVTableParam] = RootDescriptorTableParameter(ARRAYSIZE(descriptorRanges), descriptorRanges, D3D12_SHADER_VISIBILITY_ALL);

	RootSignatureDesc rootSignatureDesc(kIntegrateNumRootParams, rootParams);
	m_pIntegrateRootSignature = new RootSignature(pParams->m_pRenderEnv->m_pDevice, &rootSignatureDesc, L"CubeMapToSHCoefficientsPass::m_pRootSignature");
}

void CubeMapToSHCoefficientsPass::InitIntegratePipelineState(InitParams* pParams)
{
	assert(m_pIntegrateRootSignature != nullptr);
	RenderEnv* pRenderEnv = pParams->m_pRenderEnv;

	for (u32 SHIndex = 0; SHIndex < kNumSHCoefficients; ++SHIndex)
	{
		assert(m_IntegratePipelineStates[SHIndex] == nullptr);

		const std::string faceSizeStr = std::to_string(m_CubeMapFaceSize);
		const std::string SHIndexStr = std::to_string(SHIndex);
		
		const ShaderMacro shaderDefines[] =
		{
			ShaderMacro("INTEGRATE", "1"),
			ShaderMacro("FACE_SIZE", faceSizeStr.c_str()),
			ShaderMacro("SH_INDEX", SHIndexStr.c_str()),
			ShaderMacro()
		};
		Shader computeShader(L"Shaders//CubeMapToSHCoefficientsCS.hlsl", "Main", "cs_5_0", shaderDefines);

		ComputePipelineStateDesc pipelineStateDesc;
		pipelineStateDesc.SetRootSignature(m_pIntegrateRootSignature);
		pipelineStateDesc.SetComputeShader(&computeShader);

		m_IntegratePipelineStates[SHIndex] = new PipelineState(pRenderEnv->m_pDevice, &pipelineStateDesc, L"CubeMapToSHCoefficientsPass::m_pIntegratePipelineState");
	}
}

void CubeMapToSHCoefficientsPass::InitMergeRootSignature(InitParams* pParams)
{
	assert(m_pMergeRootSignature == nullptr);
	D3D12_ROOT_PARAMETER rootParams[kMergeNumRootParams];
		
	D3D12_DESCRIPTOR_RANGE descriptorRanges[] = {SRVDescriptorRange(1, 0), UAVDescriptorRange(1, 0)};
	rootParams[kMergeRootSRVTableParam] = RootDescriptorTableParameter(ARRAYSIZE(descriptorRanges), descriptorRanges, D3D12_SHADER_VISIBILITY_ALL);

	RootSignatureDesc rootSignatureDesc(kMergeNumRootParams, rootParams);
	m_pMergeRootSignature = new RootSignature(pParams->m_pRenderEnv->m_pDevice, &rootSignatureDesc, L"CubeMapToSHCoefficientsPass::m_pMergeRootSignature");
}

void CubeMapToSHCoefficientsPass::InitMergePipelineState(InitParams* pParams)
{
	assert(m_pMergePipelineState == nullptr);
	assert(m_pMergeRootSignature != nullptr);
	RenderEnv* pRenderEnv = pParams->m_pRenderEnv;

	const std::string faceSizeStr = std::to_string(m_CubeMapFaceSize);
	const ShaderMacro shaderDefines[] =
	{
		ShaderMacro("MERGE", "1"),
		ShaderMacro("FACE_SIZE", faceSizeStr.c_str()),
		ShaderMacro()
	};
	Shader computeShader(L"Shaders//CubeMapToSHCoefficientsCS.hlsl", "Main", "cs_5_0", shaderDefines);

	ComputePipelineStateDesc pipelineStateDesc;
	pipelineStateDesc.SetRootSignature(m_pMergeRootSignature);
	pipelineStateDesc.SetComputeShader(&computeShader);

	m_pMergePipelineState = new PipelineState(pRenderEnv->m_pDevice, &pipelineStateDesc, L"CubeMapToSHCoefficientsPass::m_pMergePipelineState");
}
