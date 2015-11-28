#pragma once

#include "Math/Vector4f.h"

struct Material
{
	Vector4f mAmbientColor;
	Vector4f mDiffuseColor;
	Vector4f mSpecularColor;
	Vector4f mEmissiveColor;
	f32 mSpecularPower;
};