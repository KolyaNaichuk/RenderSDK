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
				
	float3 worldSpaceTangentFrameX;
	float3 worldSpaceTangentFrameY;
	BuildOrthonormalBasis(worldSpaceNormal, worldSpaceTangentFrameX, worldSpaceTangentFrameY);
	
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
						
			float3 tangentSpaceSampleDir = float3(sinTheta * cosPhi, sinTheta * sinPhi, cosTheta);
			
			float3 worldSpaceSampleDir = tangentSpaceSampleDir.x * worldSpaceTangentFrameX +
				tangentSpaceSampleDir.y * worldSpaceTangentFrameY +
				tangentSpaceSampleDir.z * worldSpaceNormal;

			float3 radiance = radianceMap.SampleLevel(radianceMapSampler, worldSpaceSampleDir, 0.0f);
			irradiance += radiance * (sinTheta * cosTheta);

			++numSamples;
		}
	}

	irradiance *= g_PI * g_PI / float(numSamples);
	return irradiance;
}

void ComputeDiffuseIrradianceSHCoefficients();
