#pragma once

class DXDevice;
class DXPipelineState;
class DXRootSignature;

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

class ScaleTextureRecorder
{
public:
	ScaleTextureRecorder(DXDevice* pDevice, ScaleFactor scaleFactor);
	~ScaleTextureRecorder();

private:
	DXRootSignature* m_pRootSignature;
	DXPipelineState* m_pPipelineState;
};
