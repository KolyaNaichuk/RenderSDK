#pragma once

#include "D3DWrapper/GraphicsResource.h"

struct RenderEnv;
class ColorTexture;
class CommandList;
class RootSignature;
class PipelineState;

class CubeMapToSHCoefficientsPass
{
public:
	struct ResourceStates
	{
		D3D12_RESOURCE_STATES m_CubeMapState;
	};

	struct InitParams
	{
		const char* m_pName = nullptr;
		RenderEnv* m_pRenderEnv = nullptr;
		ResourceStates m_InputResourceStates;
		u32 m_CubeMapFaceSize = 0;
	};

	struct RenderParams
	{
		RenderEnv* m_pRenderEnv = nullptr;
		CommandList* m_pCommandList = nullptr;
		ColorTexture* m_pCubeMap = nullptr;
	};

	CubeMapToSHCoefficientsPass(InitParams* pParams);
	~CubeMapToSHCoefficientsPass();

	void Record(RenderParams* pParams);
	const ResourceStates* GetOutputResourceStates() const { return &m_OutputResourceStates; }

private:
	void InitResources(InitParams* pParams);

	void InitIntegrateRootSignature(InitParams* pParams);
	void InitIntegratePipelineState(InitParams* pParams);

	void InitMergeRootSignature(InitParams* pParams);
	void InitMergePipelineState(InitParams* pParams);

private:
	enum { kNumSHCoefficients = 9 };

	std::string m_Name;
	u32 m_CubeMapFaceSize = 0;

	RootSignature* m_pIntegrateRootSignature = nullptr;
	PipelineState* m_IntegratePipelineStates[kNumSHCoefficients];

	RootSignature* m_pMergeRootSignature = nullptr;
	PipelineState* m_pMergePipelineState = nullptr;

	ResourceStates m_OutputResourceStates;
};
