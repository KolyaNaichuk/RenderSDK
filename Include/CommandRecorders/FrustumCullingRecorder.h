#pragma once

class DXDevice;
class DXRootSignature;
class DXPipelineState;

class FrustumCullingRecorder
{
public:
	struct InitParams
	{
	};
	struct RenderPassParams
	{
	};
	
	FrustumCullingRecorder(InitParams* pParams);
	~FrustumCullingRecorder();

	void Record(RenderPassParams* pParams);

private:
	DXRootSignature* m_pRootSignature;
	DXPipelineState* m_pPipelineState;
};