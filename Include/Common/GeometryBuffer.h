#pragma once

#include "D3DWrapper/GraphicsResource.h"

struct RenderEnv;
class ColorTexture;

class GeometryBuffer
{
public:
	struct ResourceStates
	{
		D3D12_RESOURCE_STATES m_GBuffer1State;
		D3D12_RESOURCE_STATES m_GBuffer2State;
		D3D12_RESOURCE_STATES m_GBuffer3State;
		D3D12_RESOURCE_STATES m_GBuffer4State;
	};

	struct InitParams
	{
		RenderEnv* m_pRenderEnv;
		UINT m_BufferWidth;
		UINT m_BufferHeight;
		ResourceStates m_InputResourceStates;
	};

	GeometryBuffer(InitParams* pParams);
	~GeometryBuffer();

	ColorTexture* GetGBuffer1() { return m_pGBuffer1; };
	ColorTexture* GetGBuffer2() { return m_pGBuffer2; }
	ColorTexture* GetGBuffer3() { return m_pGBuffer3; };
	ColorTexture* GetGBuffer4() { return m_pGBuffer4; };
	
	const ResourceStates* GetOutputResourceStates() const { return &m_OutputResourceStates; }
		
private:
	ColorTexture* m_pGBuffer1 = nullptr;
	ColorTexture* m_pGBuffer2 = nullptr;
	ColorTexture* m_pGBuffer3 = nullptr;
	ColorTexture* m_pGBuffer4 = nullptr;
	ResourceStates m_OutputResourceStates;
};