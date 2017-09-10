#include "Common/GeometryBuffer.h"
#include "D3DWrapper/GraphicsResource.h"
#include "D3DWrapper/RenderEnv.h"

GeometryBuffer::GeometryBuffer(RenderEnv* pRenderEnv, UINT bufferWidth, UINT bufferHeight)
	: m_pTexCoordTexture(nullptr)
	, m_pNormalTexture(nullptr)
	, m_pMaterialIDTexture(nullptr)
{
	const FLOAT optimizedClearColor[4] = {0.0f, 0.0f, 0.0f, 0.0f};
		
	ColorTexture2DDesc texCoordTextureDesc(DXGI_FORMAT_R16G16_SNORM, bufferWidth, bufferHeight, true, true, false);
	m_pTexCoordTexture = new ColorTexture(pRenderEnv, pRenderEnv->m_pDefaultHeapProps, &texCoordTextureDesc,
		D3D12_RESOURCE_STATE_RENDER_TARGET, optimizedClearColor, L"m_pTexCoordTexture");

	ColorTexture2DDesc normalTextureDesc(DXGI_FORMAT_R8G8B8A8_SNORM, bufferWidth, bufferHeight, true, true, false);
	m_pNormalTexture = new ColorTexture(pRenderEnv, pRenderEnv->m_pDefaultHeapProps, &normalTextureDesc,
		D3D12_RESOURCE_STATE_RENDER_TARGET, optimizedClearColor, L"m_pNormalTexture");

	ColorTexture2DDesc materialIDTextureDesc(DXGI_FORMAT_R16_UINT, bufferWidth, bufferHeight, true, true, false);
	m_pMaterialIDTexture = new ColorTexture(pRenderEnv, pRenderEnv->m_pDefaultHeapProps, &materialIDTextureDesc,
		D3D12_RESOURCE_STATE_RENDER_TARGET, optimizedClearColor, L"m_pMaterialIDTexture");
}

GeometryBuffer::~GeometryBuffer()
{
	SafeDelete(m_pTexCoordTexture);
	SafeDelete(m_pNormalTexture);
	SafeDelete(m_pMaterialIDTexture);
}
