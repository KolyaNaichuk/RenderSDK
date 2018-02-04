#include "D3DWrapper/RootSignature.h"
#include "D3DWrapper/GraphicsDevice.h"

CBVDescriptorRange::CBVDescriptorRange(UINT numDescriptors, UINT baseShaderRegister, UINT registerSpace, UINT offsetInDescriptorsFromTableStart)
{
	RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
	NumDescriptors = numDescriptors;
	BaseShaderRegister = baseShaderRegister;
	RegisterSpace = registerSpace;
	OffsetInDescriptorsFromTableStart = offsetInDescriptorsFromTableStart;
}

SRVDescriptorRange::SRVDescriptorRange(UINT numDescriptors, UINT baseShaderRegister, UINT registerSpace, UINT offsetInDescriptorsFromTableStart)
{
	RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	NumDescriptors = numDescriptors;
	BaseShaderRegister = baseShaderRegister;
	RegisterSpace = registerSpace;
	OffsetInDescriptorsFromTableStart = offsetInDescriptorsFromTableStart;
}

UAVDescriptorRange::UAVDescriptorRange(UINT numDescriptors, UINT baseShaderRegister, UINT registerSpace, UINT offsetInDescriptorsFromTableStart)
{
	RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
	NumDescriptors = numDescriptors;
	BaseShaderRegister = baseShaderRegister;
	RegisterSpace = registerSpace;
	OffsetInDescriptorsFromTableStart = offsetInDescriptorsFromTableStart;
}

SamplerRange::SamplerRange(UINT numDescriptors, UINT baseShaderRegister, UINT registerSpace, UINT offsetInDescriptorsFromTableStart)
{
	RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
	NumDescriptors = numDescriptors;
	BaseShaderRegister = baseShaderRegister;
	RegisterSpace = registerSpace;
	OffsetInDescriptorsFromTableStart = offsetInDescriptorsFromTableStart;
}

RootDescriptorTableParameter::RootDescriptorTableParameter(UINT numDescriptorRanges, const D3D12_DESCRIPTOR_RANGE* pDescriptorRanges, D3D12_SHADER_VISIBILITY shaderVisibility)
{
	ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	DescriptorTable.NumDescriptorRanges = numDescriptorRanges;
	DescriptorTable.pDescriptorRanges = pDescriptorRanges;
	ShaderVisibility = shaderVisibility;
}

RootCBVParameter::RootCBVParameter(UINT shaderRegister, D3D12_SHADER_VISIBILITY shaderVisibility, UINT registerSpace)
{
	ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	Descriptor.ShaderRegister = shaderRegister;
	Descriptor.RegisterSpace = registerSpace;
	ShaderVisibility = shaderVisibility;
}

RootSRVParameter::RootSRVParameter(UINT shaderRegister, D3D12_SHADER_VISIBILITY shaderVisibility, UINT registerSpace)
{
	ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
	Descriptor.ShaderRegister = shaderRegister;
	Descriptor.RegisterSpace = registerSpace;
	ShaderVisibility = shaderVisibility;
}

RootUAVParameter::RootUAVParameter(UINT shaderRegister, D3D12_SHADER_VISIBILITY shaderVisibility, UINT registerSpace)
{
	ParameterType = D3D12_ROOT_PARAMETER_TYPE_UAV;
	Descriptor.ShaderRegister = shaderRegister;
	Descriptor.RegisterSpace = registerSpace;
	ShaderVisibility = shaderVisibility;
}

Root32BitConstantsParameter::Root32BitConstantsParameter(UINT shaderRegister, D3D12_SHADER_VISIBILITY shaderVisibility, UINT num32BitValues, UINT registerSpace)
{
	ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
	Constants.ShaderRegister = shaderRegister;
	Constants.RegisterSpace = registerSpace;
	Constants.Num32BitValues = num32BitValues;
	ShaderVisibility = shaderVisibility;
}

RootSignatureDesc::RootSignatureDesc()
	: RootSignatureDesc(0, nullptr, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_NONE)
{
}

RootSignatureDesc::RootSignatureDesc(UINT numParameters, const D3D12_ROOT_PARAMETER* pFirstParameter, D3D12_ROOT_SIGNATURE_FLAGS flags)
	: RootSignatureDesc(numParameters, pFirstParameter, 0, nullptr, flags)
{
}

RootSignatureDesc::RootSignatureDesc(UINT numStaticSamplers, const D3D12_STATIC_SAMPLER_DESC* pFirstStaticSampler, D3D12_ROOT_SIGNATURE_FLAGS flags)
	: RootSignatureDesc(0, nullptr, numStaticSamplers, pFirstStaticSampler, flags)
{
}

RootSignatureDesc::RootSignatureDesc(D3D12_ROOT_SIGNATURE_FLAGS flags)
	: RootSignatureDesc(0, nullptr, 0, nullptr, flags)
{
}

RootSignatureDesc::RootSignatureDesc(UINT numParameters, const D3D12_ROOT_PARAMETER* pFirstParameter,
	UINT numStaticSamplers, const D3D12_STATIC_SAMPLER_DESC* pFirstStaticSampler, D3D12_ROOT_SIGNATURE_FLAGS flags)
{
	NumParameters = numParameters;
	pParameters = pFirstParameter;
	NumStaticSamplers = numStaticSamplers;
	pStaticSamplers = pFirstStaticSampler;
	Flags = flags;
}

RootSignature::RootSignature(GraphicsDevice* pDevice, const RootSignatureDesc* pDesc, LPCWSTR pName)
{
	ComPtr<ID3DBlob> d3dRootSignatureBlob;
	ComPtr<ID3DBlob> d3dErrorBlob;
	
	HRESULT result = D3D12SerializeRootSignature(pDesc, D3D_ROOT_SIGNATURE_VERSION_1, &d3dRootSignatureBlob, &d3dErrorBlob);
	if (FAILED(result)) {
		OutputDebugStringA((LPCSTR)d3dErrorBlob->GetBufferPointer());
		assert(false);
	}

	UINT nodeMask = 0;
	VerifyD3DResult(pDevice->GetD3DObject()->CreateRootSignature(nodeMask,
		d3dRootSignatureBlob->GetBufferPointer(), 
		d3dRootSignatureBlob->GetBufferSize(),
		IID_PPV_ARGS(&m_D3DRootSignature)));

#ifdef ENABLE_GRAPHICS_DEBUGGING
	VerifyD3DResult(m_D3DRootSignature->SetName(pName));
#endif // ENABLE_GRAPHICS_DEBUGGING
}
