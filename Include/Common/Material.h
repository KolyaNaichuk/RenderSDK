#pragma once

#include "Math/Vector4f.h"

struct Material
{
	Vector4f mAmbientColor;
	Vector4f mDiffuseColor;
	Vector4f mSpecularColor;
	f32	mSpecularPower;
	Vector4f mEmissiveColor;
	
	std::string mTextureFileName;
};