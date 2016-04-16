#include "DX/DXDescriptorHeap.h"
#include "DX/DXDevice.h"

DXDescriptorHandle::DXDescriptorHandle()
{
	Reset();
}

DXDescriptorHandle::DXDescriptorHandle(D3D12_CPU_DESCRIPTOR_HANDLE CPUHandle, D3D12_GPU_DESCRIPTOR_HANDLE GPUHandle)
	: m_CPUHandle(CPUHandle)
	, m_GPUHandle(GPUHandle)
{
}

DXDescriptorHandle::DXDescriptorHandle(DXDescriptorHandle firstDescriptor, INT offsetInDescriptors, UINT descriptorSize)
	: m_CPUHandle(firstDescriptor.m_CPUHandle)
	, m_GPUHandle(firstDescriptor.m_GPUHandle)
{
	Offset(offsetInDescriptors, descriptorSize);
}

void DXDescriptorHandle::Offset(INT offsetInDescriptors, UINT descriptorSize)
{
	assert(IsValid());
	m_CPUHandle.ptr += offsetInDescriptors * descriptorSize;

	if (IsShaderVisible())
		m_GPUHandle.ptr += offsetInDescriptors * descriptorSize;
}

bool DXDescriptorHandle::IsValid() const
{
	return (m_CPUHandle.ptr != 0);
}

bool DXDescriptorHandle::IsShaderVisible() const
{
	return (m_GPUHandle.ptr != 0);
}

void DXDescriptorHandle::Reset(D3D12_CPU_DESCRIPTOR_HANDLE CPUHandle, D3D12_GPU_DESCRIPTOR_HANDLE GPUHandle)
{
	m_CPUHandle = CPUHandle;
	m_GPUHandle = GPUHandle;
}

void DXDescriptorHandle::Reset()
{
	m_CPUHandle.ptr = 0;
	m_GPUHandle.ptr = 0;
}

DXDescriptorHeapDesc::DXDescriptorHeapDesc(D3D12_DESCRIPTOR_HEAP_TYPE type, UINT numDescriptors, bool shaderVisible)
{
	Type = type;
	NumDescriptors = numDescriptors;
	Flags = shaderVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	NodeMask = 0;
}

DXDescriptorHeap::DXDescriptorHeap(DXDevice* pDevice, const DXDescriptorHeapDesc* pDesc, LPCWSTR pName)
{
	ID3D12Device* pDXDevice = pDevice->GetDXObject();
	DXVerify(pDXDevice->CreateDescriptorHeap(pDesc, IID_PPV_ARGS(GetDXObjectAddress())));
	
	m_FirstDescriptor.m_CPUHandle = GetDXObject()->GetCPUDescriptorHandleForHeapStart();
	if (pDesc->Flags & D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE)
		m_FirstDescriptor.m_GPUHandle = GetDXObject()->GetGPUDescriptorHandleForHeapStart();
	
	m_DescriptorSize = pDXDevice->GetDescriptorHandleIncrementSize(pDesc->Type);
	m_UsedNumDescriptors = 0;
	m_ReservedNumDescriptors = pDesc->NumDescriptors;

#ifdef _DEBUG
	SetName(pName);
#endif
}

D3D12_CPU_DESCRIPTOR_HANDLE DXDescriptorHeap::GetCPUDescriptor(UINT index)
{
	DXDescriptorHandle descriptorHandle = m_FirstDescriptor;
	descriptorHandle.Offset(index, m_DescriptorSize);
	return descriptorHandle;
}

D3D12_GPU_DESCRIPTOR_HANDLE DXDescriptorHeap::GetGPUDescriptor(UINT index)
{
	DXDescriptorHandle descriptorHandle = m_FirstDescriptor;
	descriptorHandle.Offset(index, m_DescriptorSize);
	return descriptorHandle;
}

DXDescriptorHandle DXDescriptorHeap::Allocate()
{
	assert(m_UsedNumDescriptors < m_ReservedNumDescriptors);
	
	INT offsetInDescriptors = m_UsedNumDescriptors;
	++m_UsedNumDescriptors;

	return DXDescriptorHandle(m_FirstDescriptor, offsetInDescriptors, m_DescriptorSize);
}

DXDescriptorHandle DXDescriptorHeap::AllocateRange(UINT numDescriptors)
{
	assert(m_UsedNumDescriptors + numDescriptors <= m_ReservedNumDescriptors);

	INT offsetInDescriptors = m_UsedNumDescriptors;
	m_UsedNumDescriptors += numDescriptors;

	return DXDescriptorHandle(m_FirstDescriptor, offsetInDescriptors, m_DescriptorSize);
}

void DXDescriptorHeap::Reset()
{
	m_UsedNumDescriptors = 0;
}

