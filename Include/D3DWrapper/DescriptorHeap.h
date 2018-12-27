#pragma once

#include "D3DWrapper/Common.h"

class GraphicsDevice;

struct DescriptorHandle
{
	DescriptorHandle();
	DescriptorHandle(D3D12_CPU_DESCRIPTOR_HANDLE CPUHandle, D3D12_GPU_DESCRIPTOR_HANDLE GPUHandle, UINT descriptorSize);
	DescriptorHandle(DescriptorHandle firstDescriptor, INT offsetInDescriptors);
	
	operator D3D12_CPU_DESCRIPTOR_HANDLE() const { return m_CPUHandle; }
	operator D3D12_GPU_DESCRIPTOR_HANDLE() const { return m_GPUHandle; }

	bool IsValid() const;
	bool IsShaderVisible() const;

	void Offset(UINT offsetInDescriptors);
	
	void Reset(D3D12_CPU_DESCRIPTOR_HANDLE CPUHandle, D3D12_GPU_DESCRIPTOR_HANDLE GPUHandle);
	void Reset();

	D3D12_CPU_DESCRIPTOR_HANDLE m_CPUHandle;
	D3D12_GPU_DESCRIPTOR_HANDLE m_GPUHandle;
	UINT m_DescriptorSize;
};

struct DescriptorHeapDesc : public D3D12_DESCRIPTOR_HEAP_DESC
{
	DescriptorHeapDesc(D3D12_DESCRIPTOR_HEAP_TYPE type, UINT numDescriptors, bool shaderVisible);
};

class DescriptorHeap
{
public:
	DescriptorHeap(GraphicsDevice* pDevice, const DescriptorHeapDesc* pDesc, LPCWSTR pName);
	ID3D12DescriptorHeap* GetD3DObject() { return m_D3DDescriptorHeap.Get(); }

	DescriptorHandle Allocate();
	DescriptorHandle AllocateRange(UINT numDescriptors);
	
	void Reset();
	
private:
	ComPtr<ID3D12DescriptorHeap> m_D3DDescriptorHeap;
	DescriptorHandle m_FirstDescriptor;
	UINT m_NumUsedDescriptors;
	UINT m_NumReservedDescriptors;
};
