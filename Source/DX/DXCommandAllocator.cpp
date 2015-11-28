#include "DX/DXDevice.h"
#include "DX/DXCommandAllocator.h"

DXCommandAllocator::DXCommandAllocator(DXDevice* pDevice, D3D12_COMMAND_LIST_TYPE type, LPCWSTR pName)
	: m_type(type)
{
	DXVerify(pDevice->GetDXObject()->CreateCommandAllocator(type, IID_PPV_ARGS(GetDXObjectAddress())));

#ifdef _DEBUG
	SetName(pName);
#endif
}

void DXCommandAllocator::Reset()
{
	DXVerify(GetDXObject()->Reset());
}
