struct PSInput
{
	float4 screenSpacePos		: SV_Position;
	float3 worldSpacePos		: POSITION;
	float3 worldSpaceNormal		: NORMAL;
	float2 texCoord				: TEXCOORD;
};

cbuffer AppDataBuffer : register(b0)
{
	AppData g_AppData;
}

cbuffer MaterialIDBuffer : register(b1)
{
	uint g_MaterialID;
}

RasterizerOrderedTexture3D<float4> g_VoxelReflectenceTexture : register(u1);

#if ENABLE_POINT_LIGHTS == 1
StructuredBuffer<Sphere> g_PointLightBoundsBuffer : register(t0);
StructuredBuffer<PointLightProps> g_PointLightPropsBuffer : register(t1);
Buffer<uint> g_NumPointLightsBuffer : register(t2);
Buffer<uint> g_PointLightIndexBuffer : register(t3);
#endif // ENABLE_POINT_LIGHTS

#if ENABLE_SPOT_LIGHTS == 1
StructuredBuffer<Sphere> g_SpotLightWorldBoundsBuffer : register(t4);
StructuredBuffer<SpotLightProps> g_SpotLightPropsBuffer : register(t5);
Buffer<uint> g_NumSpotLightsBuffer : register(t6);
Buffer<uint> g_SpotLightIndexBuffer : register(t7);
#endif // ENABLE_SPOT_LIGHTS

float4 MovingAverage(float3 prevValue, float prevCount, float3 newValue)
{
	float newCount = prevCount + 1;
	return float4((prevCount * prevValue + newValue) / newCount, newCount);
}

#if ENABLE_POINT_LIGHTS == 1
float3 ComputeReflectedRadianceFromPointLights()
{
	return float3(0.0f, 0.0f, 0.0f);
}
#endif // ENABLE_POINT_LIGHTS

#if ENABLE_SPOT_LIGHTS == 1
float3 ComputeReflectedRadianceFromSpotLights()
{
	return float3(0.0f, 0.0f, 0.0f);
}
#endif // ENABLE_SPOT_LIGHTS

#if ENABLE_DIRECTIONAL_LIGHT == 1
float3 ComputeReflectedRadianceFromDirectionalLight()
{
	return float3(0.0f, 0.0f, 0.0f);
}
#endif // ENABLE_DIRECTIONAL_LIGHT

void Main(PSInput input)
{
	float3 worldSpacePos = input.worldSpacePos;
	if (any(worldSpacePos < g_AppData.voxelGridWorldMinPoint) || any(worldSpacePos > g_AppData.voxelGridWorldMaxPoint))
		return;

	float3 worldSpaceNormal = normalize(input.worldSpaceNormal);
	
	float3 reflectedRadiance = float3(0.0f, 0.0f, 0.0f);
#if ENABLE_POINT_LIGHTS == 1
	reflectedRadiance += ComputeReflectedRadianceFromPointLights();
#endif // ENABLE_POINT_LIGHTS

#if ENABLE_SPOT_LIGHTS == 1
	reflectedRadiance += ComputeReflectedRadianceFromSpotLights();
#endif // ENABLE_SPOT_LIGHTS

#if ENABLE_DIRECTIONAL_LIGHT == 1
	reflectedRadiance += ComputeReflectedRadianceFromDirectionalLight();
#endif // ENABLE_DIRECTIONAL_LIGHT	
	
	float3 gridSpacePos = worldSpacePos - g_AppData.voxelGridWorldMinPoint;
	int3 voxelPos = floor(gridSpacePos * g_AppData.voxelRcpSize);
	
	float4 prevReflectedRadiance = g_VoxelReflectenceTexture[voxelPos];
	g_VoxelReflectenceTexture[voxelPos] = MovingAverage(prevReflectedRadiance.rgb, prevReflectedRadiance.a, reflectedRadiance);
}
