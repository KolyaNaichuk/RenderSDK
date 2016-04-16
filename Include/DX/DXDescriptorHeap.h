#pragma once

#include "DX/DXObject.h"

class DXDevice;

struct DXDescriptorHandle
{
	DXDescriptorHandle();
	DXDescriptorHandle(D3D12_CPU_DESCRIPTOR_HANDLE CPUHandle, D3D12_GPU_DESCRIPTOR_HANDLE GPUHandle);
	DXDescriptorHandle(DXDescriptorHandle firstDescriptor, INT offsetInDescriptors, UINT descriptorSize);

	operator D3D12_CPU_DESCRIPTOR_HANDLE () const { return m_CPUHandle; }
	operator D3D12_GPU_DESCRIPTOR_HANDLE () const { return m_GPUHandle; }

	bool IsValid() const;
	bool IsShaderVisible() const;

	void Offset(INT offsetInDescriptors, UINT descriptorSize);

	void Reset(D3D12_CPU_DESCRIPTOR_HANDLE CPUHandle, D3D12_GPU_DESCRIPTOR_HANDLE GPUHandle);
	void Reset();

	D3D12_CPU_DESCRIPTOR_HANDLE m_CPUHandle;
	D3D12_GPU_DESCRIPTOR_HANDLE m_GPUHandle;
};

struct DXDescriptorHeapDesc : public D3D12_DESCRIPTOR_HEAP_DESC
{
	DXDescriptorHeapDesc(D3D12_DESCRIPTOR_HEAP_TYPE type, UINT numDescriptors, bool shaderVisible);
};

class DXDescriptorHeap : public DXObject<ID3D12DescriptorHeap>
{
public:
	DXDescriptorHeap(DXDevice* pDevice, const DXDescriptorHeapDesc* pDesc, LPCWSTR pName);

	// Kolya. Deprecated. Should be removed
	D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptor(UINT index);
	// Kolya. Deprecated. Should be removed
	D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptor(UINT index);

	DXDescriptorHandle Allocate();
	DXDescriptorHandle AllocateRange(UINT numDescriptors);

	void Reset();

private:
	DXDescriptorHandle m_FirstDescriptor;	
	UINT m_DescriptorSize;
	UINT m_UsedNumDescriptors;
	UINT m_ReservedNumDescriptors;
};
