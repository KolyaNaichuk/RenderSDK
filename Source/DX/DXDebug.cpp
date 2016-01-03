#include "DX/DXDebug.h"

DXDebug::DXDebug()
{
	DXVerify(D3D12GetDebugInterface(IID_PPV_ARGS(GetDXObjectAddress())));
}

void DXDebug::EnableDebugLayer()
{
	GetDXObject()->EnableDebugLayer();
	
	ComPtr<IDXGIDebug1> dxgiDebug;
	DXVerify(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiDebug)));
	DXVerify(dxgiDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL));
}
