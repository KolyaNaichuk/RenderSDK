#include "Foundation.hlsl"
#include "Reconstruction.hlsl"
#include "GammaCorrection.hlsl"

struct PSInput
{
	float4 screenSpacePos	: SV_Position;
	float2 texCoord			: TEXCOORD0;
};

cbuffer AppDataBuffer : register(b0)
{
	AppData g_AppData;
}

Texture2D<float> g_DepthTexture : register(t0);
Texture3D<float4> g_VoxelReflectanceTexture : register(t1);

float4 Main(PSInput input) : SV_Target
{
	float hardwareDepth = g_DepthTexture.Load(int3(input.screenSpacePos.xy, 0)).r;
	float3 worldSpacePos = ComputeWorldSpacePosition(input.texCoord, hardwareDepth, g_AppData.viewProjInvMatrix).xyz;
	
	if (any(worldSpacePos < g_AppData.voxelGridWorldMinPoint) || any(worldSpacePos > g_AppData.voxelGridWorldMaxPoint))
		return float4(0.0f, 0.0f, 0.0f, 1.0f);
		
	float3 gridSpacePos = worldSpacePos - g_AppData.voxelGridWorldMinPoint;
	int3 voxelPos = floor(gridSpacePos * g_AppData.voxelRcpSize);
	float4 reflectedRadiance = g_VoxelReflectanceTexture[voxelPos];
	
	return float4(GammaCorrection(reflectedRadiance.rgb), 1.0f);
}