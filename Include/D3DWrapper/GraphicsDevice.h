#pragma once

#include "D3DWrapper/Common.h"

class GraphicsResource;
class GraphicsFactory;

class GraphicsDevice
{
public:
	GraphicsDevice(GraphicsFactory* pFactory, D3D_FEATURE_LEVEL minFeatureLevel, bool useWarpAdapter = false);
	ID3D12Device* GetD3DObject() { return m_D3DDevice.Get(); }

	void CheckFeatureSupport(D3D12_FEATURE feature, void* pFeatureSupportData, UINT featureSupportDataSize);
	void CopyDescriptor(D3D12_CPU_DESCRIPTOR_HANDLE destDescriptor, D3D12_CPU_DESCRIPTOR_HANDLE srcDescriptor, D3D12_DESCRIPTOR_HEAP_TYPE descriptorHeapType);

private:
	ComPtr<ID3D12Device> m_D3DDevice;
};
