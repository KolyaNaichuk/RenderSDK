#include "Reconstruction.hlsl"
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
Texture3D g_FluxRCoeffsTexture : register(t3);
Texture3D g_FluxGCoeffsTexture : register(t4);
Texture3D g_FluxBCoeffsTexture : register(t5);

float4 Main(PSInput input) : SV_Target
{
	uint2 screenSpacePos = uint2(input.screenSpacePos.xy);
	
	float hardwareDepth = DepthTexture[screenSpacePos].x;
	float4 worldSpacePos = ComputeWorldSpacePosition(input.texCoord, hardwareDepth, g_Transform.viewProjInvMatrix);
	float3 worldSpaceNormal = g_NormalTexture[screenSpacePos].xyz;
	float3 diffuseAlbedo = g_DiffuseTexture[screenSpacePos].rgb;

	int3 gridCell = ComputeGridCell(g_GridConfig, worldSpacePos.xyz);
	
	SHSpectralCoeffs incidentFluxCoeffs;
	incidentFluxCoeffs.r = g_FluxRCoeffsTexture[gridCell];
	incidentFluxCoeffs.g = g_FluxGCoeffsTexture[gridCell];
	incidentFluxCoeffs.b = g_FluxBCoeffsTexture[gridCell];
	
	float4 cosineCoeffs = SHProjectClampedCosine(-worldSpaceNormal);

	float3 incidentFlux;
	incidentFlux.r = dot(incidentFluxCoeffs.r, cosineCoeffs);
	incidentFlux.g = dot(incidentFluxCoeffs.g, cosineCoeffs);
	incidentFlux.b = dot(incidentFluxCoeffs.b, cosineCoeffs);
	incidentFlux = max(incidentFlux, 0.0f);
	
	float3 diffuseBRDF = diffuseAlbedo * g_RcpPI;
	float3 reflectedIntensity = incidentFlux * diffuseBRDF;

	return float4(reflectedIntensity, 1.0f);
}