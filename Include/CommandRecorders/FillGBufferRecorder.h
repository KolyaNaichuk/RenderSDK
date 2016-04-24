#pragma once

#include "Math/Vector4.h"

class DXRootSignature;
class DXPipelineState;
class DXCommandList;
class DXCommandAllocator;
class DXColorTexture;
class DXDepthTexture;
class DXBuffer;
class DXSampler;
class Mesh;

struct DXRenderEnvironment;
struct Material;

struct GBuffer
{
	DXColorTexture* m_pDiffuseTexture;
	DXColorTexture* m_pNormalTexture;
	DXColorTexture* m_pSpecularTexture;
	DXColorTexture* m_pAccumLightTexture;
	DXDepthTexture* m_pDepthTexture;
};

struct MaterialBufferData
{
	Vector4f m_AmbientColor;
	Vector4f m_DiffuseColor;
	Vector4f m_SpecularColor;
	Vector4f m_EmissiveColor;
	f32	m_SpecularPower;
	f32 m_NotUsed[47];
};

class FillGBufferRecorder
{
public:
	struct InitParams
	{
		DXRenderEnvironment* m_pEnv;
		DXGI_FORMAT m_DiffuseRTVFormat;
		DXGI_FORMAT m_NormalRTVFormat;
		DXGI_FORMAT m_SpecularRTVFormat;
		DXGI_FORMAT m_DSVFormat;
		u8 m_VertexElementFlags;
		u8 m_MaterialElementFlags;
	};
	struct RenderPassParams
	{
		DXRenderEnvironment* m_pEnv;
		DXCommandList* m_pCommandList;
		DXCommandAllocator* m_pCommandAllocator;
		DXBuffer* m_pTransformBuffer;
		DXBuffer* m_pMaterialBuffer;
		DXSampler* m_pAnisoSampler;

		Mesh* m_pMesh;
		GBuffer* m_pGBuffer;
		Material* m_pMaterial;
	};

	FillGBufferRecorder(InitParams* pParams);
	~FillGBufferRecorder();

	void Record(RenderPassParams* pParams);

private:
	const u8 m_MaterialElementFlags;

	DXRootSignature* m_pRootSignature;
	DXPipelineState* m_pPipelineState;

	u8 m_TransformCBVRootParam;
	u8 m_MaterialCBVRootParam;
	u8 m_AnisoSamplerRootParam;
	u8 m_DiffuseSRVRootParam;
	u8 m_NormalSRVRootParam;
	u8 m_SpecularSRVRootParam;
};
