#include "Common/GeometryBuffer.h"
#include "D3DWrapper/GraphicsResource.h"
#include "D3DWrapper/RenderEnv.h"

GeometryBuffer::GeometryBuffer(RenderEnv* pRenderEnv, UINT64 width, UINT height)
	: m_pTexture0(nullptr)
	, m_pTexture1(nullptr)
	, m_pTexture2(nullptr)
	, m_pTexture3(nullptr)
{
	const FLOAT optimizedClearColor[4] = {0.0f, 0.0f, 0.0f, 0.0f};
	
	// Texture coordinates (fractional part)
	ColorTexture2DDesc textureDesc0(DXGI_FORMAT_R16G16_SNORM, width, height, true, true, false);
	m_pTexture0 = new ColorTexture(pRenderEnv, pRenderEnv->m_pDefaultHeapProps, &textureDesc0, D3D12_RESOURCE_STATE_RENDER_TARGET, optimizedClearColor, L"m_pTexture0");
}

GeometryBuffer::~GeometryBuffer()
{
	SafeDelete(m_pTexture0);
	SafeDelete(m_pTexture1);
	SafeDelete(m_pTexture2);
	SafeDelete(m_pTexture3);
}
