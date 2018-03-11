#include "Foundation.hlsl"
#include "EncodingUtils.hlsl"

struct PSInput
{
	float4 screenSpacePos		: SV_Position;
	float3 worldSpaceNormal		: NORMAL;
	float2 texCoord				: TEXCOORD;
};

struct PSOutput
{
	float2 buffer1				: SV_Target0;
	float2 buffer2				: SV_Target1;
	uint2  buffer3				: SV_Target2;
	float4 buffer4				: SV_Target3;
};

cbuffer MaterialIDBuffer : register(b0)
{
	uint g_MaterialID;
}

PSOutput Main(PSInput input)
{
	float3 worldSpaceNormal = normalize(input.worldSpaceNormal);
	
	float2 ddX = ddx_fine(input.texCoord);
	float2 ddY = ddy_fine(input.texCoord);
				
	float2 derivativesLength;
	uint encodedDerivativesRotation;
	EncodeTextureCoordinateDerivatives(ddX, ddY, derivativesLength, encodedDerivativesRotation);

	PSOutput output;
	output.buffer1 = frac(input.texCoord);
	output.buffer2 = derivativesLength;
	output.buffer3 = uint2(encodedDerivativesRotation, g_MaterialID);
	output.buffer4 = float4(worldSpaceNormal, 0.0f);
	
	return output;
}