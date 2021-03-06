#include "D3DWrapper/CommandSignature.h"
#include "D3DWrapper/GraphicsDevice.h"
#include "D3DWrapper/RootSignature.h"

DrawIndexedArguments::DrawIndexedArguments()
	: m_IndexCountPerInstance(0)
	, m_InstanceCount(0)
	, m_StartIndexLocation(0)
	, m_BaseVertexLocation(0)
	, m_StartInstanceLocation(0)
{
}

DrawIndexedArguments::DrawIndexedArguments(UINT indexCountPerInstance, UINT instanceCount,
		UINT startIndexLocation, INT baseVertexLocation, UINT startInstanceLocation)
	: m_IndexCountPerInstance(indexCountPerInstance)
	, m_InstanceCount(instanceCount)
	, m_StartIndexLocation(startIndexLocation)
	, m_BaseVertexLocation(baseVertexLocation)
	, m_StartInstanceLocation(startInstanceLocation)
{
}

DispatchArguments::DispatchArguments()
	: m_ThreadGroupCountX(0)
	, m_ThreadGroupCountY(0)
	, m_ThreadGroupCountZ(0)
{
}

DispatchArguments::DispatchArguments(UINT threadGroupCountX, UINT threadGroupCountY, UINT threadGroupCountZ)
	: m_ThreadGroupCountX(threadGroupCountX)
	, m_ThreadGroupCountY(threadGroupCountY)
	, m_ThreadGroupCountZ(threadGroupCountZ)
{
}

DrawArgument::DrawArgument()
{
	Type = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW;
}

DrawIndexedArgument::DrawIndexedArgument()
{
	Type = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED;
}

DispatchArgument::DispatchArgument()
{
	Type = D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH;
}

Constant32BitArgument::Constant32BitArgument(UINT rootParamIndex, UINT destOffsetIn32BitValues, UINT num32BitValues)
{
	Type = D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT;
	Constant.RootParameterIndex = rootParamIndex;
	Constant.DestOffsetIn32BitValues = destOffsetIn32BitValues;
	Constant.Num32BitValuesToSet = num32BitValues;
}

ConstantBufferViewArgument::ConstantBufferViewArgument(UINT rootParamIndex)
{
	Type = D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT_BUFFER_VIEW;
	ConstantBufferView.RootParameterIndex = rootParamIndex;
}

ShaderResourceViewArgument::ShaderResourceViewArgument(UINT rootParamIndex)
{
	Type = D3D12_INDIRECT_ARGUMENT_TYPE_SHADER_RESOURCE_VIEW;
	ShaderResourceView.RootParameterIndex = rootParamIndex;
}

UnorderedAccessViewArgument::UnorderedAccessViewArgument(UINT rootParamIndex)
{
	Type = D3D12_INDIRECT_ARGUMENT_TYPE_UNORDERED_ACCESS_VIEW;
	UnorderedAccessView.RootParameterIndex = rootParamIndex;
}

CommandSignatureDesc::CommandSignatureDesc(UINT byteStride, UINT numArgumentDescs, const D3D12_INDIRECT_ARGUMENT_DESC* pFirstArgumentDesc)
{
	ByteStride = byteStride;
	NumArgumentDescs = numArgumentDescs;
	pArgumentDescs = pFirstArgumentDesc;
	NodeMask = 0;
}

CommandSignature::CommandSignature(GraphicsDevice* pDevice, RootSignature* pRootSignature, const CommandSignatureDesc* pDesc, LPCWSTR pName)
{
	VerifyD3DResult(pDevice->GetD3DObject()->CreateCommandSignature(
		pDesc, (pRootSignature != nullptr) ? pRootSignature->GetD3DObject() : nullptr,
		IID_PPV_ARGS(&m_D3DCommandSignature)));

#ifdef ENABLE_GRAPHICS_DEBUGGING
	VerifyD3DResult(m_D3DCommandSignature->SetName(pName));
#endif // ENABLE_GRAPHICS_DEBUGGING
}
