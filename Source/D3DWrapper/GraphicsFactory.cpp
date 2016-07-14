#include "D3DWrapper/GraphicsFactory.h"

GraphicsFactory::GraphicsFactory()
{
	VerifyD3DResult(CreateDXGIFactory1(IID_PPV_ARGS(&m_DXGIFactory)));
}

