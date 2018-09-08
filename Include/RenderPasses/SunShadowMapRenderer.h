#pragma once

#include "D3DWrapper/GraphicsResource.h"

struct RenderEnv;
class MeshRenderResources;
class CommandList;

class SunShadowMapRenderer
{
public:
	struct ResourceStates
	{
	};

	struct InitParams
	{
		RenderEnv* m_pRenderEnv;
		ResourceStates m_InputResourceStates;
	};

	struct RenderParams
	{
		RenderEnv* m_pRenderEnv;
		CommandList* m_pCommandList;
	};

private:
};