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
	float3 N, float3 V, float3 f0, float roughness, uint numSamples)
{
	float3 reflectedRadiance = 0.0f;

	float3 upVector = abs(N.z) < 0.999f ? float3(0.0f, 0.0f, 1.0f) : float3(1.0f, 0.0f, 0.0f);
	float3 worldBasisX = normalize(cross(upVector, N));
	float3 worldBasisY = cross(N, worldBasisX);

	float squaredRoughness = roughness * roughness;
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

			float Vis = V_SmithGGXCorrelated(squaredRoughness, NdotV, NdotL);
			float3 F = F_Schlick(f0, LdotH);

			// PDF in half angle space = D * NdotH.
			// Jacobian to convert PDF from half angle to incident light space = 1 / (4 * LdotH).
			// See PBR 3rd edition section Sampling Reflection Functions for Jacobian derivation. 
			// PDF in incident light space = D * NdotH / (4 * LdotH).
			// Cook-Torrance specular BRDF = D * Vis * F.

			reflectedRadiance += incidentRadiance * F * (Vis * (4 * LdotH / NdotH) * NdotL);
		}
	}

	reflectedRadiance /= float(numSamples);
	return reflectedRadiance;
}

float3 IntegrateLD(TextureCube<float4> radianceCubeMap, SamplerState radianceSampler,
	float cubeMapFaceSize, float numMipLevels, float3 R, float roughness, uint numSamples)
{
	// See section 4.9.2 "Light probe filtering"
	// in paper "Moving Frostbite to PBR" for DFG term derivation.

	// We integrate based on the reflected direction R, where R = reflect(-V, N).
	// So for every direction R, we store a preintegrated value in the cube map.
	// Different mip levels of the cube map correspond to different roughness values.
	// To simplify the integral, we assume N is equal to V, which means that R is equal to V as well.

	float3 N = R;
	float3 V = R;
	
	float3 LD = 0.0f;
	float totalWeight = 0.0f;

	float3 upVector = abs(N.z) < 0.999f ? float3(0.0f, 0.0f, 1.0f) : float3(1.0f, 0.0f, 0.0f);
	float3 worldBasisX = normalize(cross(upVector, N));
	float3 worldBasisY = cross(N, worldBasisX);

	float squaredRoughness = roughness * roughness;

	// Crude approximation of the pixel solid area.
	// Assumed to be the same for all the sample directions.
	// Need to incorrporate Jacobian for transforming from sphere to cube map space to be correct. 
	
	float pixelSolidAngle = g_4PI / (6.0f * cubeMapFaceSize * cubeMapFaceSize);
	
	for (uint sampleIndex = 0; sampleIndex < numSamples; ++sampleIndex)
	{
		float2 E = Hammersley(sampleIndex, numSamples);

		float3 H = SampleGGX(E, squaredRoughness);
		H = normalize(H.x * worldBasisX + H.y * worldBasisY + H.z * N);

		float3 L = normalize(2.0f * dot(V, H) * H - V);

		float NdotL = saturate(dot(N, L));
		float NdotH = saturate(dot(N, H));

		if (NdotL > 0.0f)
		{
			// Prefiltering importance sampling.
			// See chapter 20. GPU-Based importance sampling from GPU Gems 3.

			// PDF in half angle space = D * NdotH.
			// Jacobian to convert PDF from half angle to incident light space = 1 / (4 * LdotH).
			// See PBR 3rd edition section Sampling Reflection Functions for Jacobian derivation. 
			// PDF in incident light space = D * NdotH / (4 * LdotH).
			// Since V = N, then VdotH = LdotH = NdotH and PDF = D / 4.

			float PDF = 0.25f * D_GGX(squaredRoughness, NdotH);
			float sampleSolidAngle = 1.0f / (float(numSamples) * PDF);

			float mipLevel = clamp(0.5f * log2(sampleSolidAngle / pixelSolidAngle), 0.0f, numMipLevels);
			float3 incidentRadiance = radianceCubeMap.Sample(radianceSampler, L, mipLevel).rgb;

			LD += incidentRadiance * NdotL;
			totalWeight += NdotL;
		}
	}

	LD /= max(totalWeight, 0.000001f);
	return LD;
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
			float Vis = V_SmithGGXCorrelated(squaredRoughness, NdotV, NdotL);
			float GVis = (4.0f * Vis * NdotL * VdotH) / NdotH;

			float Fc = pow(1.0f - VdotH, 5.0f);

			DFG.x += (1.0f - Fc) * GVis;
			DFG.y += Fc * GVis;
		}
	}

	DFG /= float(numSamples);
	return DFG;
}
