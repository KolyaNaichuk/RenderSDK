#pragma once

#include "D3DWrapper/DXObject.h"

class D3DDevice;
class D3DRootSignature;

struct DrawMeshCommand
{
	UINT m_Root32BitConstant;
	D3D12_DRAW_INDEXED_ARGUMENTS m_DrawArgs;
};

struct D3DDrawArgument : public D3D12_INDIRECT_ARGUMENT_DESC
{
	D3DDrawArgument();
};

struct D3DDrawIndexedArgument : public D3D12_INDIRECT_ARGUMENT_DESC
{
	D3DDrawIndexedArgument();
};

struct D3DDispatchArgument : public D3D12_INDIRECT_ARGUMENT_DESC
{
	D3DDispatchArgument();
};

struct D3D32BitConstantsArgument : public D3D12_INDIRECT_ARGUMENT_DESC
{
	D3D32BitConstantsArgument(UINT rootParameterIndex, UINT destOffsetIn32BitValues, UINT num32BitValues);
};

struct D3DCBVArgument : public D3D12_INDIRECT_ARGUMENT_DESC
{
	D3DCBVArgument(UINT rootParamIndex);
};

struct D3DSRVArgument : public D3D12_INDIRECT_ARGUMENT_DESC
{
	D3DSRVArgument(UINT rootParamIndex);
};

struct D3DUAVArgument : public D3D12_INDIRECT_ARGUMENT_DESC
{
	D3DUAVArgument(UINT rootParamIndex);
};

struct D3DCommandSignatureDesc : public D3D12_COMMAND_SIGNATURE_DESC
{
	D3DCommandSignatureDesc(UINT byteStride, UINT numArguments, const D3D12_INDIRECT_ARGUMENT_DESC* pFirstArgumentDesc);
};

class D3DCommandSignature : public DXObject<ID3D12CommandSignature>
{
public:
	D3DCommandSignature(D3DDevice* pDevice, D3DRootSignature* pRootSignature, const D3DCommandSignatureDesc* pDesc, LPCWSTR pName);
};