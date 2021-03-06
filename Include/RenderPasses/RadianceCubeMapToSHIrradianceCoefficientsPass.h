#pragma once

#include "D3DWrapper/GraphicsResource.h"

struct RenderEnv;
class Buffer;
class ColorTexture;
class CommandList;
class RootSignature;
class PipelineState;

class RadianceCubeMapToSHIrradianceCoefficientsPass
{
public:
	struct ResourceStates
	{
		D3D12_RESOURCE_STATES m_RadianceCubeMapState;
		D3D12_RESOURCE_STATES m_SHIrradianceCoefficientBufferState;
	};

	struct InitParams
	{
		RenderEnv* m_pRenderEnv = nullptr;
		u32 m_MaxNumSHCoefficients = 0;
		u32 m_RadianceCubeMapFaceSize = 0;
	};

	struct RenderParams
	{
		RenderEnv* m_pRenderEnv = nullptr;
		CommandList* m_pCommandList = nullptr;
		ResourceStates m_InputResourceStates;
		ColorTexture* m_pRadianceCubeMap = nullptr;
		Buffer* m_pSHIrradianceCoefficientBuffer = nullptr;
		u32 m_NumSHCoefficients = 0;
	};

	RadianceCubeMapToSHIrradianceCoefficientsPass(InitParams* pParams);
	~RadianceCubeMapToSHIrradianceCoefficientsPass();

	void Record(RenderParams* pParams);
	const ResourceStates* GetOutputResourceStates() const { return &m_OutputResourceStates; }

private:
	void InitResources(InitParams* pParams);
	void PrecomputeWeightedSHMap(InitParams* pParams);

	void InitIntegrateRootSignature(InitParams* pParams);
	void InitIntegratePipelineState(InitParams* pParams);

	void InitMergeRootSignature(InitParams* pParams);
	void InitMergePipelineState(InitParams* pParams);

private:
	u32 m_MaxNumSHCoefficients = 0;
	u32 m_RadianceCubeMapFaceSize = 0;

	RootSignature* m_pIntegrateRootSignature = nullptr;
	PipelineState* m_pIntegratePipelineState = nullptr;
	DescriptorHandle m_IntegrateSRVHeapStart;

	RootSignature* m_pMergeRootSignature = nullptr;
	PipelineState* m_pMergePipelineState = nullptr;
	DescriptorHandle m_MergeSRVHeapStart;

	ResourceStates m_OutputResourceStates;
	ColorTexture* m_pWeightedSHMap = nullptr;
	Buffer* m_pSumPerColumnBuffer = nullptr;
};
