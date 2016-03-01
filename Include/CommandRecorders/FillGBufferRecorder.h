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
	Mesh* m_pMesh;

	DXCommandList* m_pCommandList;
	DXCommandAllocator* m_pCommandAllocator;

	DXResource* m_pDiffuseTexture;
	D3D12_CPU_DESCRIPTOR_HANDLE m_DiffuseRTVHandle;

	DXResource* m_pNormalTexture;
	D3D12_CPU_DESCRIPTOR_HANDLE m_NormalRTVHandle;

	DXResource* m_pDepthTexture;
	D3D12_CPU_DESCRIPTOR_HANDLE m_DSVHandle;

	DXDescriptorHeap* m_pCBVDescriptorHeap;
	D3D12_GPU_DESCRIPTOR_HANDLE m_TransformCBVHandle;
	D3D12_GPU_DESCRIPTOR_HANDLE m_MaterialCBVHandle;
};

class GBuffer
{
	DXResource* m_pDiffuseTexture;
	D3D12_CPU_DESCRIPTOR_HANDLE m_DiffuseRTVHandle;
	D3D12_GPU_DESCRIPTOR_HANDLE m_DiffuseSRVHandle;

	DXResource* m_pNormalTexture;
	D3D12_CPU_DESCRIPTOR_HANDLE m_NormalRTVHandle;
	D3D12_GPU_DESCRIPTOR_HANDLE m_NormalSRVHandle;

	DXResource* m_pDepthTexture;
	D3D12_CPU_DESCRIPTOR_HANDLE m_DSVHandle;
	D3D12_GPU_DESCRIPTOR_HANDLE m_DepthSRVHandle;
};

class FillGBufferRecorder
{
public:
	FillGBufferRecorder(FillGBufferInitParams* pParams);
	~FillGBufferRecorder();

	void Record(FillGBufferRecordParams* pParams);

private:
	DXRootSignature* m_pRootSignature;
	DXPipelineState* m_pPipelineState;
};
