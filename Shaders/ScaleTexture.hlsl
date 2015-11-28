#ifdef SCALE_UP_2X
#endif // SCALE_UP_2X

#ifdef SCALE_UP_3X
#endif // SCALE_UP_3X

#ifdef SCALE_UP_4X
#endif // SCALE_UP_4X

#ifdef SCALE_DOWN_2X
#endif // SCALE_DOWN_2X

#ifdef SCALE_DOWN_3X
#endif // SCALE_DOWN_3X

#ifdef SCALE_DOWN_4X
#endif // SCALE_DOWN_4X

#ifdef SCALE_AUTO

Texture2D<float4> InputTexture : register(t0);
RWTexture2D<float4> OutputTexture : register(u0);
SamplerState LinearSampler : register(s0);

[numthreads(NUM_THREADS_X, NUM_THREADS_Y, 1)]
void Main(uint3 groupId : SV_GroupID, uint3 groupThreadId : SV_GroupThreadID)
{
	uint2 samplePos = groupId.xy * uint2(NUM_THREADS_X, NUM_THREADS_Y) + groupThreadId.xy;
	
	uint2 textureSize;
	OutputTexture.GetDimensions(textureSize.x, textureSize.y);
	
	if ((samplePos.x < textureSize.x) && (samplePos.y < textureSize.y))
	{
		float2 texCoord = (float2(samplePos.x, samplePos.y) + 0.5f) / float2(textureSize.x, textureSize.y);
		OutputTexture[samplePos] = InputTexture.Sample(LinearSampler, texCoord);
	}
}

#endif // SCALE_AUTO