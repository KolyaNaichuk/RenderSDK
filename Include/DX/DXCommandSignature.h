#pragma once

#include "DX/DXObject.h"

class DXDevice;
class DXRootSignature;

struct DrawMeshCommand
{
	UINT m_Root32BitConstant;
	D3D12_DRAW_INDEXED_ARGUMENTS m_DrawArgs;
};

struct DXDrawArgument : public D3D12_INDIRECT_ARGUMENT_DESC
{
	DXDrawArgument();
};

struct DXDrawIndexedArgument : public D3D12_INDIRECT_ARGUMENT_DESC
{
	DXDrawIndexedArgument();
};

struct DXDispatchArgument : public D3D12_INDIRECT_ARGUMENT_DESC
{
	DXDispatchArgument();
};

struct DX32BitConstantsArgument : public D3D12_INDIRECT_ARGUMENT_DESC
{
	DX32BitConstantsArgument(UINT rootParameterIndex, UINT destOffsetIn32BitValues, UINT num32BitValues);
};

struct DXCBVArgument : public D3D12_INDIRECT_ARGUMENT_DESC
{
	DXCBVArgument(UINT rootParamIndex);
};

struct DXSRVArgument : public D3D12_INDIRECT_ARGUMENT_DESC
{
	DXSRVArgument(UINT rootParamIndex);
};

struct DXUAVArgument : public D3D12_INDIRECT_ARGUMENT_DESC
{
	DXUAVArgument(UINT rootParamIndex);
};

struct DXCommandSignatureDesc : public D3D12_COMMAND_SIGNATURE_DESC
{
	DXCommandSignatureDesc(UINT byteStride, UINT numArguments, const D3D12_INDIRECT_ARGUMENT_DESC* pFirstArgumentDesc);
};

class DXCommandSignature : public DXObject<ID3D12CommandSignature>
{
public:
	DXCommandSignature(DXDevice* pDevice, DXRootSignature* pRootSignature, const DXCommandSignatureDesc* pDesc, LPCWSTR pName);
};