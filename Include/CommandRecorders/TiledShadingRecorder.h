#pragma once

#include "Common/Common.h"

class DXRootSignature;
class DXPipelineState;
class DXCommandList;
class DXCommandAllocator;
class DXColorTexture;

struct DXRenderEnvironment;
struct DXBindingResourceList;

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
		DXRenderEnvironment* m_pEnv;
		ShadingMode m_ShadingMode;
		u16 m_NumTilesX;
		u16 m_NumTilesY;
		u16 m_NumPointLights;
		u16 m_NumSpotLights;
		bool m_UseDirectLight;
	};

	struct RenderPassParams
	{
		DXRenderEnvironment* m_pEnv;
		DXCommandList* m_pCommandList;
		DXCommandAllocator* m_pCommandAllocator;
		DXBindingResourceList* m_pResources;
		DXColorTexture* m_pAccumLightTexture;
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