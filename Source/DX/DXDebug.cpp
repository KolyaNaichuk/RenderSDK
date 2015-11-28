#include "DX/DXDebug.h"

DXDebug::DXDebug()
{
	DXVerify(D3D12GetDebugInterface(IID_PPV_ARGS(GetDXObjectAddress())));
}

void DXDebug::EnableDebugLayer()
{
	GetDXObject()->EnableDebugLayer();
}
