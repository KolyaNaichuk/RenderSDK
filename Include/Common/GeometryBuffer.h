#pragma once

#include "Common/Common.h"

struct RenderEnv;
class ColorTexture;

class GeometryBuffer
{
public:
	GeometryBuffer(RenderEnv* pRenderEnv, UINT64 width, UINT height);
	~GeometryBuffer();

	ColorTexture* GetTexture0() { return m_pTexture0; };
	ColorTexture* GetTexture1() { return m_pTexture1; };
	ColorTexture* GetTexture2() { return m_pTexture2; };
	ColorTexture* GetTexture3() { return m_pTexture3; };

private:
	ColorTexture* m_pTexture0;
	ColorTexture* m_pTexture1;
	ColorTexture* m_pTexture2;
	ColorTexture* m_pTexture3;
};