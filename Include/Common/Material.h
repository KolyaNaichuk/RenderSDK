#pragma once

#include "Math/Vector4.h"

class DXResource;

enum MaterialElementFlags
{
	MaterialElementFlag_DiffuseMap = 1 << 0,
	MaterialElementFlag_NormalMap = 1 << 1,
	MaterialElementFlag_SpecularMap = 1 << 2
};

struct Material
{
	Vector4f m_AmbientColor;
	Vector4f m_DiffuseColor;
	Vector4f m_SpecularColor;
	f32	m_SpecularPower;
	Vector4f m_EmissiveColor;

	DXResource* m_pDiffuseMap;
	D3D12_GPU_DESCRIPTOR_HANDLE m_DiffuseSRVHandle;

	DXResource* m_pNormalMap;
	D3D12_GPU_DESCRIPTOR_HANDLE m_NormalSRVHandle;

	DXResource* m_pSpecularMap;
	D3D12_GPU_DESCRIPTOR_HANDLE m_SpecualSRVHandle;
};
