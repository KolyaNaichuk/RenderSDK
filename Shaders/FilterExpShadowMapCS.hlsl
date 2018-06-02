static const float4 g_FilterWeights = {1.0f / 7.0f, 1.0f / 7.0f, 1.0f / 7.0f, 1.0f / 7.0f};

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
	float3 texCoord = float3((float2(globalThreadId.xy) + 0.5f) * rcpShadowMapSize, g_ExpShadowMapIndex);

	float4 depthValues1 = g_ExpShadowMaps.Gather(g_PointSampler, texCoord, int2(0, 0), int2(-1, 0), int2(-2, 0), int2(-3, 0));
	float4 depthValues2 = g_ExpShadowMaps.Gather(g_PointSampler, texCoord, int2(0, 0), int2( 1, 0), int2( 2, 0), int2( 3, 0));

	float filteredDepthValue = dot(g_FilterWeights.xyzw, depthValues1.xyzw) + dot(g_FilterWeights.yzw, depthValues2.yzw);
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
	float3 texCoord = float3((float2(globalThreadId.xy) + 0.5f) * rcpShadowMapSize, g_IntermediateResultIndex);

	float4 depthValues1 = g_IntermediateResults.Gather(g_PointSampler, texCoord, int2(0, 0), int2(0, -1), int2(0, -2), int2(0, -3));
	float4 depthValues2 = g_IntermediateResults.Gather(g_PointSampler, texCoord, int2(0, 0), int2(0,  1), int2(0,  2), int2(0,  3));

	float filteredDepthValue = dot(g_FilterWeights.xyzw, depthValues1.xyzw) + dot(g_FilterWeights.yzw, depthValues2.yzw);
	g_FilteredExpShadowMaps[uint3(globalThreadId.xy, g_ExpShadowMapIndex)] = filteredDepthValue;
}
#endif // FILTER_Y