#pragma once

#include "Common/Common.h"

struct RenderEnv;
class ColorTexture;

class GeometryBuffer
{
public:
	struct InitParams
	{
		RenderEnv* m_pRenderEnv;
		UINT64 m_BufferWidth;
		UINT m_BufferHeight;
	};

	GeometryBuffer(InitParams* pParams);
	~GeometryBuffer();

	ColorTexture* GetTexCoordTexture() { return m_pTexCoordTexture; };
	ColorTexture* GetNormalTexture() { return m_pNormalTexture; };
	ColorTexture* GetMaterialTexture() { return m_pMaterialTexture; };
		
private:
	ColorTexture* m_pTexCoordTexture;
	ColorTexture* m_pNormalTexture;
	ColorTexture* m_pMaterialTexture;
};