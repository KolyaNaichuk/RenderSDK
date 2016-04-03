#pragma once

#include "Common/Common.h"

class DXDevice;
class DXRootSignature;
class DXPipelineState;

enum ShadingMode
{
	ShadingMode_Phong = 1,
	ShadingMode_BlinnPhong = 2
};

class TiledShadingRecorder
{
public:
	struct InitParams
	{
		DXDevice* m_pDevice;
		ShadingMode m_ShadingMode;
		u16 m_NumTilesX;
		u16 m_NumTilesY;
		u16 m_NumPointLights;
		u16 m_NumSpotLights;
		bool m_UseDirectLight;
	};
	struct RenderPassParams
	{
	};

	TiledShadingRecorder(InitParams* pParams);
	~TiledShadingRecorder();

	void Record(RenderPassParams* pParams);

private:
	DXRootSignature* m_pRootSignature;
	DXPipelineState* m_pPipelineState;

	u16 m_NumThreadGroupsX;
	u16 m_NumThreadGroupsY;
};