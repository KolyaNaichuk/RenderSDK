#ifndef __ENCODING_UTILS__
#define __ENCODING_UTILS__

void EncodeTextureCoordinateDerivatives(in float2 ddX, in float2 ddY, out float2 derivativesLength, out uint encodedDerivativesRotation)
{
	derivativesLength = float2(length(ddX), length(ddY));

	float2 derivativesRotation = float2(ddX.x / derivativesLength.x, ddY.x / derivativesLength.y) * 0.5f + 0.5f;
	uint signX = (ddX.y < 0.0f) ? 1 : 0;
	uint signY = (ddY.y < 0.0f) ? 1 : 0;

	encodedDerivativesRotation = (signY << 15u) | (uint(derivativesRotation.y * 127.0f) << 8u)
		| (signX << 7u) | uint(derivativesRotation.x * 127.0f);
}

void DecodeTextureCoordinateDerivatives(in float2 derivativesLength, in uint encodedDerivativesRotation, out float2 ddX, out float2 ddY)
{
	float2 derivativesRotation;
	derivativesRotation.x = float(encodedDerivativesRotation & 0x7F) / 127.0f;
	derivativesRotation.y = float((encodedDerivativesRotation >> 8u) & 0x7F) / 127.0f;
	derivativesRotation = derivativesRotation * 2.0f - 1.0f;

	float signX = (((encodedDerivativesRotation >> 7u) & 0x1) == 0) ? 1.0f : -1.0f;
	float signY = (((encodedDerivativesRotation >> 15u) & 0x1) == 0) ? 1.0f : -1.0f;

	ddX.x = derivativesRotation.x;
	ddX.y = signX * sqrt(1.0f - derivativesRotation.x * derivativesRotation.x);
	ddX *= derivativesLength.x;
	
	ddY.x = derivativesRotation.y;
	ddY.y = signY * sqrt(1.0f - derivativesRotation.y * derivativesRotation.y);
	ddY *= derivativesLength.y;
}

#endif // __ENCODING_UTILS__