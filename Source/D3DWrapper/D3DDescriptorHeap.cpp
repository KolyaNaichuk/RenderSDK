#include "D3DWrapper/D3DDescriptorHeap.h"
#include "D3DWrapper/D3DDevice.h"

D3DDescriptorHandle::D3DDescriptorHandle()
	: m_DescriptorSize(0)
{
	Reset();
}

D3DDescriptorHandle::D3DDescriptorHandle(D3D12_CPU_DESCRIPTOR_HANDLE CPUHandle, D3D12_GPU_DESCRIPTOR_HANDLE GPUHandle, UINT descriptorSize)
	: m_CPUHandle(CPUHandle)
	, m_GPUHandle(GPUHandle)
	, m_DescriptorSize(descriptorSize)
{
}

D3DDescriptorHandle::D3DDescriptorHandle(D3DDescriptorHandle firstDescriptor, INT offsetInDescriptors)
	: m_CPUHandle(firstDescriptor.m_CPUHandle)
	, m_GPUHandle(firstDescriptor.m_GPUHandle)
	, m_DescriptorSize(firstDescriptor.m_DescriptorSize)
{
	Offset(offsetInDescriptors);
}

void D3DDescriptorHandle::Offset(UINT offsetInDescriptors)
{
	assert(IsValid());
	m_CPUHandle.ptr += offsetInDescriptors * m_DescriptorSize;

	if (IsShaderVisible())
		m_GPUHandle.ptr += offsetInDescriptors * m_DescriptorSize;
}

bool D3DDescriptorHandle::IsValid() const
{
	return (m_CPUHandle.ptr != 0);
}

bool D3DDescriptorHandle::IsShaderVisible() const
{
	return (m_GPUHandle.ptr != 0);
}

void D3DDescriptorHandle::Reset(D3D12_CPU_DESCRIPTOR_HANDLE CPUHandle, D3D12_GPU_DESCRIPTOR_HANDLE GPUHandle)
{
	m_CPUHandle = CPUHandle;
	m_GPUHandle = GPUHandle;
}

void D3DDescriptorHandle::Reset()
{
	m_CPUHandle.ptr = 0;
	m_GPUHandle.ptr = 0;
}

D3DDescriptorHeapDesc::D3DDescriptorHeapDesc(D3D12_DESCRIPTOR_HEAP_TYPE type, UINT numDescriptors, bool shaderVisible)
{
	Type = type;
	NumDescriptors = numDescriptors;
	Flags = shaderVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	NodeMask = 0;
}

D3DDescriptorHeap::D3DDescriptorHeap(D3DDevice* pDevice, const D3DDescriptorHeapDesc* pDesc, LPCWSTR pName)
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

D3DDescriptorHandle D3DDescriptorHeap::Allocate()
{
	assert(m_NumUsedDescriptors < m_NumReservedDescriptors);
	
	INT offsetInDescriptors = m_NumUsedDescriptors;
	++m_NumUsedDescriptors;

	return D3DDescriptorHandle(m_FirstDescriptor, offsetInDescriptors);
}

D3DDescriptorHandle D3DDescriptorHeap::AllocateRange(UINT numDescriptors)
{
	assert(m_NumUsedDescriptors + numDescriptors <= m_NumReservedDescriptors);

	INT offsetInDescriptors = m_NumUsedDescriptors;
	m_NumUsedDescriptors += numDescriptors;

	return D3DDescriptorHandle(m_FirstDescriptor, offsetInDescriptors);
}

void D3DDescriptorHeap::Reset()
{
	m_NumUsedDescriptors = 0;
}
