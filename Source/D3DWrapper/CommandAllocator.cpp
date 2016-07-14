#include "D3DWrapper/GraphicsDevice.h"
#include "D3DWrapper/CommandAllocator.h"

CommandAllocator::CommandAllocator(GraphicsDevice* pDevice, D3D12_COMMAND_LIST_TYPE type, LPCWSTR pName)
	: m_Type(type)
{
	VerifyD3DResult(pDevice->GetD3DObject()->CreateCommandAllocator(type, IID_PPV_ARGS(&m_D3DCommandAllocator)));
#ifdef _DEBUG
	VerifyD3DResult(m_D3DCommandAllocator->SetName(pName));
#endif
}

void CommandAllocator::Reset()
{
	VerifyD3DResult(m_D3DCommandAllocator->Reset());
}
