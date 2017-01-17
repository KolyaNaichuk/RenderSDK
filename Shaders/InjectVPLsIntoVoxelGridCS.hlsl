#include "VoxelGrid.hlsl"
#include "Lighting.hlsl"
#include "SphericalHarmonics.hlsl"

cbuffer GridConfigBuffer : register(b0)
{
	GridConfig g_GridConfig;
}

StructuredBuffer<Voxel> g_GridBuffer : register(t0);

#if ENABLE_POINT_LIGHTS == 1
StructuredBuffer<Sphere> g_PointLightBoundsBuffer : register(t1);
StructuredBuffer<PointLightProps> g_PointLightPropsBuffer : register(t2);
Buffer<uint> g_NumPointLightsBuffer : register(t3);
Buffer<uint> g_PointLightIndexBuffer : register(t4);
#endif

RWStructuredBuffer<SHSpectralCoeffs> g_VPLFluxBuffer : register(u0);

[numthreads(NUM_THREADS_X, NUM_THREADS_Y, NUM_THREADS_Z)]
void Main(int3 gridCell : SV_DispatchThreadID)
{
	int cellIndex = ComputeGridCellIndex(g_GridConfig, gridCell);

	float4 colorAndNumOccluders = g_GridBuffer[cellIndex].colorAndNumOccluders;
	if (colorAndNumOccluders.a == 0)
		return;
	
	float3 diffuseAlbedo = colorAndNumOccluders.rgb;

	SHSpectralCoeffs totalFluxCoeffs;
	totalFluxCoeffs.r = SHCoeffs(0.0f, 0.0f, 0.0f, 0.0f);
	totalFluxCoeffs.g = SHCoeffs(0.0f, 0.0f, 0.0f, 0.0f);
	totalFluxCoeffs.b = SHCoeffs(0.0f, 0.0f, 0.0f, 0.0f);

#if ENABLE_POINT_LIGHTS == 1
	for (uint index = 0; index < g_NumPointLightsBuffer[0]; ++index)
	{
		uint lightIndex = g_PointLightIndexBuffer[index];

		float3 worldSpaceLightPos = g_PointLightBoundsBuffer[lightIndex].center;
		float lightAttenEndRange = g_PointLightBoundsBuffer[lightIndex].radius;

		float3 worldSpaceCellPos = ComputeWorldSpacePosition(g_GridConfig, gridCell);
		float3 worldSpaceDirToLight = worldSpaceLightPos - worldSpaceCellPos;
		
		float distToLight = length(worldSpaceDirToLight);
		if (distToLight < lightAttenEndRange)
		{
			float3 worldSpaceNormDirToLight = worldSpaceDirToLight * rcp(distToLight);

			float3 lightColor = g_PointLightPropsBuffer[lightIndex].color;
			float lightAttenStartRange = g_PointLightPropsBuffer[lightIndex].attenStartRange;
			
			float distAtten = CalcLightDistanceAttenuation(distToLight, lightAttenStartRange, lightAttenEndRange);
								
			float NdotL = saturate(dot(worldSpaceNormal, worldSpaceNormDirToLight));
			float3 currFlux = NdotL * diffuseAlbedo * lightColor * distAtten;

			SHCoeffs cosineCoeffs = SHProjectClampedCosine(worldSpaceNormal);

			totalFluxCoeffs.r += currFlux.r * cosineCoeffs;
			totalFluxCoeffs.g += currFlux.g * cosineCoeffs;
			totalFluxCoeffs.b += currFlux.b * cosineCoeffs;
		}
	}
#endif
	
	g_VPLFluxBuffer[cellIndex] = totalFluxCoeffs;
}