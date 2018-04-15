#include "RenderPasses/GeometryBuffer.h"
#include "D3DWrapper/GraphicsResource.h"
#include "D3DWrapper/RenderEnv.h"

GeometryBuffer::GeometryBuffer(InitParams* pParams)
{
	const FLOAT optimizedClearColor[4] = {0.0f, 0.0f, 0.0f, 0.0f};
	m_OutputResourceStates = pParams->m_InputResourceStates;
		
	ColorTexture2DDesc bufferDesc1(DXGI_FORMAT_R16G16_SNORM, pParams->m_BufferWidth, pParams->m_BufferHeight, true, true, false);
	m_pGBuffer1 = new ColorTexture(pParams->m_pRenderEnv, pParams->m_pRenderEnv->m_pDefaultHeapProps, &bufferDesc1,
		m_OutputResourceStates.m_GBuffer1State, optimizedClearColor, L"GeometryBuffer::m_pGBuffer1");

	ColorTexture2DDesc bufferDesc2(DXGI_FORMAT_R16G16_FLOAT, pParams->m_BufferWidth, pParams->m_BufferHeight, true, true, false);
	m_pGBuffer2 = new ColorTexture(pParams->m_pRenderEnv, pParams->m_pRenderEnv->m_pDefaultHeapProps, &bufferDesc2,
		m_OutputResourceStates.m_GBuffer2State, optimizedClearColor, L"GeometryBuffer::m_pGBuffer2");

	ColorTexture2DDesc bufferDesc3(DXGI_FORMAT_R16G16_UINT, pParams->m_BufferWidth, pParams->m_BufferHeight, true, true, false);
	m_pGBuffer3 = new ColorTexture(pParams->m_pRenderEnv, pParams->m_pRenderEnv->m_pDefaultHeapProps, &bufferDesc3,
		m_OutputResourceStates.m_GBuffer3State, optimizedClearColor, L"GeometryBuffer::m_pGBuffer3");

	ColorTexture2DDesc normalTextureDesc(DXGI_FORMAT_R8G8B8A8_SNORM, pParams->m_BufferWidth, pParams->m_BufferHeight, true, true, false);
	m_pGBuffer4 = new ColorTexture(pParams->m_pRenderEnv, pParams->m_pRenderEnv->m_pDefaultHeapProps, &normalTextureDesc,
		m_OutputResourceStates.m_GBuffer4State, optimizedClearColor, L"GeometryBuffer::m_pGBuffer4");
}

GeometryBuffer::~GeometryBuffer()
{
	SafeDelete(m_pGBuffer1);
	SafeDelete(m_pGBuffer2);
	SafeDelete(m_pGBuffer3);
	SafeDelete(m_pGBuffer4);
}
