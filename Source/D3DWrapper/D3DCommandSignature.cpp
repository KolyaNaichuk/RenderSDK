#include "D3DWrapper/D3DCommandSignature.h"
#include "D3DWrapper/D3DDevice.h"
#include "D3DWrapper/D3DRootSignature.h"

D3DCommandSignature::D3DCommandSignature(D3DDevice* pDevice, D3DRootSignature* pRootSignature, const D3DCommandSignatureDesc* pDesc, LPCWSTR pName)
{
	DXVerify(pDevice->GetDXObject()->CreateCommandSignature(
		pDesc, (pRootSignature != nullptr) ? pRootSignature->GetDXObject() : nullptr,
		IID_PPV_ARGS(GetDXObjectAddress())));

#ifdef _DEBUG
	SetName(pName);
#endif
}

D3DDrawArgument::D3DDrawArgument()
{
	Type = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW;
}

D3DDrawIndexedArgument::D3DDrawIndexedArgument()
{
	Type = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED;
}

D3DDispatchArgument::D3DDispatchArgument()
{
	Type = D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH;
}

D3D32BitConstantsArgument::D3D32BitConstantsArgument(UINT rootParameterIndex, UINT destOffsetIn32BitValues, UINT num32BitValues)
{
	Type = D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT;
	Constant.RootParameterIndex = rootParameterIndex;
	Constant.DestOffsetIn32BitValues = destOffsetIn32BitValues;
	Constant.Num32BitValuesToSet = num32BitValues;
}

D3DCBVArgument::D3DCBVArgument(UINT rootParamIndex)
{
	Type = D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT_BUFFER_VIEW;
	ConstantBufferView.RootParameterIndex = rootParamIndex;
}

D3DSRVArgument::D3DSRVArgument(UINT rootParamIndex)
{
	Type = D3D12_INDIRECT_ARGUMENT_TYPE_SHADER_RESOURCE_VIEW;
	ShaderResourceView.RootParameterIndex = rootParamIndex;
}

D3DUAVArgument::D3DUAVArgument(UINT rootParamIndex)
{
	Type = D3D12_INDIRECT_ARGUMENT_TYPE_UNORDERED_ACCESS_VIEW;
	UnorderedAccessView.RootParameterIndex = rootParamIndex;
}

D3DCommandSignatureDesc::D3DCommandSignatureDesc(UINT byteStride, UINT numArgumentDescs, const D3D12_INDIRECT_ARGUMENT_DESC* pFirstArgumentDesc)
{
	ByteStride = byteStride;
	NumArgumentDescs = numArgumentDescs;
	pArgumentDescs = pFirstArgumentDesc;
	NodeMask = 0;
}
