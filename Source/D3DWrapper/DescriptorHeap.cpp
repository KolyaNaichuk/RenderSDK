#include "D3DWrapper/DescriptorHeap.h"
#include "D3DWrapper/GraphicsDevice.h"

DescriptorHandle::DescriptorHandle()
	: m_DescriptorSize(0)
{
	Reset();
}

DescriptorHandle::DescriptorHandle(D3D12_CPU_DESCRIPTOR_HANDLE CPUHandle, D3D12_GPU_DESCRIPTOR_HANDLE GPUHandle, UINT descriptorSize)
	: m_CPUHandle(CPUHandle)
	, m_GPUHandle(GPUHandle)
	, m_DescriptorSize(descriptorSize)
{
}

DescriptorHandle::DescriptorHandle(DescriptorHandle firstDescriptor, INT offsetInDescriptors)
	: m_CPUHandle(firstDescriptor.m_CPUHandle)
	, m_GPUHandle(firstDescriptor.m_GPUHandle)
	, m_DescriptorSize(firstDescriptor.m_DescriptorSize)
{
	Offset(offsetInDescriptors);
}

void DescriptorHandle::Offset(UINT offsetInDescriptors)
{
	assert(IsValid());
	m_CPUHandle.ptr += offsetInDescriptors * m_DescriptorSize;

	if (IsShaderVisible())
		m_GPUHandle.ptr += offsetInDescriptors * m_DescriptorSize;
}

bool DescriptorHandle::IsValid() const
{
	return (m_CPUHandle.ptr != 0);
}

bool DescriptorHandle::IsShaderVisible() const
{
	return (m_GPUHandle.ptr != 0);
}

void DescriptorHandle::Reset(D3D12_CPU_DESCRIPTOR_HANDLE CPUHandle, D3D12_GPU_DESCRIPTOR_HANDLE GPUHandle)
{
	m_CPUHandle = CPUHandle;
	m_GPUHandle = GPUHandle;
}

void DescriptorHandle::Reset()
{
	m_CPUHandle.ptr = 0;
	m_GPUHandle.ptr = 0;
}

DescriptorHeapDesc::DescriptorHeapDesc(D3D12_DESCRIPTOR_HEAP_TYPE type, UINT numDescriptors, bool shaderVisible)
{
	Type = type;
	NumDescriptors = numDescriptors;
	Flags = shaderVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	NodeMask = 0;
}

DescriptorHeap::DescriptorHeap(GraphicsDevice* pDevice, const DescriptorHeapDesc* pDesc, LPCWSTR pName)
{
	ID3D12Device* pD3DDevice = pDevice->GetD3DObject();
	VerifyD3DResult(pD3DDevice->CreateDescriptorHeap(pDesc, IID_PPV_ARGS(&m_D3DDescriptorHeap)));
	
	m_FirstDescriptor.m_CPUHandle = GetD3DObject()->GetCPUDescriptorHandleForHeapStart();
	
	if (pDesc->Flags & D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE)
		m_FirstDescriptor.m_GPUHandle = GetD3DObject()->GetGPUDescriptorHandleForHeapStart();

	m_FirstDescriptor.m_DescriptorSize = pD3DDevice->GetDescriptorHandleIncrementSize(pDesc->Type);

	m_NumUsedDescriptors = 0;
	m_NumReservedDescriptors = pDesc->NumDescriptors;

#ifdef ENABLE_GRAPHICS_DEBUGGING
	VerifyD3DResult(m_D3DDescriptorHeap->SetName(pName));
#endif // ENABLE_GRAPHICS_DEBUGGING
}

DescriptorHandle DescriptorHeap::Allocate()
{
	assert(m_NumUsedDescriptors < m_NumReservedDescriptors);
	
	INT offsetInDescriptors = m_NumUsedDescriptors;
	++m_NumUsedDescriptors;

	return DescriptorHandle(m_FirstDescriptor, offsetInDescriptors);
}

DescriptorHandle DescriptorHeap::AllocateRange(UINT numDescriptors)
{
	assert(numDescriptors > 0);
	assert(m_NumUsedDescriptors + numDescriptors <= m_NumReservedDescriptors);

	INT offsetInDescriptors = m_NumUsedDescriptors;
	m_NumUsedDescriptors += numDescriptors;

	return DescriptorHandle(m_FirstDescriptor, offsetInDescriptors);
}

void DescriptorHeap::Reset()
{
	m_NumUsedDescriptors = 0;
}
