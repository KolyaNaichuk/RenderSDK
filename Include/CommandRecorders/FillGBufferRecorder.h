#pragma once

#include "Math/Vector4.h"

class DXDevice;
class DXRootSignature;
class DXPipelineState;
class DXCommandList;
class DXCommandAllocator;
class DXDescriptorHeap;
class DXResource;
class Mesh;

struct Material;

struct GBuffer
{
	DXResource* m_pDiffuseTexture;
	D3D12_CPU_DESCRIPTOR_HANDLE m_DiffuseRTVHandle;
	D3D12_GPU_DESCRIPTOR_HANDLE m_DiffuseSRVHandle;

	DXResource* m_pNormalTexture;
	D3D12_CPU_DESCRIPTOR_HANDLE m_NormalRTVHandle;
	D3D12_GPU_DESCRIPTOR_HANDLE m_NormalSRVHandle;

	DXResource* m_pSpecularTexture;
	D3D12_CPU_DESCRIPTOR_HANDLE m_SpecularRTVHandle;
	D3D12_GPU_DESCRIPTOR_HANDLE m_SpecularSRVHandle;
	
	DXResource* m_pAccumLightTexture;
	D3D12_GPU_DESCRIPTOR_HANDLE m_AccumLightUAVHandle;
	D3D12_GPU_DESCRIPTOR_HANDLE m_AccumLightSRVHandle;

	DXResource* m_pDepthTexture;
	D3D12_CPU_DESCRIPTOR_HANDLE m_DSVHandle;
	D3D12_GPU_DESCRIPTOR_HANDLE m_DepthSRVHandle;
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
		DXDevice* m_pDevice;
		DXGI_FORMAT m_DiffuseRTVFormat;
		DXGI_FORMAT m_NormalRTVFormat;
		DXGI_FORMAT m_SpecularRTVFormat;
		DXGI_FORMAT m_DSVFormat;
		u8 m_VertexElementFlags;
		u8 m_MaterialElementFlags;
	};
	struct RenderPassParams
	{
		DXCommandList* m_pCommandList;
		DXCommandAllocator* m_pCommandAllocator;

		GBuffer* m_pGBuffer;
		Mesh* m_pMesh;
		Material* m_pMaterial;

		DXDescriptorHeap* m_pCBVDescriptorHeap;
		D3D12_GPU_DESCRIPTOR_HANDLE m_TransformCBVHandle;
		D3D12_GPU_DESCRIPTOR_HANDLE m_MaterialCBVHandle;

		DXDescriptorHeap* m_pSamplerDescriptorHeap;
		D3D12_GPU_DESCRIPTOR_HANDLE m_AnisoSamplerHandle;
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
