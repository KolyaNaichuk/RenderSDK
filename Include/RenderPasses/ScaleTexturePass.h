#pragma once

class D3DDevice;
class D3DPipelineState;
class D3DRootSignature;

enum ScaleFactor
{
	ScaleFactor_Up2X,
	ScaleFactor_Up3X,
	ScaleFactor_Up4X,
	ScaleFactor_Down2X,
	ScaleFactor_Down3X,
	ScaleFactor_Down4X,
	ScaleFactor_Auto
};

class ScaleTexturePass
{
public:
	ScaleTexturePass(D3DDevice* pDevice, ScaleFactor scaleFactor);
	~ScaleTexturePass();

private:
	D3DRootSignature* m_pRootSignature;
	D3DPipelineState* m_pPipelineState;
};
