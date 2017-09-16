#include "Common/GeometryBuffer.h"
#include "D3DWrapper/GraphicsResource.h"
#include "D3DWrapper/RenderEnv.h"

GeometryBuffer::GeometryBuffer(InitParams* pParams)
	: m_pTexCoordTexture(nullptr)
	, m_pNormalTexture(nullptr)
	, m_pMaterialIDTexture(nullptr)
{
	const FLOAT optimizedClearColor[4] = {0.0f, 0.0f, 0.0f, 0.0f};
	m_OutputResourceStates = pParams->m_InputResourceStates;
		
	ColorTexture2DDesc texCoordTextureDesc(DXGI_FORMAT_R16G16_SNORM, pParams->m_BufferWidth, pParams->m_BufferHeight, true, true, false);
	m_pTexCoordTexture = new ColorTexture(pParams->m_pRenderEnv, pParams->m_pRenderEnv->m_pDefaultHeapProps, &texCoordTextureDesc,
		m_OutputResourceStates.m_TexCoordTextureState, optimizedClearColor, L"m_pTexCoordTexture");

	ColorTexture2DDesc normalTextureDesc(DXGI_FORMAT_R8G8B8A8_SNORM, pParams->m_BufferWidth, pParams->m_BufferHeight, true, true, false);
	m_pNormalTexture = new ColorTexture(pParams->m_pRenderEnv, pParams->m_pRenderEnv->m_pDefaultHeapProps, &normalTextureDesc,
		m_OutputResourceStates.m_NormalTextureState, optimizedClearColor, L"m_pNormalTexture");

	ColorTexture2DDesc materialIDTextureDesc(DXGI_FORMAT_R16_UINT, pParams->m_BufferWidth, pParams->m_BufferHeight, true, true, false);
	m_pMaterialIDTexture = new ColorTexture(pParams->m_pRenderEnv, pParams->m_pRenderEnv->m_pDefaultHeapProps, &materialIDTextureDesc,
		m_OutputResourceStates.m_MaterialIDTextureState, optimizedClearColor, L"m_pMaterialIDTexture");
}

GeometryBuffer::~GeometryBuffer()
{
	SafeDelete(m_pTexCoordTexture);
	SafeDelete(m_pNormalTexture);
	SafeDelete(m_pMaterialIDTexture);
}
