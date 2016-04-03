#include "CommandRecorders/TiledShadingRecorder.h"
#include "DX/DXPipelineState.h"
#include "DX/DXRootSignature.h"

TiledShadingRecorder::TiledShadingRecorder(InitParams* pParams)
	: m_pRootSignature(nullptr)
	, m_pPipelineState(nullptr)
	, m_NumThreadGroupsX(pParams->m_NumTilesX)
	, m_NumThreadGroupsY(pParams->m_NumTilesY)
{
	const u16 tileSize = 16;

	std::string tileSizeStr = std::to_string(tileSize);
	std::string shadingModeStr = std::to_string(pParams->m_ShadingMode);
	std::string numPointLightsStr = std::to_string(pParams->m_NumPointLights);
	std::string numSpotLightsStr = std::to_string(pParams->m_NumSpotLights);
	std::string useDirectLightStr = std::to_string(pParams->m_UseDirectLight ? 1 : 0);
	
	const DXShaderMacro shaderDefines[] =
	{
		DXShaderMacro("TILE_SIZE", tileSizeStr.c_str()),
		DXShaderMacro("SHADING_MODE", shadingModeStr.c_str()),
		DXShaderMacro("NUM_POINT_LIGHTS", numPointLightsStr.c_str()),
		DXShaderMacro("NUM_SPOT_LIGHTS", numSpotLightsStr.c_str()),
		DXShaderMacro("USE_DIRECT_LIGHT", useDirectLightStr.c_str()),
		DXShaderMacro()
	};
	DXShader computeShader(L"Shaders//TiledShadingCS.hlsl", "Main", "cs_5_0", shaderDefines);
}

TiledShadingRecorder::~TiledShadingRecorder()
{
	SafeDelete(m_pPipelineState);
	SafeDelete(m_pRootSignature);
}

void TiledShadingRecorder::Record(RenderPassParams* pParams)
{
}
