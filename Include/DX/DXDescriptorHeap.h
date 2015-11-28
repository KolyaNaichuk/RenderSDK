#pragma once

#include "DXObject.h"

class DXDevice;

struct DXDescriptorHeapDesc : public D3D12_DESCRIPTOR_HEAP_DESC
{
	DXDescriptorHeapDesc(D3D12_DESCRIPTOR_HEAP_TYPE type, UINT numDescriptors, bool shaderVisible);
};

class DXDescriptorHeap : public DXObject<ID3D12DescriptorHeap>
{
public:
	DXDescriptorHeap(DXDevice* pDevice, const DXDescriptorHeapDesc* pDesc, LPCWSTR pName);

	D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptor(UINT index);
	D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptor(UINT index);

private:
	D3D12_CPU_DESCRIPTOR_HANDLE mFirstCPUDescriptor;
	D3D12_GPU_DESCRIPTOR_HANDLE mFirstGPUDescriptor;
	UINT mDescriptorSize;
};
