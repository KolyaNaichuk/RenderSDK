#include "BRDF.hlsl"

float3 CalcDiffuseLightingReference(TextureCube<float4> radianceCubeMap, SamplerState radianceSampler,
	float3 N, float3 diffuseAlbedo, uint numSamples)
{
	float3 reflectedRadiance = 0.0f;

	float3 upVector = abs(N.z) < 0.999f ? float3(0.0f, 0.0f, 1.0f) : float3(1.0f, 0.0f, 0.0f);
	float3 worldBasisX = normalize(cross(upVector, N));
	float3 worldBasisY = cross(N, worldBasisX);

	for (uint sampleIndex = 0; sampleIndex < numSamples; ++sampleIndex)
	{
		float2 E = Hammersley(sampleIndex, numSamples);
		
		float3 L = CosineSampleHemisphere(E);
		L = normalize(L.x * worldBasisX + L.y * worldBasisY + L.z * N);

		float NdotL = saturate(dot(N, L));
		if (NdotL > 0.0f)
		{
			float3 incidentRadiance = radianceCubeMap.Sample(radianceSampler, L, 0.0f).rgb;

			// Lambert diffuse BRDF = diffuseAlbedo / PI
			// PDF = NdotL / PI

			reflectedRadiance += incidentRadiance * diffuseAlbedo;
		}
	}

	reflectedRadiance /= float(numSamples);
	return reflectedRadiance;
}

float3 CalcSpecularLightingReference(TextureCube<float4> radianceCubeMap, SamplerState radianceSampler,
	float3 N, float3 V, float3 f0, float squaredRoughness, uint numSamples)
{
	float3 reflectedRadiance = 0.0f;

	float3 upVector = abs(N.z) < 0.999f ? float3(0.0f, 0.0f, 1.0f) : float3(1.0f, 0.0f, 0.0f);
	float3 worldBasisX = normalize(cross(upVector, N));
	float3 worldBasisY = cross(N, worldBasisX);

	for (uint sampleIndex = 0; sampleIndex < numSamples; ++sampleIndex)
	{
		float2 E = Hammersley(sampleIndex, numSamples);
		
		float3 H = SampleGGX(E, squaredRoughness);
		H = normalize(H.x * worldBasisX + H.y * worldBasisY + H.z * N);

		float3 L = normalize(2.0f * dot(V, H) * H - V);
		
		float NdotV = saturate(dot(N, V));
		float NdotL = saturate(dot(N, L));
		float NdotH = saturate(dot(N, H));
		float LdotH = saturate(dot(L, H));

		if (NdotL > 0.0f)
		{
			float3 incidentRadiance = radianceCubeMap.Sample(radianceSampler, L, 0.0f).rgb;

			float V = V_SmithGGXCorrelated(squaredRoughness, NdotV, NdotL);
			float3 F = F_Schlick(f0, LdotH);
			
			// PDF in half angle space = D * NdotH.
			// Jacobian to convert PDF from half angle to incident light space = 1 / (4 * LdotH).
			// See PBR 3rd edition section Sampling Reflection Functions for Jacobian derivation. 
			// PDF in incident light space = D * NdotH / (4 * LdotH).
			// Cook-Torrance specular BRDF = D * V * F.

			reflectedRadiance += incidentRadiance * F * (V * (4 * LdotH / NdotH) * NdotL);
		}
	}

	reflectedRadiance /= float(numSamples);
	return reflectedRadiance;
}