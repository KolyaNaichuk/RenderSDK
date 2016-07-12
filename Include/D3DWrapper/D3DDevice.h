#pragma once

#include "DXObject.h"

class D3DResource;
class D3DFactory;

class D3DDevice : public DXObject<ID3D12Device>
{
public:
	D3DDevice(D3DFactory* pFactory, D3D_FEATURE_LEVEL minFeatureLevel, bool useWarpAdapter = false);
	
	void CheckFeatureSupport(D3D12_FEATURE feature, void* pFeatureSupportData, UINT featureSupportDataSize);
	void CopyDescriptor(D3D12_CPU_DESCRIPTOR_HANDLE destDescriptor, D3D12_CPU_DESCRIPTOR_HANDLE srcDescriptor, D3D12_DESCRIPTOR_HEAP_TYPE descriptorHeapType);
};
