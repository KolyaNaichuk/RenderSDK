#include "Foundation.hlsl"
#include "Reconstruction.hlsl"

cbuffer AppDataBuffer : register(b0)
{
	AppData g_AppData;
}

SamplerState g_MaxSampler : register(s0);
Texture2D<float> g_PrevDepthTexture : register(t0);
RWTexture2D<uint> g_ReprojectedDepthTexture : register(u0);

[numthreads(NUM_THREADS_X, NUM_THREADS_Y, 1)]
void Main(uint3 inputPos : SV_DispatchThreadID)
{
	if ((inputPos.x < g_AppData.screenQuarterSize.x) && (inputPos.y < g_AppData.screenQuarterSize.y))
	{
		float2 texCoords = (float2(inputPos.xy) + 0.5f) * g_AppData.rcpScreenQuarterSize;
		float2 texOffset = g_AppData.rcpScreenSize;

		float prevDepthLT = g_PrevDepthTexture.SampleLevel(g_MaxSampler, texCoords + float2(-texOffset.x, -texOffset.y), 0);
		float prevDepthRT = g_PrevDepthTexture.SampleLevel(g_MaxSampler, texCoords + float2( texOffset.x, -texOffset.y), 0);
		float prevDepthLB = g_PrevDepthTexture.SampleLevel(g_MaxSampler, texCoords + float2(-texOffset.x,  texOffset.y), 0);
		float prevDepthRB = g_PrevDepthTexture.SampleLevel(g_MaxSampler, texCoords + float2( texOffset.x,  texOffset.y), 0);

		float prevDepth = max(max(max(prevDepthLT, prevDepthRT), prevDepthLB), prevDepthRB);
		float4 worldSpacePos = ComputeWorldSpacePosition(texCoords, prevDepth, g_AppData.prevViewProjInvMatrix);
		
		float4 clipSpacePos = mul(g_AppData.viewProjMatrix, worldSpacePos);
		float4 postWDivideProjSpacePos = clipSpacePos / clipSpacePos.w;
						
		float viewSpaceDepth = clipSpacePos.w;
		float depth = (viewSpaceDepth < 0.0f) ? prevDepth : postWDivideProjSpacePos.z;

		float2 unitCubeSpacePos;
		unitCubeSpacePos.x = 0.5f * (postWDivideProjSpacePos.x + 1.0f);
		unitCubeSpacePos.y = 0.5f * (1.0f - postWDivideProjSpacePos.y);
		unitCubeSpacePos.xy = saturate(unitCubeSpacePos.xy);

		uint2 outputPos = uint2(unitCubeSpacePos.xy) * g_AppData.screenQuarterSize;

		float invDepth = saturate(1.0f - depth);
		uint invDepthInt = asuint(invDepth);

		InterlockedMax(g_ReprojectedDepthTexture[inputPos.xy], invDepthInt);
		InterlockedMax(g_ReprojectedDepthTexture[outputPos.xy], invDepthInt);
	}
}
