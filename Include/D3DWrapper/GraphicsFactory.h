#pragma once

#include "D3DWrapper/Common.h"

class GraphicsFactory
{
public:
	GraphicsFactory();
	IDXGIFactory4* GetDXGIObject() { return m_DXGIFactory.Get(); }

private:
	ComPtr<IDXGIFactory4> m_DXGIFactory;
};
