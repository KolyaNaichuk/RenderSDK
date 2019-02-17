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

float2 IntegrateDFG(float roughness, float NdotV, uint numSamples)
{
	// See section 4.9.2 "Light probe filtering"
	// in paper "Moving Frostbite to PBR" for DFG term derivation.

	// It does not matter in which coordinate space we perform the integration.
	// All we care about is just NdotV represents the cosine of the angle between N and V.
	// For the sake of convenience, we choose N as (0, 0, 1), that is positive Z.
	// This also means that sampled H from GGX is already in this space.

	float2 DFG = 0.0f;

	// The integral is symmetrical with respect to N.
	// We can go with any V that satisfies NdotV condition.
	// Let us take V that lies in XZ plane.
	
	float3 V;
	V.x = sqrt(1.0f - NdotV * NdotV);
	V.y = 0.0f;
	V.z = NdotV;

	float squaredRoughness = roughness * roughness;
	for (uint sampleIndex = 0; sampleIndex < numSamples; ++sampleIndex)
	{
		float2 E = Hammersley(sampleIndex, numSamples);
		float3 H = SampleGGX(E, squaredRoughness);
		float3 L = normalize(2.0f * dot(V, H) * H - V);

		float NdotL = saturate(L.z);
		float NdotH = saturate(H.z);
		float VdotH = saturate(dot(V, H));

		if (NdotL > 0.0f)
		{
			float V = V_SmithGGXCorrelated(squaredRoughness, NdotV, NdotL);

			float GVis = (4.0f * V * NdotL * VdotH) / NdotH;
			float Fc = pow(1.0f - VdotH, 5.0f);

			DFG.x += (1.0f - Fc) * GVis;
			DFG.y += Fc * GVis;
		}
	}

	DFG /= float(numSamples);
	return DFG;
}