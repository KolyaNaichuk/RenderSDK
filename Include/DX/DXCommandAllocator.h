#pragma once

#include "DXObject.h"

class DXDevice;

class DXCommandAllocator : public DXObject<ID3D12CommandAllocator>
{
public:
	DXCommandAllocator(DXDevice* pDevice, D3D12_COMMAND_LIST_TYPE type, LPCWSTR pName);

	D3D12_COMMAND_LIST_TYPE GetType() const { return m_type; }
	void Reset();

private:
	D3D12_COMMAND_LIST_TYPE m_type;
};