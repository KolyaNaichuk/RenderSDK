#pragma once

#include "D3DWrapper/Common.h"

class GraphicsDevice;

class CommandAllocator
{
public:
	CommandAllocator(GraphicsDevice* pDevice, D3D12_COMMAND_LIST_TYPE type, LPCWSTR pName);
	ID3D12CommandAllocator* GetD3DObject() { return m_D3DCommandAllocator.Get(); }

	void Reset();
	D3D12_COMMAND_LIST_TYPE GetType() const { return m_Type; }

private:
	const D3D12_COMMAND_LIST_TYPE m_Type;
	ComPtr<ID3D12CommandAllocator> m_D3DCommandAllocator;
};
