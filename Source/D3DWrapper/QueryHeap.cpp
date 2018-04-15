#include "D3DWrapper/QueryHeap.h"
#include "D3DWrapper/GraphicsDevice.h"

QueryHeapDesc::QueryHeapDesc(D3D12_QUERY_HEAP_TYPE type, UINT numQueries)
{
	Type = type;
	Count = numQueries;
	NodeMask = 0;
}

QueryHeap::QueryHeap(GraphicsDevice* pDevice, const QueryHeapDesc* pDesc, LPCWSTR pName)
{
	ID3D12Device* pD3DDevice = pDevice->GetD3DObject();
	VerifyD3DResult(pD3DDevice->CreateQueryHeap(pDesc, IID_PPV_ARGS(&m_D3DQueryHeap)));

#ifdef ENABLE_GRAPHICS_DEBUGGING
	m_D3DQueryHeap->SetName(pName);
#endif // ENABLE_GRAPHICS_DEBUGGING
}
