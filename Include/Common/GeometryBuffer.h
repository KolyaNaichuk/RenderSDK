#pragma once

#include "D3DWrapper/GraphicsResource.h"

struct RenderEnv;
class ColorTexture;

class GeometryBuffer
{
public:
	struct ResourceStates
	{
		D3D12_RESOURCE_STATES m_TexCoordTextureState;
		D3D12_RESOURCE_STATES m_NormalTextureState;
		D3D12_RESOURCE_STATES m_MaterialIDTextureState;
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

	ColorTexture* GetTexCoordTexture() { return m_pTexCoordTexture; };
	ColorTexture* GetNormalTexture() { return m_pNormalTexture; };
	ColorTexture* GetMaterialIDTexture() { return m_pMaterialIDTexture; };

	const ResourceStates* GetOutputResourceStates() const { return &m_OutputResourceStates; }
		
private:
	ColorTexture* m_pTexCoordTexture;
	ColorTexture* m_pNormalTexture;
	ColorTexture* m_pMaterialIDTexture;
	ResourceStates m_OutputResourceStates;
};