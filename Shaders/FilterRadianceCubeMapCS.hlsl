Texture2DArray<float4> g_InputCubeMap : register(t0);

RWTexture2DArray<float4> g_OutputCubeMapMip1 : register(u0);
RWTexture2DArray<float4> g_OutputCubeMapMip2 : register(u1);
RWTexture2DArray<float4> g_OutputCubeMapMip3 : register(u2);
RWTexture2DArray<float4> g_OutputCubeMapMip4 : register(u3);

SamplerState g_BilinearSampler : register(s0);

cbuffer FilterCubeMapDataBuffer : register(b0)
{
	float g_RcpOutputMip1Size;
	uint g_InputMipLevel;
	uint g_NumMipLevels;
};

// Unvectorize float3 to minimise memory bank conflicts.
groupshared float3 g_SharedMem[64];

[numthreads(8, 8, 1)]
void Main(uint3 globalId : SV_DispatchThreadID, uint localIndex : SV_GroupIndex)
{
	float3 inputMipPos = float3((float2(globalId.xy) + 0.5f) * g_RcpOutputMip1Size, globalThreadId.z);
	float3 radiance1 = g_InputCubeMap.SampleLevel(g_BilinearSampler, inputMipPos, g_InputMipLevel).rgb;
	
	g_OutputCubeMapMip1[globalId] = radiance1;
	if (g_NumMipLevels == 1)
		return;

	g_SharedMem[localIndex] = radiance1;
	GroupMemoryBarrierWithGroupSync();

	// globalId.x and globalId.y are multiples of 2
	if ((localIndex & 0x9) == 0)
	{
		float3 radiance2 = g_SharedMem[localIndex + 1];
		float3 radiance3 = g_SharedMem[localIndex + 8];
		float3 radiance4 = g_SharedMem[localIndex + 9];

		radiance1 = 0.25f * (radiance1 + radiance2 + radiance3 + radiance4);
		
		g_OutputCubeMapMip2[uint3(globalId.xy >> 1, globalId.z)] = radiance1;
		g_SharedMem[localIndex] = radiance1;
	}

	if (g_NumMipLevels == 2)
		return;
	GroupMemoryBarrierWithGroupSync();

	// globalId.x and globalId.y are multiples of 4
	if ((localIndex & 0x1B) == 0)
	{
		float3 radiance2 = g_SharedMem[localIndex + 2];
		float3 radiance3 = g_SharedMem[localIndex + 16];
		float3 radiance4 = g_SharedMem[localIndex + 18];

		radiance1 = 0.25f * (radiance1 + radiance2 + radiance3 + radiance4);

		g_OutputCubeMapMip3[uint3(globalId.xy >> 2, globalId.z)] = radiance1;
		g_SharedMem[localIndex] = radiance2;
	}

	if (g_NumMipLevels == 3)
		return;
	GroupMemoryBarrierWithGroupSync();

	// globalId.x and globalId.y are multiples of 8 but only one thread satisfies that criteria
	if (localIndex == 0)
	{
		float3 radiance2 = g_SharedMem[localIndex + 4];
		float3 radiance3 = g_SharedMem[localIndex + 32];
		float3 radiance4 = g_SharedMem[localIndex + 36];
		
		radiance1 = 0.25f * (radiance1 + radiance2 + radiance3 + radiance4);
		g_OutputCubeMapMip4[uint3(globalId.xy >> 3, globalId.z)] = radiance1;
	}
}