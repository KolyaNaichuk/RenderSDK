#include "Reconstruction.hlsl"

struct ReprojectionData
{
	float2 prevDepthTextureRcpSize;
	uint2 reprojectedDepthTextureSize;
	float4 notUsed1[3];
	matrix prevViewProjInvMatrix;
	matrix currViewProjMatrix;
	float4 notUsed2[4];
};

cbuffer ReprojectionDataBuffer : register(b0)
{
	ReprojectionData g_ReprojectionData;
}

SamplerState g_MaxSampler : register(s0);
Texture2D<float> g_PrevDepthTexture : register(t0);
RWTexture2D<uint> g_ReprojectedDepthTexture : register(u0);

[numthreads(NUM_THREADS_X, NUM_THREADS_Y, 1)]
void Main(uint3 inputPos : SV_DispatchThreadID)
{
	if ((inputPos.x < g_ReprojectionData.reprojectedDepthTextureSize.x) && (inputPos.y < g_ReprojectionData.reprojectedDepthTextureSize.y))
	{
		float2 texCoords = (float2(inputPos.xy) + 0.5f) / float2(g_ReprojectionData.reprojectedDepthTextureSize.xy);
		float2 texOffset = g_ReprojectionData.prevDepthTextureRcpSize;

		float prevDepthLT = g_PrevDepthTexture.Sample(g_MaxSampler, texCoords, float2(-texOffset.x, -texOffset.y));
		float prevDepthRT = g_PrevDepthTexture.Sample(g_MaxSampler, texCoords, float2( texOffset.x, -texOffset.y));
		float prevDepthLB = g_PrevDepthTexture.Sample(g_MaxSampler, texCoords, float2(-texOffset.x,  texOffset.y));
		float prevDepthRB = g_PrevDepthTexture.Sample(g_MaxSampler, texCoords, float2( texOffset.x,  texOffset.y));

		float prevDepth = max(max(max(prevDepthLT, prevDepthRT), prevDepthLB), prevDepthRB);
		float4 worldSpacePos = ComputeWorldSpacePosition(texCoords, prevDepth, g_ReprojectionData.prevViewProjInvMatrix);
		
		float4 clipSpacePos = mul(worldSpacePos, g_ReprojectionData.currViewProjMatrix);
		float4 postWDivideProjSpacePos = clipSpacePos / clipSpacePos.w;
						
		float viewSpaceDepth = clipSpacePos.w;
		float depth = (viewSpaceDepth < 0.0f) ? prevDepth : postWDivideProjSpacePos.z;

		float2 unitCubeSpacePos;
		unitCubeSpacePos.x = 0.5f * (postWDivideProjSpacePos.x + 1.0f);
		unitCubeSpacePos.y = 0.5f * (1.0f - postWDivideProjSpacePos.y);
		unitCubeSpacePos.xy = saturate(unitCubeSpacePos.xy);

		uint2 outputPos = uint2(unitCubeSpacePos.xy) * g_ReprojectionData.reprojectedDepthTextureSize.xy;

		float invDepth = saturate(1.0f - depth);
		uint invDepthInt = asuint(invDepth);

		InterlockedMax(g_ReprojectedDepthTexture[inputPos.xy], invDepthInt);
		InterlockedMax(g_ReprojectedDepthTexture[outputPos.xy], invDepthInt);
	}
}
