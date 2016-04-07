#pragma once

#include "Common/Common.h"

class DXDevice;
class DXRootSignature;
class DXPipelineState;
class DXDescriptorHeap;
class DXResource;
class DXCommandList;
class DXCommandAllocator;

struct GBuffer;

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
		DXCommandList* m_pCommandList;
		DXCommandAllocator* m_pCommandAllocator;
				
		DXDescriptorHeap* m_pCBVSRVUAVDescriptorHeap;
		D3D12_GPU_DESCRIPTOR_HANDLE m_ShadingDataCBVHandle;

		DXResource* m_pPointLightGeometryBuffer;
		D3D12_GPU_DESCRIPTOR_HANDLE m_PointLightGeometrySRVHandle;

		DXResource* m_pPointLightPropsBuffer;
		D3D12_GPU_DESCRIPTOR_HANDLE m_PointLightPropsSRVHandle;

		DXResource* m_SpotLightGeometryBuffer;
		D3D12_GPU_DESCRIPTOR_HANDLE m_SpotLightGeometrySRVHandle;

		DXResource* m_SpotLightPropsBuffer;
		D3D12_GPU_DESCRIPTOR_HANDLE m_SpotLightPropsSRVHandle;
		
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