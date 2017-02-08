#pragma once

#include "D3DWrapper/Common.h"

class RootSignature;
class PipelineState;
class CommandList;
class ColorTexture;

struct RenderEnv;
struct BindingResourceList;

enum ShadingMode
{
	ShadingMode_Phong = 1,
	ShadingMode_BlinnPhong = 2
};

class TiledShadingPass
{
public:
	struct InitParams
	{
		RenderEnv* m_pRenderEnv;
		ShadingMode m_ShadingMode;
		u16 m_TileSize;
		u16 m_NumTilesX;
		u16 m_NumTilesY;
		bool m_EnablePointLights;
		bool m_EnableSpotLights;
		bool m_EnableDirectionalLight;
	};

	struct RenderParams
	{
		RenderEnv* m_pRenderEnv;
		CommandList* m_pCommandList;
		BindingResourceList* m_pResources;
		ColorTexture* m_pAccumLightingTexture;
	};
	
	TiledShadingPass(InitParams* pParams);
	~TiledShadingPass();

	void Record(RenderParams* pParams);

private:
	RootSignature* m_pRootSignature;
	PipelineState* m_pPipelineState;

	u16 m_NumThreadGroupsX;
	u16 m_NumThreadGroupsY;
};