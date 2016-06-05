#ifndef __MATERIAL__
#define __MATERIAL__

struct Material
{
	float4 ambientColor;
	float4 diffuseColor;
	float4 specularColor;
	float  specularPower;
	float4 emissiveColor;
};

#endif // __MATERIAL__