#pragma once

#include "Math/Vector4.h"

enum MaterialElementFlags
{
	MaterialElementFlag_DiffuseMap = 1 << 0,
	MaterialElementFlag_NormalMap = 1 << 1,
	MaterialElementFlag_SpecularMap = 1 << 2
};

class DXResource;

struct Material
{
	Vector4f m_AmbientColor;
	Vector4f m_DiffuseColor;
	Vector4f m_SpecularColor;
	f32	m_SpecularPower;
	Vector4f m_EmissiveColor;

	DXResource* m_pDiffuseMap;
	DXResource* m_pNormalMap;
	DXResource* m_pSpecularMap;
};
