#pragma once

#include "D3DWrapper/Common.h"

class GraphicsDevice;
class PipelineState;
class RootSignature;

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
	ScaleTexturePass(GraphicsDevice* pDevice, ScaleFactor scaleFactor);
	~ScaleTexturePass();

private:
	RootSignature* m_pRootSignature;
	PipelineState* m_pPipelineState;
};
