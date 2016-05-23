#include "DX/DXCommandSignature.h"
#include "DX/DXDevice.h"
#include "DX/DXRootSignature.h"

DXCommandSignature::DXCommandSignature(DXDevice* pDevice, DXRootSignature* pRootSignature, const DXCommandSignatureDesc* pDesc, LPCWSTR pName)
{
	DXVerify(pDevice->GetDXObject()->CreateCommandSignature(
		pDesc, (pRootSignature != nullptr) ? pRootSignature->GetDXObject() : nullptr,
		IID_PPV_ARGS(GetDXObjectAddress())));

#ifdef _DEBUG
	SetName(pName);
#endif
}

DXDrawArgument::DXDrawArgument()
{
	Type = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW;
}

DXDrawIndexedArgument::DXDrawIndexedArgument()
{
	Type = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED;
}

DXDispatchArgument::DXDispatchArgument()
{
	Type = D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH;
}

DX32BitConstantsArgument::DX32BitConstantsArgument(UINT rootParameterIndex, UINT destOffsetIn32BitValues, UINT num32BitValues)
{
	Type = D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT;
	Constant.RootParameterIndex = rootParameterIndex;
	Constant.DestOffsetIn32BitValues = destOffsetIn32BitValues;
	Constant.Num32BitValuesToSet = num32BitValues;
}

DXCBVArgument::DXCBVArgument(UINT rootParamIndex)
{
	Type = D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT_BUFFER_VIEW;
	ConstantBufferView.RootParameterIndex = rootParamIndex;
}

DXSRVArgument::DXSRVArgument(UINT rootParamIndex)
{
	Type = D3D12_INDIRECT_ARGUMENT_TYPE_SHADER_RESOURCE_VIEW;
	ShaderResourceView.RootParameterIndex = rootParamIndex;
}

DXUAVArgument::DXUAVArgument(UINT rootParamIndex)
{
	Type = D3D12_INDIRECT_ARGUMENT_TYPE_UNORDERED_ACCESS_VIEW;
	UnorderedAccessView.RootParameterIndex = rootParamIndex;
}

DXCommandSignatureDesc::DXCommandSignatureDesc(UINT byteStride, UINT numArgumentDescs, const D3D12_INDIRECT_ARGUMENT_DESC* pFirstArgumentDesc)
{
	ByteStride = byteStride;
	NumArgumentDescs = numArgumentDescs;
	pArgumentDescs = pFirstArgumentDesc;
	NodeMask = 0;
}
