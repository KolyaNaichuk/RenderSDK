#include "DX/DXRootSignature.h"
#include "DX/DXDevice.h"

DXCBVRange::DXCBVRange(UINT numDescriptors, UINT baseShaderRegister, UINT registerSpace, UINT offsetInDescriptorsFromTableStart)
{
	RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
	NumDescriptors = numDescriptors;
	BaseShaderRegister = baseShaderRegister;
	RegisterSpace = registerSpace;
	OffsetInDescriptorsFromTableStart = offsetInDescriptorsFromTableStart;
}

DXSRVRange::DXSRVRange(UINT numDescriptors, UINT baseShaderRegister, UINT registerSpace, UINT offsetInDescriptorsFromTableStart)
{
	RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	NumDescriptors = numDescriptors;
	BaseShaderRegister = baseShaderRegister;
	RegisterSpace = registerSpace;
	OffsetInDescriptorsFromTableStart = offsetInDescriptorsFromTableStart;
}

DXUAVRange::DXUAVRange(UINT numDescriptors, UINT baseShaderRegister, UINT registerSpace, UINT offsetInDescriptorsFromTableStart)
{
	RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
	NumDescriptors = numDescriptors;
	BaseShaderRegister = baseShaderRegister;
	RegisterSpace = registerSpace;
	OffsetInDescriptorsFromTableStart = offsetInDescriptorsFromTableStart;
}

DXSamplerRange::DXSamplerRange(UINT numDescriptors, UINT baseShaderRegister, UINT registerSpace, UINT offsetInDescriptorsFromTableStart)
{
	RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
	NumDescriptors = numDescriptors;
	BaseShaderRegister = baseShaderRegister;
	RegisterSpace = registerSpace;
	OffsetInDescriptorsFromTableStart = offsetInDescriptorsFromTableStart;
}

DXRootDescriptorTableParameter::DXRootDescriptorTableParameter()
{
	ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	DescriptorTable.NumDescriptorRanges = 0;
	DescriptorTable.pDescriptorRanges = nullptr;
	ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
}

DXRootDescriptorTableParameter::DXRootDescriptorTableParameter(UINT numDescriptorRanges, const D3D12_DESCRIPTOR_RANGE* pDescriptorRanges, D3D12_SHADER_VISIBILITY shaderVisibility)
{
	ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	DescriptorTable.NumDescriptorRanges = numDescriptorRanges;
	DescriptorTable.pDescriptorRanges = pDescriptorRanges;
	ShaderVisibility = shaderVisibility;
}

DXRootCBVParameter::DXRootCBVParameter(UINT shaderRegister, D3D12_SHADER_VISIBILITY shaderVisibility, UINT registerSpace)
{
	ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	Descriptor.ShaderRegister = shaderRegister;
	Descriptor.RegisterSpace = registerSpace;
	ShaderVisibility = shaderVisibility;
}

DXRootSRVParameter::DXRootSRVParameter(UINT shaderRegister, D3D12_SHADER_VISIBILITY shaderVisibility, UINT registerSpace)
{
	ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
	Descriptor.ShaderRegister = shaderRegister;
	Descriptor.RegisterSpace = registerSpace;
	ShaderVisibility = shaderVisibility;
}

DXRootUAVParameter::DXRootUAVParameter(UINT shaderRegister, D3D12_SHADER_VISIBILITY shaderVisibility, UINT registerSpace)
{
	ParameterType = D3D12_ROOT_PARAMETER_TYPE_UAV;
	Descriptor.ShaderRegister = shaderRegister;
	Descriptor.RegisterSpace = registerSpace;
	ShaderVisibility = shaderVisibility;
}

DXRootSignatureDesc::DXRootSignatureDesc()
	: DXRootSignatureDesc(0, nullptr, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_NONE)
{
}

DXRootSignatureDesc::DXRootSignatureDesc(UINT numParameters, const D3D12_ROOT_PARAMETER* pFirstParameter, D3D12_ROOT_SIGNATURE_FLAGS flags)
	: DXRootSignatureDesc(numParameters, pFirstParameter, 0, nullptr, flags)
{
}

DXRootSignatureDesc::DXRootSignatureDesc(UINT numStaticSamplers, const D3D12_STATIC_SAMPLER_DESC* pFirstStaticSampler, D3D12_ROOT_SIGNATURE_FLAGS flags)
	: DXRootSignatureDesc(0, nullptr, numStaticSamplers, pFirstStaticSampler, flags)
{
}

DXRootSignatureDesc::DXRootSignatureDesc(D3D12_ROOT_SIGNATURE_FLAGS flags)
	: DXRootSignatureDesc(0, nullptr, 0, nullptr, flags)
{
}

DXRootSignatureDesc::DXRootSignatureDesc(UINT numParameters, const D3D12_ROOT_PARAMETER* pFirstParameter,
	UINT numStaticSamplers, const D3D12_STATIC_SAMPLER_DESC* pFirstStaticSampler, D3D12_ROOT_SIGNATURE_FLAGS flags)
{
	NumParameters = numParameters;
	pParameters = pFirstParameter;
	NumStaticSamplers = numStaticSamplers;
	pStaticSamplers = pFirstStaticSampler;
	Flags = flags;
}

DXRootSignature::DXRootSignature(DXDevice* pDevice, const DXRootSignatureDesc* pDesc, LPCWSTR pName)
{
	ComPtr<ID3DBlob> blob;
	ComPtr<ID3DBlob> errorBlob;
	
	HRESULT result = D3D12SerializeRootSignature(pDesc, D3D_ROOT_SIGNATURE_VERSION_1, &blob, &errorBlob);
	if (FAILED(result)) {
		OutputDebugStringA((LPCSTR)errorBlob->GetBufferPointer());
		assert(false);
	}

	UINT nodeMask = 0;
	DXVerify(pDevice->GetDXObject()->CreateRootSignature(nodeMask, blob->GetBufferPointer(), blob->GetBufferSize(), IID_PPV_ARGS(GetDXObjectAddress())));

#ifdef _DEBUG
	SetName(pName);
#endif
}
