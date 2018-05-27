struct CreateExpShadowMapParams
{
	float rcpShadowMapSize;
	float lightProjMatrix43;
	float lightProjMatrix33;
	float lightViewNearPlane;
	float lightRcpViewClipRange;
	float expShadowMapConstant;
};

cbuffer Constants32BitBuffer : register(b0)
{
	uint g_ShadowMapIndex;
}

Texture2D<float> g_StandardShadowMap : register(t0);
StructuredBuffer<CreateExpShadowMapParams> g_CreateExpShadowMapParamsBuffer : register(t1);
RWTexture2DArray<float> g_ExpShadowMaps : register(u0);

SamplerState g_PointSampler : register(s0);

[numthreads(NUM_THREADS_X, NUM_THREADS_Y, 1)]
void Main(uint3 globalThreadId : SV_DispatchThreadID)
{
	CreateExpShadowMapParams params = g_CreateExpShadowMapParamsBuffer[g_ShadowMapIndex];

#error How do I handle reading shadow map outside it bounds?
	uint2 pixelPos = globalThreadId.xy << 1;
	float2 texCoord = (float2(pixelPos) + 0.5f) * params.rcpShadowMapSize;
	float4 depthValues = g_StandardShadowMap.GatherRed(g_PointSampler, texCoord);
	
	float4 lightSpaceDepthValues;
	lightSpaceDepthValues.x = params.lightProjMatrix43 / (depthValues.x - params.lightProjMatrix33);
	lightSpaceDepthValues.y = params.lightProjMatrix43 / (depthValues.y - params.lightProjMatrix33);
	lightSpaceDepthValues.z = params.lightProjMatrix43 / (depthValues.z - params.lightProjMatrix33);
	lightSpaceDepthValues.w = params.lightProjMatrix43 / (depthValues.w - params.lightProjMatrix33);
		
	float4 linearDepthValues;
	linearDepthValues.x = (lightSpaceDepthValues.x - params.lightViewNearPlane) * params.lightRcpViewClipRange;
	linearDepthValues.y = (lightSpaceDepthValues.y - params.lightViewNearPlane) * params.lightRcpViewClipRange;
	linearDepthValues.z = (lightSpaceDepthValues.z - params.lightViewNearPlane) * params.lightRcpViewClipRange;
	linearDepthValues.w = (lightSpaceDepthValues.w - params.lightViewNearPlane) * params.lightRcpViewClipRange;
	
#error expDepthValues might exceed 1
	float4 expDepthValues = exp(params.expShadowMapConstant * linearDepthValues);
#if CREATE_DOWNSCALED_2X
	float averageExpDepth = dot(0.25f, expDepthValues);
	g_ExpShadowMaps[uint3(pixelPos + uint2(0, 0), g_ShadowMapIndex)] = averageExpDepth;
#else // CREATE_DOWNSCALED_2X
	g_ExpShadowMaps[uint3(pixelPos + uint2(0, 1), g_ShadowMapIndex)] = expDepthValues.x;
	g_ExpShadowMaps[uint3(pixelPos + uint2(1, 1), g_ShadowMapIndex)] = expDepthValues.y;
	g_ExpShadowMaps[uint3(pixelPos + uint2(1, 0), g_ShadowMapIndex)] = expDepthValues.z;
	g_ExpShadowMaps[uint3(pixelPos + uint2(0, 0), g_ShadowMapIndex)] = expDepthValues.w;
#endif // CREATE_DOWNSCALED_2X
}