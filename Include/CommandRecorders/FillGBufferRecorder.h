#pragma once

#include "Common/Material.h"

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
	
	DXResource* m_AccumulatedLightTexture;
	D3D12_CPU_DESCRIPTOR_HANDLE m_AccumulatedLightRTVHandle;
	D3D12_GPU_DESCRIPTOR_HANDLE m_AccumulatedLightSRVHandle;

	DXResource* m_pDepthTexture;
	D3D12_CPU_DESCRIPTOR_HANDLE m_DSVHandle;
	D3D12_GPU_DESCRIPTOR_HANDLE m_DepthSRVHandle;
};

struct FillGBufferInitParams
{
	DXDevice* m_pDevice;
	DXGI_FORMAT m_DiffuseRTVFormat;
	DXGI_FORMAT m_NormalRTVFormat;
	DXGI_FORMAT m_SpecularRTVFormat;
	DXGI_FORMAT m_DSVFormat;
	u8 m_VertexElementFlags;
	u8 m_MaterialElementFlags;
};

struct FillGBufferRecordParams
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

class FillGBufferRecorder
{
public:
	FillGBufferRecorder(FillGBufferInitParams* pParams);
	~FillGBufferRecorder();

	void Record(FillGBufferRecordParams* pParams);

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
