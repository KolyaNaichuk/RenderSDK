#include "VoxelGrid.hlsl"
#include "BoundingVolumes.hlsl"
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
#endif // ENABLE_POINT_LIGHTS

RWTexture3D<float4> g_IntensityRCoeffsTexture : register(u0);
RWTexture3D<float4> g_IntensityGCoeffsTexture : register(u1);
RWTexture3D<float4> g_IntensityBCoeffsTexture : register(u2);

[numthreads(NUM_THREADS_X, NUM_THREADS_Y, NUM_THREADS_Z)]
void Main(int3 gridCell : SV_DispatchThreadID)
{
	SHSpectralCoeffs accumIntensityCoeffs;
	accumIntensityCoeffs.r = float4(0.0f, 0.0f, 0.0f, 0.0f);
	accumIntensityCoeffs.g = float4(0.0f, 0.0f, 0.0f, 0.0f);
	accumIntensityCoeffs.b = float4(0.0f, 0.0f, 0.0f, 0.0f);

	const int cellIndex = ComputeGridCellIndex(g_GridConfig, gridCell);
	if (g_GridBuffer[cellIndex].numOccluders > 0.0f)
	{
		float3 diffuseAlbedo = g_GridBuffer[cellIndex].diffuseColor;
		float3 worldSpaceNormal = g_GridBuffer[cellIndex].worldSpaceNormal;

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
				float solidAngle = 1.0f;

				float3 worldSpaceNormDirToLight = worldSpaceDirToLight * rcp(distToLight);
				float3 lightIntensity = g_PointLightPropsBuffer[lightIndex].color;
								
				float NdotL = saturate(dot(worldSpaceNormal, worldSpaceNormDirToLight));
				float3 reflectedFlux = diffuseAlbedo * lightIntensity * (solidAngle * NdotL);

				float4 cosineCoeffs = SHProjectClampedCosine(worldSpaceNormal);
				SHSpectralCoeffs intensityCoeffs;
				intensityCoeffs.r = reflectedFlux.r * cosineCoeffs;
				intensityCoeffs.g = reflectedFlux.g * cosineCoeffs;
				intensityCoeffs.b = reflectedFlux.b * cosineCoeffs;

				accumIntensityCoeffs.r += intensityCoeffs.r;
				accumIntensityCoeffs.g += intensityCoeffs.g;
				accumIntensityCoeffs.b += intensityCoeffs.b;
			}
		}
#endif // ENABLE_POINT_LIGHTS
	}

	g_IntensityRCoeffsTexture[gridCell] = accumIntensityCoeffs.r;
	g_IntensityGCoeffsTexture[gridCell] = accumIntensityCoeffs.g;
	g_IntensityBCoeffsTexture[gridCell] = accumIntensityCoeffs.b;
}