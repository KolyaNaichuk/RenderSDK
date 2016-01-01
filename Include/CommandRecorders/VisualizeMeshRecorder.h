#pragma once

#include "Common/Mesh.h"

class DXDevice;
class DXCommandList;
class DXCommandAllocator;
class DXRootSignature;
class DXPipelineState;

enum MeshDataElement
{
	MeshDataElement_Normal = 1,
	MeshDataElement_Color,
	MeshDataElement_TexCoords
};

struct VisualizeMeshParams
{
	Mesh* m_pMesh;

	DXCommandList* m_pCommandList;
	DXCommandAllocator* m_pCommandAllocator;
	
	DXResource* m_pRTVTexture;
	D3D12_CPU_DESCRIPTOR_HANDLE m_RTVHandle;
	const D3D12_RESOURCE_STATES* m_pRTVEndState;

	DXResource* m_pDSVTexture;
	D3D12_CPU_DESCRIPTOR_HANDLE m_DSVHandle;

	UINT m_NumDXDescriptorHeaps;
	ID3D12DescriptorHeap* m_pDXFirstDescriptorHeap;
	D3D12_GPU_DESCRIPTOR_HANDLE m_CBVHandle;
};

class VisualizeMeshRecorder
{
public:
	VisualizeMeshRecorder(DXDevice* pDevice, DXGI_FORMAT rtvFormat, DXGI_FORMAT dsvFormat,
		MeshDataElement meshDataElement, u8 vertexElementFlags);
	~VisualizeMeshRecorder();

	void Record(VisualizeMeshParams* pParams);

private:
	DXRootSignature* m_pRootSignature;
	DXPipelineState* m_pPipelineState;
};