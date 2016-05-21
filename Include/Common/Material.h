#pragma once

#include "Math/Vector4.h"

struct Material
{
	Material();
	Material(const Vector4f& ambientColor, const Vector4f& diffuseColor,
		const Vector4f& specularColor, f32 specularPower, const Vector4f& emissiveColor);

	Vector4f m_AmbientColor;
	Vector4f m_DiffuseColor;
	Vector4f m_SpecularColor;
	f32	m_SpecularPower;
	Vector4f m_EmissiveColor;
};
