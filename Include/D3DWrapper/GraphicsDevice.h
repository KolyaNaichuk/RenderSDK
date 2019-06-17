#pragma once

#include "D3DWrapper/Common.h"

class GraphicsResource;
class GraphicsFactory;

class GraphicsDevice
{
public:
	GraphicsDevice(GraphicsFactory* pFactory, D3D_FEATURE_LEVEL minFeatureLevel, bool useWarpAdapter = false);
	ID3D12Device5* GetD3DObject() { return m_D3DDevice.Get(); }

	void CheckFeatureSupport(D3D12_FEATURE feature, void* pFeatureSupportData, UINT featureSupportDataSize);
	void CopyDescriptor(D3D12_CPU_DESCRIPTOR_HANDLE destDescriptor, D3D12_CPU_DESCRIPTOR_HANDLE srcDescriptor, D3D12_DESCRIPTOR_HEAP_TYPE descriptorHeapType);

	void GetCopyableFootprints(const D3D12_RESOURCE_DESC* pResourceDesc, UINT firstSubresource, UINT numSubresources, UINT64 baseOffsetInBytes,
		D3D12_PLACED_SUBRESOURCE_FOOTPRINT* pFootprints, UINT* pNumRows, UINT64* pRowSizeInBytes, UINT64* pTotalBytes);

	void GetRaytracingAccelerationStructurePrebuildInfo(const D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS* pInputs,
		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO* pInfo);

private:
	ComPtr<ID3D12Device5> m_D3DDevice;
};
