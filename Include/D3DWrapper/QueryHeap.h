#pragma once

#include "D3DWrapper/Common.h"

class GraphicsDevice;

struct QueryHeapDesc : public D3D12_QUERY_HEAP_DESC
{
	QueryHeapDesc(D3D12_QUERY_HEAP_TYPE type, UINT numQueries);
};

class QueryHeap
{
public:
	QueryHeap(GraphicsDevice* pDevice, const QueryHeapDesc* pDesc, LPCWSTR pName);
	ID3D12QueryHeap* GetD3DObject() { return m_D3DQueryHeap.Get(); }

private:
	ComPtr<ID3D12QueryHeap> m_D3DQueryHeap;
};