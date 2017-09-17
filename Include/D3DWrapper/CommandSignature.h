#pragma once

#include "D3DWrapper/Common.h"

class GraphicsDevice;
class RootSignature;

struct DrawIndexedArguments
{
	DrawIndexedArguments(UINT indexCountPerInstance, UINT instanceCount,
		UINT startIndexLocation, INT baseVertexLocation, UINT startInstanceLocation);

	UINT m_IndexCountPerInstance;
	UINT m_InstanceCount;
	UINT m_StartIndexLocation;
	INT m_BaseVertexLocation;
	UINT m_StartInstanceLocation;
};

struct DispatchArguments
{
	DispatchArguments(UINT threadGroupCountX, UINT threadGroupCountY, UINT threadGroupCountZ);

	UINT m_ThreadGroupCountX;
	UINT m_ThreadGroupCountY;
	UINT m_ThreadGroupCountZ;
};

struct DrawArgument : public D3D12_INDIRECT_ARGUMENT_DESC
{
	DrawArgument();
};

struct DrawIndexedArgument : public D3D12_INDIRECT_ARGUMENT_DESC
{
	DrawIndexedArgument();
};

struct DispatchArgument : public D3D12_INDIRECT_ARGUMENT_DESC
{
	DispatchArgument();
};

struct Constant32BitArgument : public D3D12_INDIRECT_ARGUMENT_DESC
{
	Constant32BitArgument(UINT rootParamIndex, UINT destOffsetIn32BitValues, UINT num32BitValues);
};

struct ConstantBufferViewArgument : public D3D12_INDIRECT_ARGUMENT_DESC
{
	ConstantBufferViewArgument(UINT rootParamIndex);
};

struct ShaderResourceViewArgument : public D3D12_INDIRECT_ARGUMENT_DESC
{
	ShaderResourceViewArgument(UINT rootParamIndex);
};

struct UnorderedAccessViewArgument : public D3D12_INDIRECT_ARGUMENT_DESC
{
	UnorderedAccessViewArgument(UINT rootParamIndex);
};

struct CommandSignatureDesc : public D3D12_COMMAND_SIGNATURE_DESC
{
	CommandSignatureDesc(UINT byteStride, UINT numArguments, const D3D12_INDIRECT_ARGUMENT_DESC* pFirstArgumentDesc);
};

class CommandSignature
{
public:
	CommandSignature(GraphicsDevice* pDevice, RootSignature* pRootSignature, const CommandSignatureDesc* pDesc, LPCWSTR pName);
	ID3D12CommandSignature* GetD3DObject() { return m_D3DCommandSignature.Get(); }

private:
	ComPtr<ID3D12CommandSignature> m_D3DCommandSignature;
};