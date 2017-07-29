#pragma once

#include "D3DWrapper/Common.h"

struct RenderEnv;
struct Viewport;
class CommandList;
class RootSignature;
class PipelineState;

class VisualizeTexturePass
{
public:
	enum TextureType
	{
		TextureType_GBufferDiffuse = 1,
		TextureType_GBufferSpecular,
		TextureType_GBufferNormal,
		TextureType_Depth,
		TextureType_Other
	};

	struct InitParams
	{
		RenderEnv* m_pRenderEnv;
		DXGI_FORMAT m_RTVFormat;
		TextureType m_TextureType;
	};

	struct RenderParams
	{
		RenderEnv* m_pRenderEnv;
		CommandList* m_pCommandList;
		Viewport* m_pViewport;
	};

	VisualizeTexturePass(InitParams* pParams);
	~VisualizeTexturePass();

	void Record(RenderParams* pParams);

private:
	RootSignature* m_pRootSignature;
	PipelineState* m_pPipelineState;
};
