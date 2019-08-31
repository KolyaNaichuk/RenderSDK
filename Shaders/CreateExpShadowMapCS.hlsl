struct CreateExpShadowMapParams
{
	float lightProjMatrix32;
	float lightProjMatrix22;
	float lightViewNearPlane;
	float lightRcpViewClipRange;
	float expShadowMapConstant;
};

cbuffer Constants32BitBuffer : register(b0)
{
	uint g_StandardShadowMapIndex;
	uint g_ExpShadowMapIndex;
}

Texture2DArray<float> g_StandardShadowMaps : register(t0);
StructuredBuffer<CreateExpShadowMapParams> g_CreateExpShadowMapParamsBuffer : register(t1);
RWTexture2DArray<float> g_ExpShadowMaps : register(u0);

[numthreads(NUM_THREADS, NUM_THREADS, 1)]
void Main(uint3 globalThreadId : SV_DispatchThreadID)
{
	CreateExpShadowMapParams params = g_CreateExpShadowMapParamsBuffer[g_ExpShadowMapIndex];

	float hardwareDepth = g_StandardShadowMaps[uint3(globalThreadId.xy, g_StandardShadowMapIndex)];
	float lightSpaceDepth = params.lightProjMatrix32 / (hardwareDepth - params.lightProjMatrix22);
	float linearDepth = (lightSpaceDepth - params.lightViewNearPlane) * params.lightRcpViewClipRange;

	g_ExpShadowMaps[uint3(globalThreadId.xy, g_ExpShadowMapIndex)] = exp(params.expShadowMapConstant * linearDepth);
}