static const float g_GaussianWeights[5] = {0.2f, 0.2f, 0.2f, 0.2f, 0.2f};

cbuffer Constants32BitBuffer : register(b0)
{
	uint g_ExpShadowMapIndex;
	uint g_IntermediateResultIndex;
};

#ifdef FILTER_X
Texture2DArray<float> g_ExpShadowMaps : register(t0);
RWTexture2DArray<float> g_IntermediateResults : register(u0);
SamplerState g_PointSampler : register(s0);

[numthreads(NUM_THREADS, NUM_THREADS, 1)]
void Main(uint3 globalThreadId : SV_DispatchThreadID)
{
	float2 texCoord = (float2(globalThreadId.xy) + 0.5f) * rcpShadowMapSize;
	g_IntermediateResults[uint3(globalThreadId.xy, g_IntermediateResultIndex)] = filteredDepthValue;
}
#endif // FILTER_X

#ifdef FILTER_Y
Texture2DArray<float> g_IntermediateResults : register(t0);
RWTexture2DArray<float> g_FilteredExpShadowMaps : register(u0);
SamplerState g_PointSampler : register(s0);

[numthreads(NUM_THREADS, NUM_THREADS, 1)]
void Main(uint3 globalThreadId : SV_DispatchThreadID)
{
	float2 texCoord = (float2(globalThreadId.xy) + 0.5f) * rcpShadowMapSize;
	g_FilteredExpShadowMaps[uint3(globalThreadId.xy, g_ExpShadowMapIndex)] = filteredDepthValue;
}
#endif // FILTER_Y