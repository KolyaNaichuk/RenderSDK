#pragma once

#include "Math/Vector4f.h"

class DXResource;

struct Material
{
	Vector4f m_AmbientColor;
	Vector4f m_DiffuseColor;
	Vector4f m_SpecularColor;
	f32	m_SpecularPower;
	Vector4f m_EmissiveColor;

	DXResource* m_pDiffuseTexture;
	DXResource* m_pNormalTexture;
	DXResource* m_pSpecularTexture;
};