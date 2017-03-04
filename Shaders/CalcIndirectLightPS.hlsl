#include "Reconstruction.hlsl"
#include "Foundation.hlsl"
#include "VoxelGrid.hlsl"
#include "SphericalHarmonics.hlsl"

struct PSInput
{
	float4 screenSpacePos	: SV_Position;
	float2 texCoord			: TEXCOORD0;
};

cbuffer GridConfigBuffer : register(b0)
{
	GridConfig g_GridConfig;
}

cbuffer CameraTransformBuffer : register(b1)
{
	CameraTransform g_Transform;
}

Texture2D g_DepthTexture : register(t0);
Texture2D g_NormalTexture : register(t1);
Texture2D g_DiffuseTexture : register(t2);
Texture3D g_IntensityRCoeffsTexture : register(t3);
Texture3D g_IntensityGCoeffsTexture : register(t4);
Texture3D g_IntensityBCoeffsTexture : register(t5);

SamplerState g_LinearSampler : register(s0);

float4 Main(PSInput input) : SV_Target
{
	uint2 screenSpacePos = uint2(input.screenSpacePos.xy);
	
	float hardwareDepth = g_DepthTexture[screenSpacePos].x;
	float4 worldSpacePos = ComputeWorldSpacePosition(input.texCoord, hardwareDepth, g_Transform.viewProjInvMatrix);
	float3 worldSpaceNormal = g_NormalTexture[screenSpacePos].xyz;
	float3 diffuseAlbedo = g_DiffuseTexture[screenSpacePos].rgb;

	float3 gridSpacePos = worldSpacePos - g_GridConfig.worldSpaceOrigin.xyz;
	float3 gridTexCoord = gridSpacePos * g_GridConfig.rcpSize.xyz;
	
	SHSpectralCoeffs incidentIntensityCoeffs;
	incidentIntensityCoeffs.r = g_IntensityRCoeffsTexture.Sample(g_LinearSampler, gridTexCoord);
	incidentIntensityCoeffs.g = g_IntensityGCoeffsTexture.Sample(g_LinearSampler, gridTexCoord);
	incidentIntensityCoeffs.b = g_IntensityBCoeffsTexture.Sample(g_LinearSampler, gridTexCoord);
	
	float4 cosineCoeffs = SHProjectClampedCosine(worldSpaceNormal);
	float3 incidentIrradiance;
	incidentIrradiance.r = dot(incidentIntensityCoeffs.r, cosineCoeffs);
	incidentIrradiance.g = dot(incidentIntensityCoeffs.g, cosineCoeffs);
	incidentIrradiance.b = dot(incidentIntensityCoeffs.b, cosineCoeffs);
	incidentIrradiance = max(incidentIrradiance, 0.0f);
	
	float3 diffuseBRDF = diffuseAlbedo * g_RcpPI;
	float3 exitantRadiance = diffuseBRDF * incidentIrradiance;

	return float4(exitantRadiance, 1.0f);
}