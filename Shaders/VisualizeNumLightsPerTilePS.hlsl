#include "Foundation.hlsl"

#define COLOR_MODE_GRAYSCALE		1
#define COLOR_MODE_RADAR			2

#define NUM_RADAR_COLORS			16
static const float4 g_RadarColors[NUM_RADAR_COLORS] =
{
	{0.0f, 0.0f, 0.0f, 1.0f},				// Black
	{0.0f, 0.9255f, 0.9255f, 1.0f},			// Cyan
	{0.0f, 0.62745f, 0.9647f, 1.0f},		// Light blue
	{0.0f, 0.0f, 0.9647f, 1.0f},			// Blue
	{0.0f, 1.0f, 0.0f, 1.0f},				// Bright green
	{0.0f, 0.7843f, 0.0f, 1.0f},			// Green
	{0.0f, 0.5647f, 0.0f, 1.0f},			// Dark green
	{1.0f, 1.0f, 0.0f, 1.0f},				// Yellow
	{0.90588f, 0.75294f, 0.0f, 1.0f},		// Yellow-orange
	{1.0f, 0.5647f, 0.0f, 1.0f},			// Orange
	{1.0f, 0.0f, 0.0f, 1.0f},				// Bright red
	{0.8392f, 0.0f, 0.0f, 1.0f},			// Red
	{0.75294f, 0.0f, 0.0f, 1.0f},			// Dark red
	{1.0f, 0.0f, 1.0f, 1.0f},				// Magenta
	{0.6f, 0.3333f, 0.7882f, 1.0f},			// Purple
	{1.0f, 1.0f, 1.0f, 1.0f}				// White
};

struct PSInput
{
	float4 screenSpacePos	: SV_Position;
	float2 texCoord			: TEXCOORD0;
};

cbuffer Constants32BitBuffer : register(b0)
{
	uint g_MaxNumLights;
}

cbuffer AppDataBuffer : register(b1)
{
	AppData g_AppData;
}

StructuredBuffer<Range> g_LightRangePerTileBuffer : register(t0);

float4 GrayscaleColor(uint numLights, uint maxNumLights)
{
	float percentOfMax = float(numLights) / float(maxNumLights);
	return float4(percentOfMax.rrr, 1.0f);
}

float4 RadarColor(uint numLights, uint maxNumLights)
{
	float lerpFactor = float(numLights) / float(maxNumLights);
	uint colorIndex = lerp(0.0f, float(NUM_RADAR_COLORS - 1), lerpFactor);

	return g_RadarColors[colorIndex];
}

float4 Main(PSInput input) : SV_Target
{
	uint2 pixelPos = uint2(input.screenSpacePos.xy);
	
	uint2 tilePos = pixelPos / g_AppData.screenTileSize;
	uint tileIndex = tilePos.y * g_AppData.numScreenTiles.x + tilePos.x;
	
	uint numLights = g_LightRangePerTileBuffer[tileIndex].length;

#if COLOR_MODE == COLOR_MODE_GRAYSCALE
	float4 color = GrayscaleColor(numLights, g_MaxNumLights);
#endif // COLOR_MODE_GRAYSCALE

#if COLOR_MODE == COLOR_MODE_RADAR
	float4 color = RadarColor(numLights, g_MaxNumLights);
#endif // COLOR_MODE_WEATHER_RADAR

	return color;
}