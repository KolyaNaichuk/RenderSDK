#include "D3DWrapper/D3DRootSignature.h"
#include "D3DWrapper/D3DDevice.h"

D3DCBVRange::D3DCBVRange(UINT numDescriptors, UINT baseShaderRegister, UINT registerSpace, UINT offsetInDescriptorsFromTableStart)
{
	RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
	NumDescriptors = numDescriptors;
	BaseShaderRegister = baseShaderRegister;
	RegisterSpace = registerSpace;
	OffsetInDescriptorsFromTableStart = offsetInDescriptorsFromTableStart;
}

D3DSRVRange::D3DSRVRange(UINT numDescriptors, UINT baseShaderRegister, UINT registerSpace, UINT offsetInDescriptorsFromTableStart)
{
	RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	NumDescriptors = numDescriptors;
	BaseShaderRegister = baseShaderRegister;
	RegisterSpace = registerSpace;
	OffsetInDescriptorsFromTableStart = offsetInDescriptorsFromTableStart;
}

D3DUAVRange::D3DUAVRange(UINT numDescriptors, UINT baseShaderRegister, UINT registerSpace, UINT offsetInDescriptorsFromTableStart)
{
	RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
	NumDescriptors = numDescriptors;
	BaseShaderRegister = baseShaderRegister;
	RegisterSpace = registerSpace;
	OffsetInDescriptorsFromTableStart = offsetInDescriptorsFromTableStart;
}

D3DSamplerRange::D3DSamplerRange(UINT numDescriptors, UINT baseShaderRegister, UINT registerSpace, UINT offsetInDescriptorsFromTableStart)
{
	RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
	NumDescriptors = numDescriptors;
	BaseShaderRegister = baseShaderRegister;
	RegisterSpace = registerSpace;
	OffsetInDescriptorsFromTableStart = offsetInDescriptorsFromTableStart;
}

D3DRootDescriptorTableParameter::D3DRootDescriptorTableParameter()
{
	ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	DescriptorTable.NumDescriptorRanges = 0;
	DescriptorTable.pDescriptorRanges = nullptr;
	ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
}

D3DRootDescriptorTableParameter::D3DRootDescriptorTableParameter(UINT numDescriptorRanges, const D3D12_DESCRIPTOR_RANGE* pDescriptorRanges, D3D12_SHADER_VISIBILITY shaderVisibility)
{
	ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	DescriptorTable.NumDescriptorRanges = numDescriptorRanges;
	DescriptorTable.pDescriptorRanges = pDescriptorRanges;
	ShaderVisibility = shaderVisibility;
}

D3DRootCBVParameter::D3DRootCBVParameter(UINT shaderRegister, D3D12_SHADER_VISIBILITY shaderVisibility, UINT registerSpace)
{
	ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	Descriptor.ShaderRegister = shaderRegister;
	Descriptor.RegisterSpace = registerSpace;
	ShaderVisibility = shaderVisibility;
}

D3DRootSRVParameter::D3DRootSRVParameter(UINT shaderRegister, D3D12_SHADER_VISIBILITY shaderVisibility, UINT registerSpace)
{
	ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
	Descriptor.ShaderRegister = shaderRegister;
	Descriptor.RegisterSpace = registerSpace;
	ShaderVisibility = shaderVisibility;
}

D3DRootUAVParameter::D3DRootUAVParameter(UINT shaderRegister, D3D12_SHADER_VISIBILITY shaderVisibility, UINT registerSpace)
{
	ParameterType = D3D12_ROOT_PARAMETER_TYPE_UAV;
	Descriptor.ShaderRegister = shaderRegister;
	Descriptor.RegisterSpace = registerSpace;
	ShaderVisibility = shaderVisibility;
}

D3DRoot32BitConstantsParameter::D3DRoot32BitConstantsParameter(UINT shaderRegister, D3D12_SHADER_VISIBILITY shaderVisibility, UINT num32BitValues, UINT registerSpace)
{
	ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
	Constants.ShaderRegister = shaderRegister;
	Constants.RegisterSpace = registerSpace;
	Constants.Num32BitValues = num32BitValues;
	ShaderVisibility = shaderVisibility;
}

D3DRootSignatureDesc::D3DRootSignatureDesc()
	: D3DRootSignatureDesc(0, nullptr, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_NONE)
{
}

D3DRootSignatureDesc::D3DRootSignatureDesc(UINT numParameters, const D3D12_ROOT_PARAMETER* pFirstParameter, D3D12_ROOT_SIGNATURE_FLAGS flags)
	: D3DRootSignatureDesc(numParameters, pFirstParameter, 0, nullptr, flags)
{
}

D3DRootSignatureDesc::D3DRootSignatureDesc(UINT numStaticSamplers, const D3D12_STATIC_SAMPLER_DESC* pFirstStaticSampler, D3D12_ROOT_SIGNATURE_FLAGS flags)
	: D3DRootSignatureDesc(0, nullptr, numStaticSamplers, pFirstStaticSampler, flags)
{
}

D3DRootSignatureDesc::D3DRootSignatureDesc(D3D12_ROOT_SIGNATURE_FLAGS flags)
	: D3DRootSignatureDesc(0, nullptr, 0, nullptr, flags)
{
}

D3DRootSignatureDesc::D3DRootSignatureDesc(UINT numParameters, const D3D12_ROOT_PARAMETER* pFirstParameter,
	UINT numStaticSamplers, const D3D12_STATIC_SAMPLER_DESC* pFirstStaticSampler, D3D12_ROOT_SIGNATURE_FLAGS flags)
{
	NumParameters = numParameters;
	pParameters = pFirstParameter;
	NumStaticSamplers = numStaticSamplers;
	pStaticSamplers = pFirstStaticSampler;
	Flags = flags;
}

D3DRootSignature::D3DRootSignature(D3DDevice* pDevice, const D3DRootSignatureDesc* pDesc, LPCWSTR pName)
{
	ComPtr<ID3DBlob> d3dRootSignatureBlob;
	ComPtr<ID3DBlob> d3dErrorBlob;
	
	HRESULT result = D3D12SerializeRootSignature(pDesc, D3D_ROOT_SIGNATURE_VERSION_1, &d3dRootSignatureBlob, &d3dErrorBlob);
	if (FAILED(result)) {
		OutputDebugStringA((LPCSTR)d3dErrorBlob->GetBufferPointer());
		assert(false);
	}

	UINT nodeMask = 0;
	DXVerify(pDevice->GetDXObject()->CreateRootSignature(nodeMask, d3dRootSignatureBlob->GetBufferPointer(), d3dRootSignatureBlob->GetBufferSize(), IID_PPV_ARGS(GetDXObjectAddress())));

#ifdef _DEBUG
	SetName(pName);
#endif
}
