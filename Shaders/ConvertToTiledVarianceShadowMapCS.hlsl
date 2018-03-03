cbuffer LightIndexBuffer : register(b0)
{
	uint g_LightIndex;
}

struct ConvertionParams
{
	uint2 tileTopLeftInPixels;
	float lightViewNearPlane;
	float lightRcpViewClipRange;
	float lightProjMatrix43;
	float lightProjMatrix33;
};

Texture2D<float> g_TiledShadowMap : register(t0);
StructuredBuffer<ConvertionParams> g_ConvertionParamsBuffer : register(t1);
RWTexture2D<float2> g_TiledVarianceShadowMap : register(u0);

[numthreads(NUM_THREADS_X, NUM_THREADS_Y, 1)]
void Main(uint3 globalThreadId : SV_DispatchThreadID)
{
	ConvertionParams params = g_ConvertionParamsBuffer[g_LightIndex];
	
	uint2 pixelPos = params.tileTopLeftInPixels + globalThreadId.xy;
	float hardwareDepth = g_TiledShadowMap[pixelPos].r;
	float lightSpaceDepth = params.lightProjMatrix43 / (hardwareDepth - params.lightProjMatrix33);
	float normalizedLightSpaceDepth = (lightSpaceDepth - params.lightViewNearPlane) * params.lightRcpViewClipRange;

	g_TiledVarianceShadowMap[pixelPos] = float2(normalizedLightSpaceDepth, normalizedLightSpaceDepth * normalizedLightSpaceDepth);
}