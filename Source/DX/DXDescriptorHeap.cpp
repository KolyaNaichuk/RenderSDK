#include "DX/DXDevice.h"
#include "DX/DXDescriptorHeap.h"

DXDescriptorHeapDesc::DXDescriptorHeapDesc(D3D12_DESCRIPTOR_HEAP_TYPE type, UINT numDescriptors, bool shaderVisible)
{
	Type = type;
	NumDescriptors = numDescriptors;
	Flags = shaderVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	NodeMask = 0;
}

DXDescriptorHeap::DXDescriptorHeap(DXDevice* pDevice, const DXDescriptorHeapDesc* pDesc, LPCWSTR pName)
{
	DXVerify(pDevice->GetDXObject()->CreateDescriptorHeap(pDesc, IID_PPV_ARGS(GetDXObjectAddress())));
	
	mFirstCPUDescriptor = GetDXObject()->GetCPUDescriptorHandleForHeapStart();
	
	if (pDesc->Flags & D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE)
		mFirstGPUDescriptor = GetDXObject()->GetGPUDescriptorHandleForHeapStart();
	else
		mFirstGPUDescriptor.ptr = 0;

	mDescriptorSize = pDevice->GetDXObject()->GetDescriptorHandleIncrementSize(pDesc->Type);

#ifdef _DEBUG
	SetName(pName);
#endif
}

D3D12_CPU_DESCRIPTOR_HANDLE DXDescriptorHeap::GetCPUDescriptor(UINT index)
{
	D3D12_CPU_DESCRIPTOR_HANDLE descriptor;
	descriptor.ptr = mFirstCPUDescriptor.ptr + index * mDescriptorSize;
	return descriptor;
}

D3D12_GPU_DESCRIPTOR_HANDLE DXDescriptorHeap::GetGPUDescriptor(UINT index)
{
	D3D12_GPU_DESCRIPTOR_HANDLE descriptor;
	descriptor.ptr = mFirstGPUDescriptor.ptr + index * mDescriptorSize;
	return descriptor;
}

