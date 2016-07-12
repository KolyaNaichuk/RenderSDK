#pragma once

#include "D3DWrapper/DXObject.h"

class D3DDevice;

struct D3DDescriptorHandle
{
	D3DDescriptorHandle();
	D3DDescriptorHandle(D3D12_CPU_DESCRIPTOR_HANDLE CPUHandle, D3D12_GPU_DESCRIPTOR_HANDLE GPUHandle, UINT descriptorSize);
	D3DDescriptorHandle(D3DDescriptorHandle firstDescriptor, INT offsetInDescriptors);
	
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

struct D3DDescriptorHeapDesc : public D3D12_DESCRIPTOR_HEAP_DESC
{
	D3DDescriptorHeapDesc(D3D12_DESCRIPTOR_HEAP_TYPE type, UINT numDescriptors, bool shaderVisible);
};

class D3DDescriptorHeap : public DXObject<ID3D12DescriptorHeap>
{
public:
	D3DDescriptorHeap(D3DDevice* pDevice, const D3DDescriptorHeapDesc* pDesc, LPCWSTR pName);
	
	D3DDescriptorHandle Allocate();
	D3DDescriptorHandle AllocateRange(UINT numDescriptors);

	void Reset();

private:
	D3DDescriptorHandle m_FirstDescriptor;
	UINT m_NumUsedDescriptors;
	UINT m_NumReservedDescriptors;
};
