struct PSInput
{
	float4 screenSpacePos	: SV_Position;
	float2 texCoord			: TEXCOORD0;
};

struct CameraSettings
{
	float projMatrix32;
	float projMatrix22;
	float viewNearPlaneDist;
	float rcpViewClipRange;
};

ConstantBuffer<CameraSettings> g_CameraSettings : register(b0);
Texture2D g_Texture	: register(t0);
SamplerState g_Sampler	: register(s0);

float4 Main(PSInput input) : SV_Target
{
	float hardwareDepth = g_Texture.Sample(g_Sampler, input.texCoord).r;
	float viewSpaceDepth = g_CameraSettings.projMatrix32 / (hardwareDepth - g_CameraSettings.projMatrix22);
	float linearDepth = (viewSpaceDepth - g_CameraSettings.viewNearPlaneDist) * g_CameraSettings.rcpViewClipRange;
	
	return float4(linearDepth.rrr, 1.0f);
}