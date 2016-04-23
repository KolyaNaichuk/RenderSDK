#pragma once

#include "Common/Common.h"

class DXRootSignature;
class DXPipelineState;
class DXCommandList;
class DXCommandAllocator;
class DXBuffer;

struct GBuffer;
struct DXRenderEnvironment;

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
		DXBuffer* m_pShadingDataBuffer;
		DXBuffer* m_pPointLightGeometryBuffer;
		DXBuffer* m_pPointLightPropsBuffer;
		DXBuffer* m_pSpotLightGeometryBuffer;
		DXBuffer* m_pSpotLightPropsBuffer;		
		GBuffer* m_pGBuffer;
	};
	
	TiledShadingRecorder(InitParams* pParams);
	~TiledShadingRecorder();

	void Record(RenderPassParams* pParams);

private:
	DXRootSignature* m_pRootSignature;
	DXPipelineState* m_pPipelineState;

	u16 m_NumThreadGroupsX;
	u16 m_NumThreadGroupsY;

	u8 m_ShadingDataCBVRootParam;
	u8 m_AccumLightUAVRootParam;
	u8 m_DepthSRVRootParam;
	u8 m_NormalSRVRootParam;
	u8 m_DiffuseSRVRootParam;
	u8 m_SpecularSRVRootParam;
	u8 m_PointLightGeometrySRVRootParam;
	u8 m_PointLightPropsSRVRootParam;
	u8 m_SpotLightGeometrySRVRootParam;
	u8 m_SpotLightPropsSRVRootParam;
};