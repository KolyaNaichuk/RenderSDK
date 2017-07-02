#include "Reconstruction.hlsl"

Texture2D<float> g_PreviousDepthTexture : register(t0);
SamplerState g_MaxSampler : register(s0);
RWTexture2D<uint> g_ReprojectedDepthTexture : register(u0);

struct ReprojectionData
{
	float2 previousDepthTextureRcpSize;
	uint2 reprojectedDepthTextureSize;
	matrix previousViewProjInvMatrix;
	matrix currentViewProjMatrix;
};

cbuffer ReprojectionDataBuffer : register(b0)
{
	ReprojectionData g_Data;
}

[numthreads(NUM_THREADS_X, NUM_THREADS_Y, 1)]
void Main(uint3 inputPos : SV_DispatchThreadID)
{
	if ((inputPos.x < g_Data.reprojectedDepthTextureSize.x) && (inputPos.y < g_Data.reprojectedDepthTextureSize.y))
	{
		float2 texCoords = (float2(inputPos.xy) + 0.5f) / float2(g_Data.reprojectedDepthTextureSize.xy);
		float2 texOffset = g_Data.previousDepthTextureRcpSize;

		float prevDepthLT = g_PreviousDepthTexture.Sample(g_MaxSampler, texCoords, float2(-texOffset.x, -texOffset.y));
		float prevDepthRT = g_PreviousDepthTexture.Sample(g_MaxSampler, texCoords, float2( texOffset.x, -texOffset.y));
		float prevDepthLB = g_PreviousDepthTexture.Sample(g_MaxSampler, texCoords, float2(-texOffset.x,  texOffset.y));
		float prevDepthRB = g_PreviousDepthTexture.Sample(g_MaxSampler, texCoords, float2( texOffset.x,  texOffset.y));

		float prevDepth = max(max(max(prevDepthLT, prevDepthRT), prevDepthLB), prevDepthRB);
		float4 worldSpacePos = ComputeWorldSpacePosition(texCoords, prevDepth, g_Data.previousViewProjInvMatrix);
		
		float4 clipSpacePos = mul(worldSpacePos, g_Data.currentViewProjMatrix);
		float4 postWDivideProjSpacePos = clipSpacePos / clipSpacePos.w;
						
		float viewSpaceDepth = clipSpacePos.w;
		float depth = (viewSpaceDepth < 0.0f) ? prevDepth : postWDivideProjSpacePos.z;

		float2 unitCubeSpacePos;
		unitCubeSpacePos.x = 0.5f * (postWDivideProjSpacePos.x + 1.0f);
		unitCubeSpacePos.y = 0.5f * (1.0f - postWDivideProjSpacePos.y);
		unitCubeSpacePos.xy = saturate(unitCubeSpacePos.xy);

		uint2 outputPos = int2(unitCubeSpacePos.xy) * g_Data.reprojectedDepthTextureSize.xy;

		float invDepth = saturate(1.0f - depth);
		uint invDepthInt = asuint(invDepth);

		InterlockedMax(g_ReprojectedDepthTexture[inputPos.xy], invDepthInt);
		InterlockedMax(g_ReprojectedDepthTexture[outputPos.xy], invDepthInt);
	}
}
