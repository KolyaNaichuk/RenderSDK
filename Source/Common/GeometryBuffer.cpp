#include "Common/GeometryBuffer.h"
#include "D3DWrapper/GraphicsResource.h"
#include "D3DWrapper/RenderEnv.h"

GeometryBuffer::GeometryBuffer(InitParams* pParams)
	: m_pTexCoordTexture(nullptr)
	, m_pNormalTexture(nullptr)
	, m_pMaterialTexture(nullptr)
{
	RenderEnv* pRenderEnv = pParams->m_pRenderEnv;
	const FLOAT optimizedClearColor[4] = {0.0f, 0.0f, 0.0f, 0.0f};
		
	ColorTexture2DDesc texCoordTextureDesc(DXGI_FORMAT_R16G16_SNORM, pParams->m_BufferWidth, pParams->m_BufferHeight, true, true, false);
	m_pTexCoordTexture = new ColorTexture(pRenderEnv, pRenderEnv->m_pDefaultHeapProps, &texCoordTextureDesc,
		D3D12_RESOURCE_STATE_RENDER_TARGET, optimizedClearColor, L"m_pTexCoordTexture");

	ColorTexture2DDesc normalTextureDesc(DXGI_FORMAT_R8G8B8A8_SNORM, pParams->m_BufferWidth, pParams->m_BufferHeight, true, true, false);
	m_pNormalTexture = new ColorTexture(pRenderEnv, pRenderEnv->m_pDefaultHeapProps, &normalTextureDesc,
		D3D12_RESOURCE_STATE_RENDER_TARGET, optimizedClearColor, L"m_pNormalTexture");

	ColorTexture2DDesc materialTextureDesc(DXGI_FORMAT_R16_UINT, pParams->m_BufferWidth, pParams->m_BufferHeight, true, true, false);
	m_pMaterialTexture = new ColorTexture(pRenderEnv, pRenderEnv->m_pDefaultHeapProps, &materialTextureDesc,
		D3D12_RESOURCE_STATE_RENDER_TARGET, optimizedClearColor, L"m_pMaterialTexture");
}

GeometryBuffer::~GeometryBuffer()
{
	SafeDelete(m_pTexCoordTexture);
	SafeDelete(m_pNormalTexture);
	SafeDelete(m_pMaterialTexture);
}
