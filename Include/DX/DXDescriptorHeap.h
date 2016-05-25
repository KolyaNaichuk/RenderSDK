#pragma once

#include "DX/DXObject.h"

class DXDevice;

struct DXDescriptorHandle
{
	DXDescriptorHandle();
	DXDescriptorHandle(D3D12_CPU_DESCRIPTOR_HANDLE CPUHandle, D3D12_GPU_DESCRIPTOR_HANDLE GPUHandle, UINT descriptorSize);
	DXDescriptorHandle(DXDescriptorHandle firstDescriptor, INT offsetInDescriptors);
	
	operator D3D12_CPU_DESCRIPTOR_HANDLE () const { return m_CPUHandle; }
	operator D3D12_GPU_DESCRIPTOR_HANDLE () const { return m_GPUHandle; }

	bool IsValid() const;
	bool IsShaderVisible() const;

	void Offset(UINT offsetInDescriptors);
	
	void Reset(D3D12_CPU_DESCRIPTOR_HANDLE CPUHandle, D3D12_GPU_DESCRIPTOR_HANDLE GPUHandle);
	void Reset();

	D3D12_CPU_DESCRIPTOR_HANDLE m_CPUHandle;
	D3D12_GPU_DESCRIPTOR_HANDLE m_GPUHandle;
	UINT m_DescriptorSize;
};

struct DXDescriptorHeapDesc : public D3D12_DESCRIPTOR_HEAP_DESC
{
	DXDescriptorHeapDesc(D3D12_DESCRIPTOR_HEAP_TYPE type, UINT numDescriptors, bool shaderVisible);
};

class DXDescriptorHeap : public DXObject<ID3D12DescriptorHeap>
{
public:
	DXDescriptorHeap(DXDevice* pDevice, const DXDescriptorHeapDesc* pDesc, LPCWSTR pName);
	
	DXDescriptorHandle Allocate();
	DXDescriptorHandle AllocateRange(UINT numDescriptors);

	void Reset();

private:
	DXDescriptorHandle m_FirstDescriptor;
	UINT m_NumUsedDescriptors;
	UINT m_NumReservedDescriptors;
};
