#include "DX/DXDescriptorHeap.h"
#include "DX/DXDevice.h"

DXDescriptorHandle::DXDescriptorHandle()
	: m_DescriptorSize(0)
{
	Reset();
}

DXDescriptorHandle::DXDescriptorHandle(D3D12_CPU_DESCRIPTOR_HANDLE CPUHandle, D3D12_GPU_DESCRIPTOR_HANDLE GPUHandle, UINT descriptorSize)
	: m_CPUHandle(CPUHandle)
	, m_GPUHandle(GPUHandle)
	, m_DescriptorSize(descriptorSize)
{
}

DXDescriptorHandle::DXDescriptorHandle(DXDescriptorHandle firstDescriptor, INT offsetInDescriptors)
	: m_CPUHandle(firstDescriptor.m_CPUHandle)
	, m_GPUHandle(firstDescriptor.m_GPUHandle)
	, m_DescriptorSize(firstDescriptor.m_DescriptorSize)
{
	Offset(offsetInDescriptors);
}

void DXDescriptorHandle::Offset(UINT offsetInDescriptors)
{
	assert(IsValid());
	m_CPUHandle.ptr += offsetInDescriptors * m_DescriptorSize;

	if (IsShaderVisible())
		m_GPUHandle.ptr += offsetInDescriptors * m_DescriptorSize;
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

	m_FirstDescriptor.m_DescriptorSize = pDXDevice->GetDescriptorHandleIncrementSize(pDesc->Type);

	m_NumUsedDescriptors = 0;
	m_NumReservedDescriptors = pDesc->NumDescriptors;

#ifdef _DEBUG
	SetName(pName);
#endif
}

DXDescriptorHandle DXDescriptorHeap::Allocate()
{
	assert(m_NumUsedDescriptors < m_NumReservedDescriptors);
	
	INT offsetInDescriptors = m_NumUsedDescriptors;
	++m_NumUsedDescriptors;

	return DXDescriptorHandle(m_FirstDescriptor, offsetInDescriptors);
}

DXDescriptorHandle DXDescriptorHeap::AllocateRange(UINT numDescriptors)
{
	assert(m_NumUsedDescriptors + numDescriptors <= m_NumReservedDescriptors);

	INT offsetInDescriptors = m_NumUsedDescriptors;
	m_NumUsedDescriptors += numDescriptors;

	return DXDescriptorHandle(m_FirstDescriptor, offsetInDescriptors);
}

void DXDescriptorHeap::Reset()
{
	m_NumUsedDescriptors = 0;
}
