#define VOXEL_TYPE_ISOTROPIC		1
#define VOXEL_TYPE_ANISOTROPIC		2

struct PSInput
{
	float4 screenSpacePos		: SV_Position;
	float3 worldSpacePos		: POSITION;
	float3 worldSpaceNormal		: NORMAL;
};

#if VOXEL_TYPE == VOXEL_TYPE_ISOTROPIC
	RasterizerOrderedTexture3D<float4> g_VoxelReflectedRadianceTexture : register(u1);
#elif VOXEL_TYPE == VOXEL_TYPE_ANISOTROPIC
	RasterizerOrderedTexture3D<float4> g_VoxelReflectedRadianceTexture[6] : register(u1); 
#endif

float4 MovingAverage(float3 prevValue, float prevCount, float3 newValue)
{
	float newCount = prevCount + 1;
	return float4((prevCount * prevValue + newValue) / newCount, newCount);
}

void Main(PSInput input)
{
	if (any(input.worldSpacePos < g_AppData.voxelGridWorldMinPoint) || any(input.worldSpacePos > g_AppData.voxelGridWorldMaxPoint))
		return;
	
	float3 worldSpaceNormal = normalize(input.worldSpaceNormal);
	
	float3 reflectedRadiance = float3(0.0f, 0.0f, 0.0f);
#if ENABLE_POINT_LIGHTS == 1
	
#endif // ENABLE_POINT_LIGHTS

#if ENABLE_SPOT_LIGHTS == 1
#endif // ENABLE_SPOT_LIGHTS

#if ENABLE_DIRECTIONAL_LIGHT == 1
#endif // ENABLE_DIRECTIONAL_LIGHT	
		
	int3 voxelPos = 

#if VOXEL_TYPE == VOXEL_TYPE_ISOTROPIC
#elif VOXEL_TYPE == VOXEL_TYPE_ANISOTROPIC
#endif
}
