#pragma once

#include "D3DWrapper/Common.h"

class CommandList;
class RootSignature;
class PipelineState;

struct RenderEnv;
struct BindingResourceList;
struct Viewport;

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
		BindingResourceList* m_pResources;
		Viewport* m_pViewport;
	};

	VisualizeTexturePass(InitParams* pParams);
	~VisualizeTexturePass();

	void Record(RenderParams* pParams);

private:
	RootSignature* m_pRootSignature;
	PipelineState* m_pPipelineState;
};
