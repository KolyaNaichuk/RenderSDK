static const float g_GaussianWeights[5] = {0.2f, 0.2f, 0.2f, 0.2f, 0.2f};

#ifdef FILTER_X
Texture2D<float> g_ExpShadowMap : register(t0);
SamplerState g_PointSampler : register(s0);

[numthreads(NUM_THREADS, 1, 1)]
void Main(uint3 globalThreadId : SV_DispatchThreadID)
{
	float2 texCoord = (float2(globalThreadId.xy) + 0.5f) * rcpShadowMapSize;

#error Should use GatherRed instead
	float4 depthValues1 = g_ExpShadowMap.SampleLevel(g_PointSampler, texCoord, 0.0f, int2(-2, 0));
	float4 depthValues2 = g_ExpShadowMap.SampleLevel(g_PointSampler, texCoord, 0.0f, int2( 0, 0));
	float4 depthValues3 = g_ExpShadowMap.SampleLevel(g_PointSampler, texCoord, 0.0f, int2( 2, 0));

	float filteredDepthValue =
		g_GaussianWeights[0] * depthValues1.w +
		g_GaussianWeights[1] * depthValues1.z +
		g_GaussianWeights[2] * depthValues2.w +
		g_GaussianWeights[3] * depthValues2.z +
		g_GaussianWeights[4] * depthValues3.w;

#error output result
}
#endif // FILTER_X

#ifdef FILTER_Y
Texture2D<float> g_ExpShadowMap : register(t0);
SamplerState g_PointSampler : register(s0);

[numthreads(1, NUM_THREADS, 1)]
void Main(uint3 globalThreadId : SV_DispatchThreadID)
{
	float2 texCoord = (float2(globalThreadId.xy) + 0.5f) * rcpShadowMapSize;

#error Should use GatherRed instead
	float4 depthValues1 = g_ExpShadowMap.SampleLevel(g_PointSampler, texCoord, 0.0f, int2(0, -2));
	float4 depthValues2 = g_ExpShadowMap.SampleLevel(g_PointSampler, texCoord, 0.0f, int2(0,  0));
	float4 depthValues3 = g_ExpShadowMap.SampleLevel(g_PointSampler, texCoord, 0.0f, int2(0,  2));

	float filteredDepthValue =
		g_GaussianWeights[0] * depthValues1.w +
		g_GaussianWeights[1] * depthValues1.x +
		g_GaussianWeights[2] * depthValues2.w +
		g_GaussianWeights[3] * depthValues2.x +
		g_GaussianWeights[4] * depthValues3.w;

#error output result
}
#endif // FILTER_Y