#include "RenderPasses/RadianceCubeMapToSHIrradianceCoefficientsPass.h"
#include "D3DWrapper/GraphicsDevice.h"
#include "D3DWrapper/GraphicsResource.h"
#include "D3DWrapper/PipelineState.h"
#include "D3DWrapper/RenderEnv.h"
#include "D3DWrapper/RootSignature.h"
#include "Math/Vector3.h"
#include "Profiler/GPUProfiler.h"

namespace
{
	enum PrecomputeRootParams
	{
		kPrecomputeSRVTableParam = 0,
		kPrecomputeNumRootParams
	};

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

RadianceCubeMapToSHIrradianceCoefficientsPass::RadianceCubeMapToSHIrradianceCoefficientsPass(InitParams* pParams)
{
	InitResources(pParams);

	InitIntegrateRootSignature(pParams);
	InitIntegratePipelineState(pParams);

	InitMergeRootSignature(pParams);
	InitMergePipelineState(pParams);

	PrecomputeWeightedSHMap(pParams);
}

RadianceCubeMapToSHIrradianceCoefficientsPass::~RadianceCubeMapToSHIrradianceCoefficientsPass()
{
	SafeDelete(m_pIntegratePipelineState);
	SafeDelete(m_pIntegrateRootSignature);

	SafeDelete(m_pMergePipelineState);
	SafeDelete(m_pMergeRootSignature);

	SafeDelete(m_pSumPerColumnBuffer);
}

void RadianceCubeMapToSHIrradianceCoefficientsPass::Record(RenderParams* pParams)
{
	assert(pParams->m_pRadianceCubeMap->GetDepthOrArraySize() == kNumCubeMapFaces);
	assert(m_RadianceCubeMapFaceSize == pParams->m_pRadianceCubeMap->GetWidth());
	assert(m_RadianceCubeMapFaceSize == pParams->m_pRadianceCubeMap->GetHeight());
	assert(pParams->m_NumSHCoefficients > 0);
	assert(pParams->m_NumSHCoefficients <= m_MaxNumSHCoefficients);

	RenderEnv* pRenderEnv = pParams->m_pRenderEnv;
	CommandList* pCommandList = pParams->m_pCommandList;
	GPUProfiler* pGPUProfiler = pRenderEnv->m_pGPUProfiler;

	pCommandList->Begin();
	pCommandList->SetDescriptorHeaps(pRenderEnv->m_pShaderVisibleSRVHeap);

	{
		ResourceTransitionBarrier resourceBarriers[2];
		u32 numBarriers = 0;

		resourceBarriers[numBarriers++] = ResourceTransitionBarrier(
			m_pSumPerColumnBuffer,
			D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS
		);

		if (pParams->m_InputResourceStates.m_RadianceCubeMapState != m_OutputResourceStates.m_RadianceCubeMapState)
		{
			resourceBarriers[numBarriers++] = ResourceTransitionBarrier(
				pParams->m_pRadianceCubeMap,
				pParams->m_InputResourceStates.m_RadianceCubeMapState,
				m_OutputResourceStates.m_RadianceCubeMapState
			);
		}

		pRenderEnv->m_pDevice->CopyDescriptor(DescriptorHandle(m_IntegrateSRVHeapStart, 2),
			pParams->m_pRadianceCubeMap->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

#ifdef ENABLE_PROFILING
		u32 profileIndex1 = pGPUProfiler->StartProfile(pCommandList, "RadianceCubeMapToSHIrradianceCoefficientsPass: Integrate");
#endif // ENABLE_PROFILING

		pCommandList->SetComputeRootSignature(m_pIntegrateRootSignature);
		pCommandList->SetPipelineState(m_pIntegratePipelineState);
		pCommandList->ResourceBarrier(numBarriers, resourceBarriers);
		pCommandList->SetComputeRootDescriptorTable(kIntegrateRootSRVTableParam, m_IntegrateSRVHeapStart);
		pCommandList->Dispatch(m_RadianceCubeMapFaceSize / 4, pParams->m_NumSHCoefficients, kNumCubeMapFaces);

#ifdef ENABLE_PROFILING
		pGPUProfiler->EndProfile(pCommandList, profileIndex1);
#endif // ENABLE_PROFILING
	}
	{
		ResourceTransitionBarrier resourceBarriers[2];
		u32 numBarriers = 0;

		resourceBarriers[numBarriers++] = ResourceTransitionBarrier(
			m_pSumPerColumnBuffer,
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
			D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE
		);

		if (pParams->m_InputResourceStates.m_SHIrradianceCoefficientBufferState != m_OutputResourceStates.m_SHIrradianceCoefficientBufferState)
		{
			resourceBarriers[numBarriers++] = ResourceTransitionBarrier(
				pParams->m_pSHIrradianceCoefficientBuffer,
				pParams->m_InputResourceStates.m_SHIrradianceCoefficientBufferState,
				m_OutputResourceStates.m_SHIrradianceCoefficientBufferState
			);
		}

		pRenderEnv->m_pDevice->CopyDescriptor(DescriptorHandle(m_MergeSRVHeapStart, 1),
			pParams->m_pSHIrradianceCoefficientBuffer->GetUAVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

#ifdef ENABLE_PROFILING
		u32 profileIndex2 = pGPUProfiler->StartProfile(pCommandList, "RadianceCubeMapToSHIrradianceCoefficientsPass: Merge");
#endif // ENABLE_PROFILING

		pCommandList->SetComputeRootSignature(m_pMergeRootSignature);
		pCommandList->SetPipelineState(m_pMergePipelineState);
		pCommandList->ResourceBarrier(numBarriers, resourceBarriers);
		pCommandList->SetComputeRootDescriptorTable(kMergeRootSRVTableParam, m_MergeSRVHeapStart);
		pCommandList->Dispatch(1, pParams->m_NumSHCoefficients, 1);

#ifdef ENABLE_PROFILING
		pGPUProfiler->EndProfile(pCommandList, profileIndex2);
#endif // ENABLE_PROFILING
	}

	pCommandList->End();
}

void RadianceCubeMapToSHIrradianceCoefficientsPass::InitResources(InitParams* pParams)
{
	RenderEnv* pRenderEnv = pParams->m_pRenderEnv;

	assert(pParams->m_MaxNumSHCoefficients > 0);
	m_MaxNumSHCoefficients = pParams->m_MaxNumSHCoefficients;

	assert(pParams->m_RadianceCubeMapFaceSize > 0);
	m_RadianceCubeMapFaceSize = pParams->m_RadianceCubeMapFaceSize;

	assert(m_pWeightedSHMap == nullptr);
	ColorTexture2DDesc weightedSHMapDesc(DXGI_FORMAT_R32_FLOAT, m_RadianceCubeMapFaceSize, m_RadianceCubeMapFaceSize,
		false, true, true, 1, m_MaxNumSHCoefficients * kNumCubeMapFaces);
	m_pWeightedSHMap = new ColorTexture(pRenderEnv, pRenderEnv->m_pDefaultHeapProps, &weightedSHMapDesc,
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS, nullptr, L"RadianceCubeMapToSHIrradianceCoefficientsPass::m_pWeightedSHMap");

	assert(m_pSumPerColumnBuffer == nullptr);
	StructuredBufferDesc sumPerColumnBufferDesc(m_MaxNumSHCoefficients * kNumCubeMapFaces * (m_RadianceCubeMapFaceSize / 4), sizeof(Vector3f), true, true);
	m_pSumPerColumnBuffer = new Buffer(pRenderEnv, pRenderEnv->m_pDefaultHeapProps, &sumPerColumnBufferDesc,
		D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, L"RadianceCubeMapToSHIrradianceCoefficientsPass::m_pSumPerColumnBuffer");

	m_OutputResourceStates.m_RadianceCubeMapState = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
	m_OutputResourceStates.m_SHIrradianceCoefficientBufferState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;

	assert(!m_IntegrateSRVHeapStart.IsValid());
	m_IntegrateSRVHeapStart = pRenderEnv->m_pShaderVisibleSRVHeap->AllocateRange(3);

	pRenderEnv->m_pDevice->CopyDescriptor(m_IntegrateSRVHeapStart,
		m_pSumPerColumnBuffer->GetUAVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	pRenderEnv->m_pDevice->CopyDescriptor(DescriptorHandle(m_IntegrateSRVHeapStart, 1),
		m_pWeightedSHMap->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	assert(!m_MergeSRVHeapStart.IsValid());
	m_MergeSRVHeapStart = pRenderEnv->m_pShaderVisibleSRVHeap->AllocateRange(2);

	pRenderEnv->m_pDevice->CopyDescriptor(m_MergeSRVHeapStart,
		m_pSumPerColumnBuffer->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}

void RadianceCubeMapToSHIrradianceCoefficientsPass::PrecomputeWeightedSHMap(InitParams* pParams)
{
	assert(m_pWeightedSHMap != nullptr);

	RenderEnv* pRenderEnv = pParams->m_pRenderEnv;
	GPUProfiler* pGPUProfiler = pRenderEnv->m_pGPUProfiler;

	D3D12_ROOT_PARAMETER rootParams[kPrecomputeNumRootParams];

	D3D12_DESCRIPTOR_RANGE descriptorRanges[] = {UAVDescriptorRange(1, 0)};
	rootParams[kPrecomputeSRVTableParam] = RootDescriptorTableParameter(ARRAYSIZE(descriptorRanges), descriptorRanges, D3D12_SHADER_VISIBILITY_ALL);

	RootSignatureDesc rootSignatureDesc(kPrecomputeNumRootParams, rootParams);
	RootSignature rootSignature(pRenderEnv->m_pDevice, &rootSignatureDesc, L"RadianceCubeMapToSHIrradianceCoefficientsPass::rootSignature");

	std::vector<PipelineState*> pipelineStates(m_MaxNumSHCoefficients);
	for (u32 SHIndex = 0; SHIndex < m_MaxNumSHCoefficients; ++SHIndex)
	{
		const std::wstring faceSizeStr = std::to_wstring(m_RadianceCubeMapFaceSize);
		const std::wstring SHIndexStr = std::to_wstring(SHIndex);

		const ShaderDefine shaderDefines[] =
		{
			ShaderDefine(L"PRECOMPUTE", L"1"),
			ShaderDefine(L"FACE_SIZE", faceSizeStr.c_str()),
			ShaderDefine(L"SH_INDEX", SHIndexStr.c_str())
		};
		Shader computeShader(L"Shaders//RadianceCubeMapToSHIrradianceCoefficientsCS.hlsl", L"Main", L"cs_6_1", shaderDefines, ARRAYSIZE(shaderDefines));

		ComputePipelineStateDesc pipelineStateDesc;
		pipelineStateDesc.SetRootSignature(&rootSignature);
		pipelineStateDesc.SetComputeShader(&computeShader);

		pipelineStates[SHIndex] = new PipelineState(pRenderEnv->m_pDevice, &pipelineStateDesc, L"RadianceCubeMapToSHIrradianceCoefficientsPass::pipelineState");
	}

	DescriptorHandle SRVHeapStart = pRenderEnv->m_pShaderVisibleSRVHeap->Allocate();
	pRenderEnv->m_pDevice->CopyDescriptor(SRVHeapStart,
		m_pWeightedSHMap->GetUAVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	CommandList* pCommandList = pRenderEnv->m_pCommandListPool->Create(L"RadianceCubeMapToSHIrradianceCoefficientsPass::pPrecomputeCommandList");
	pCommandList->Begin();
#ifdef ENABLE_PROFILING
	u32 profileIndex = pGPUProfiler->StartProfile(pCommandList, "PrecomputeWeightedSHMapPass");
#endif // ENABLE_PROFILING

	pCommandList->SetDescriptorHeaps(pRenderEnv->m_pShaderVisibleSRVHeap);
	pCommandList->SetComputeRootSignature(&rootSignature);
	pCommandList->SetComputeRootDescriptorTable(kPrecomputeSRVTableParam, SRVHeapStart);

	for (u32 SHIndex = 0; SHIndex < m_MaxNumSHCoefficients; ++SHIndex)
	{
		pCommandList->SetPipelineState(pipelineStates[SHIndex]);
		pCommandList->Dispatch(1, m_RadianceCubeMapFaceSize, kNumCubeMapFaces);
	}

	ResourceTransitionBarrier resourceBarrier(m_pWeightedSHMap,
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
		D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

	pCommandList->ResourceBarrier(1, &resourceBarrier);

#ifdef ENABLE_PROFILING
	pGPUProfiler->EndProfile(pCommandList, profileIndex);
#endif // ENABLE_PROFILING
	pCommandList->End();

	++pRenderEnv->m_LastSubmissionFenceValue;
	pRenderEnv->m_pCommandQueue->ExecuteCommandLists(1, &pCommandList, pRenderEnv->m_pFence, pRenderEnv->m_LastSubmissionFenceValue);
	pRenderEnv->m_pFence->WaitForSignalOnCPU(pRenderEnv->m_LastSubmissionFenceValue);

	for (u32 SHIndex = 0; SHIndex < m_MaxNumSHCoefficients; ++SHIndex)
		SafeDelete(pipelineStates[SHIndex]);
}

void RadianceCubeMapToSHIrradianceCoefficientsPass::InitIntegrateRootSignature(InitParams* pParams)
{
	assert(m_pIntegrateRootSignature == nullptr);
	D3D12_ROOT_PARAMETER rootParams[kIntegrateNumRootParams];

	D3D12_DESCRIPTOR_RANGE descriptorRanges[] = {UAVDescriptorRange(1, 0), SRVDescriptorRange(2, 0)};
	rootParams[kIntegrateRootSRVTableParam] = RootDescriptorTableParameter(ARRAYSIZE(descriptorRanges), descriptorRanges, D3D12_SHADER_VISIBILITY_ALL);

	StaticSamplerDesc samplerDesc(StaticSamplerDesc::Point, 0, D3D12_SHADER_VISIBILITY_ALL);
	RootSignatureDesc rootSignatureDesc(kIntegrateNumRootParams, rootParams, 1, &samplerDesc);
	m_pIntegrateRootSignature = new RootSignature(pParams->m_pRenderEnv->m_pDevice, &rootSignatureDesc, L"RadianceCubeMapToSHIrradianceCoefficientsPass::m_pRootSignature");
}

void RadianceCubeMapToSHIrradianceCoefficientsPass::InitIntegratePipelineState(InitParams* pParams)
{
	assert(m_pIntegrateRootSignature != nullptr);
	assert(m_pIntegratePipelineState == nullptr);

	RenderEnv* pRenderEnv = pParams->m_pRenderEnv;

	const std::wstring faceSizeStr = std::to_wstring(m_RadianceCubeMapFaceSize);
	const ShaderDefine shaderDefines[] =
	{
		ShaderDefine(L"INTEGRATE", L"1"),
		ShaderDefine(L"FACE_SIZE", faceSizeStr.c_str())
	};
	Shader computeShader(L"Shaders//RadianceCubeMapToSHIrradianceCoefficientsCS.hlsl", L"Main", L"cs_6_1", shaderDefines, ARRAYSIZE(shaderDefines));

	ComputePipelineStateDesc pipelineStateDesc;
	pipelineStateDesc.SetRootSignature(m_pIntegrateRootSignature);
	pipelineStateDesc.SetComputeShader(&computeShader);

	m_pIntegratePipelineState = new PipelineState(pRenderEnv->m_pDevice, &pipelineStateDesc, L"RadianceCubeMapToSHIrradianceCoefficientsPass::m_pIntegratePipelineState");
}

void RadianceCubeMapToSHIrradianceCoefficientsPass::InitMergeRootSignature(InitParams* pParams)
{
	assert(m_pMergeRootSignature == nullptr);
	D3D12_ROOT_PARAMETER rootParams[kMergeNumRootParams];

	D3D12_DESCRIPTOR_RANGE descriptorRanges[] = {SRVDescriptorRange(1, 0), UAVDescriptorRange(1, 0)};
	rootParams[kMergeRootSRVTableParam] = RootDescriptorTableParameter(ARRAYSIZE(descriptorRanges), descriptorRanges, D3D12_SHADER_VISIBILITY_ALL);

	RootSignatureDesc rootSignatureDesc(kMergeNumRootParams, rootParams);
	m_pMergeRootSignature = new RootSignature(pParams->m_pRenderEnv->m_pDevice, &rootSignatureDesc, L"RadianceCubeMapToSHIrradianceCoefficientsPass::m_pMergeRootSignature");
}

void RadianceCubeMapToSHIrradianceCoefficientsPass::InitMergePipelineState(InitParams* pParams)
{
	assert(m_pMergePipelineState == nullptr);
	assert(m_pMergeRootSignature != nullptr);
	RenderEnv* pRenderEnv = pParams->m_pRenderEnv;

	const std::wstring faceSizeStr = std::to_wstring(m_RadianceCubeMapFaceSize);
	const ShaderDefine shaderDefines[] =
	{
		ShaderDefine(L"MERGE", L"1"),
		ShaderDefine(L"FACE_SIZE", faceSizeStr.c_str())
	};
	Shader computeShader(L"Shaders//RadianceCubeMapToSHIrradianceCoefficientsCS.hlsl", L"Main", L"cs_6_1", shaderDefines, ARRAYSIZE(shaderDefines));

	ComputePipelineStateDesc pipelineStateDesc;
	pipelineStateDesc.SetRootSignature(m_pMergeRootSignature);
	pipelineStateDesc.SetComputeShader(&computeShader);

	m_pMergePipelineState = new PipelineState(pRenderEnv->m_pDevice, &pipelineStateDesc, L"RadianceCubeMapToSHIrradianceCoefficientsPass::m_pMergePipelineState");
}
