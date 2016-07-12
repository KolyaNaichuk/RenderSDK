#pragma once

#include "Common/Common.h"

class D3DRootSignature;
class D3DPipelineState;
class D3DCommandList;
class D3DCommandAllocator;
class D3DColorTexture;

struct D3DRenderEnv;
struct D3DResourceList;

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
		D3DRenderEnv* m_pRenderEnv;
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
		D3DRenderEnv* m_pRenderEnv;
		D3DCommandList* m_pCommandList;
		D3DCommandAllocator* m_pCommandAllocator;
		D3DResourceList* m_pResources;
		D3DColorTexture* m_pAccumLightTexture;
	};
	
	TiledShadingPass(InitParams* pParams);
	~TiledShadingPass();

	void Record(RenderParams* pParams);

private:
	D3DRootSignature* m_pRootSignature;
	D3DPipelineState* m_pPipelineState;

	u16 m_NumThreadGroupsX;
	u16 m_NumThreadGroupsY;
};