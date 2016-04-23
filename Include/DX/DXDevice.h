#pragma once

#include "DXObject.h"

class DXResource;
class DXFactory;

class DXDevice : public DXObject<ID3D12Device>
{
public:
	DXDevice(DXFactory* pFactory, D3D_FEATURE_LEVEL minFeatureLevel, bool useWarpAdapter = false);

	void CheckFeatureSupport(D3D12_FEATURE feature, void* pFeatureSupportData, UINT featureSupportDataSize);
};
