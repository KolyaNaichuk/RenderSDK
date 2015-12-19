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

class VisualizeMeshRecorder
{
public:
	VisualizeMeshRecorder(DXDevice* pDevice, DXGI_FORMAT rtvFormat, MeshDataElement meshDataElement, u8 vertexElementFlags);
	~VisualizeMeshRecorder();

	void Record(DXCommandList* pCommandList, DXCommandAllocator* pCommandAllocator,
		DXResource* pRTVTexture, D3D12_CPU_DESCRIPTOR_HANDLE rtvDescriptor,
		UINT numDXDescriptorHeaps, ID3D12DescriptorHeap* pDXFirstDescriptorHeap,
		D3D12_GPU_DESCRIPTOR_HANDLE objectTransformCBVDescriptor,
		Mesh* pMesh, const D3D12_RESOURCE_STATES* pRTVEndState = nullptr);

private:
	DXRootSignature* m_pRootSignature;
	DXPipelineState* m_pPipelineState;
};