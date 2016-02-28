#ifndef __PACKING__
#define __PACKING__

uint PackColor(float3 color)
{
	uint3 colorUInt = uint3(color * 255.0f);
	return ((colorUInt.r << 16u) | (colorUInt.g << 8u) | colorUInt.b);
}

float3 UnpackColor(uint packedColor)
{
	float3 color;
	color.r = (packedColor >> 16u) & 0x000000ff;
	color.g = (packedColor >> 8u)  & 0x000000ff;
	color.b =  packedColor         & 0x000000ff;
	color /= 255.0f;

	return color;
}

uint PackNormal(float3 normal)
{
	int3 normalInt = int3(normal * 255.0f);
	
	int3 axisSigns;
	axisSigns.x = (normalInt.x >> 5)  & 0x04000000;
	axisSigns.y = (normalInt.y >> 14) & 0x00020000;
	axisSigns.z = (normalInt.z >> 23) & 0x00000100;
	
	normalInt = abs(normalInt);
	return (axisSigns.x | (normalInt.x << 18) | axisSigns.y | (normalInt.y << 9) | axisSigns.z | normalInt.z);
}

float3 UnpackNormal(uint packedNormal)
{
	int3 axisSigns;
	axisSigns.x = (packedNormal >> 25) & 0x00000002;
	axisSigns.y = (packedNormal >> 16) & 0x00000002;
	axisSigns.z = (packedNormal >> 7)  & 0x00000002;
	axisSigns = 1 - axisSigns;
	
	float3 normal;
	normal.x = (packedNormal >> 18) & 0x000000ff;
	normal.y = (packedNormal >> 9)  & 0x000000ff;
	normal.z =  packedNormal        & 0x000000ff;
	normal /= 255.0f; 
	normal *= axisSigns;
	
	return normal;
}

#endif // __PACKING__