#include "Foundation.hlsl"

TextureCube<float3> g_DiffuseRadianceMap;
TextureCube<float3> g_DiffuseIrradianceMap;
SamplerState g_DiffuseRandianceMapSampler;

struct DiffuseIrradianceSHProbe
{
	float3 L[9];
};

void RenderDiffuseRadianceMap();

float3 ComputeDiffuseIrradiance(TextureCube<float3> radianceMap, SamplerState radianceMapSampler, float3 worldSpaceNormal)
{
	// To estimate incoming irradiance to the surface point, we use Monte Carlo integration with uniformly distributed samples.
	
	static const float step = 0.025f;
	float3 irradiance = float3(0.0f, 0.0f, 0.0f);
	
	float3 worldSpaceFrameX;
	float3 worldSpaceFrameZ;
	BuildOrthonormalBasis(worldSpaceNormal, worldSpaceFrameX, worldSpaceFrameZ);
	
	uint numSamples = 0;
	for (float phi = 0.0f; phi < g_2PI; phi += step)
	{
		float sinPhi;
		float cosPhi;
		sincos(phi, sinPhi, cosPhi);
						
		for (float theta = 0.0f; theta < g_PIDIV2; theta += step)
		{
			float sinTheta;
			float cosTheta;
			sincos(theta, sinTheta, cosTheta);
						
			float3 localSpaceDir = float3(cosPhi * sinTheta, cosTheta, sinPhi * sinTheta);
			float3 worldSpaceDir = localSpaceDir.x * worldSpaceFrameX + localSpaceDir.y * worldSpaceNormal + localSpaceDir.z * worldSpaceFrameZ;

			float3 radiance = radianceMap.SampleLevel(radianceMapSampler, worldSpaceDir, 0.0f);
			irradiance += radiance * (sinTheta * cosTheta);

			++numSamples;
		}
	}

	irradiance *= g_PI * g_PI / float(numSamples);
	return irradiance;
}

void ComputeSHCoefficientsForDiffuseIrradiance();
