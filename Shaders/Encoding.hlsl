#ifndef __ENCODING__
#define __ENCODING__

uint EncodeColor(float3 color)
{
	uint3 colorInt = uint3(color * 255.0f);
	return ((colorInt.r << 16u) | (colorInt.g << 8u) | colorInt.b);
}

float3 DecodeColor(uint encodedColor)
{
	float3 color;
	color.r = (encodedColor >> 16u) & 0x000000ff;
	color.g = (encodedColor >> 8u)  & 0x000000ff;
	color.b =  encodedColor & 0x000000ff;
	color /= 255.0f;

	return color;
}

float2 EncodeNormal(float3 normal)
{
	normal /= (abs(normal.x) + abs(normal.y) + abs(normal.z));
	normal.xy = normal.z >= 0.0f ? normal.xy : (1.0f - abs(normal.yx)) * (normal.xy >= 0.0f ? 1.0f : -1.0f);
	normal.xy =  normal.xy * 0.5f + 0.5f;
	return normal.xy;
}

float3 DecodeNormal(float2 encodedNormal)
{
	encodedNormal = encodedNormal * 2.0f - 1.0f;

	float3 normal;
	normal.z = 1.0f - abs(encodedNormal.x) - abs(encodedNormal.y);
	normal.xy = normal.z >= 0.0f ? encodedNormal.xy : (1.0f - abs(encodedNormal.yx)) * (encodedNormal.xy >= 0.0f ? 1.0f : -1.0f);
	normal = normalize(normal);

	return normal;
}

#endif // __ENCODING__