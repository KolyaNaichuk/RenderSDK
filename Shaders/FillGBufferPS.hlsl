struct PSInput
{
	float4 screenSpacePos			: SV_Position;
	float3 worldSpaceNormal			: NORMAL;
	float4 color					: COLOR;
};

struct PSOutput
{
	float4 diffuseColor				: SV_Target0;
	float2 packedWorldSpaceNormal	: SV_Target1;
	float4 specularColor			: SV_Target2;
};

struct Material
{
	float4 ambientColor;
	float4 diffuseColor;
	float4 specularColor;
	float4 emissiveColor;
	float  specularPower;
	float  notUsed[47];
};

cbuffer MaterialBuffer : register(b0)
{
	Material g_Material;
}

#if defined(DIFFUSE_MAP)
Texture2D g_DiffuseMap : register(t0);
#endif

#if defined(NORMAL_MAP)
Texture2D g_NormalMap : register(t1);
#endif

#if defined(SPECULAR_MAP)
Texture2D g_SpecularMap : register(t2);
#endif

#if defined(DIFFUSE_MAP) || defined(NORMAL_MAP) || defined(SPECULAR_MAP)
SamplerState g_AnisoSampler : register(s0);
#endif

PSOutput Main(PSInput input)
{
	PSOutput output;
	
#if defined(DIFFUSE_MAP)
#endif

#if defined(NORMAL_MAP)
#endif

#if defined(SPECULAR_MAP)
#endif

	return output;
}