#pragma once

#include "Common/Common.h"

struct RenderEnv;
class ColorTexture;

class GeometryBuffer
{
public:
	GeometryBuffer(RenderEnv* pRenderEnv, UINT bufferWidth, UINT bufferHeight);
	~GeometryBuffer();

	ColorTexture* GetTexCoordTexture() { return m_pTexCoordTexture; };
	ColorTexture* GetNormalTexture() { return m_pNormalTexture; };
	ColorTexture* GetMaterialIDTexture() { return m_pMaterialIDTexture; };
		
private:
	ColorTexture* m_pTexCoordTexture;
	ColorTexture* m_pNormalTexture;
	ColorTexture* m_pMaterialIDTexture;
};