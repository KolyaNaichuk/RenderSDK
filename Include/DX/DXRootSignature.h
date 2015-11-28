#pragma once

#include "DXObject.h"

class DXDevice;

struct DXCBVRange : public D3D12_DESCRIPTOR_RANGE
{
	DXCBVRange(UINT numDescriptors, UINT baseShaderRegister, UINT registerSpace = 0, UINT offsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND);
};

struct DXSRVRange : public D3D12_DESCRIPTOR_RANGE
{
	DXSRVRange(UINT numDescriptors, UINT baseShaderRegister, UINT registerSpace = 0, UINT offsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND);
};

struct DXUAVRange : public D3D12_DESCRIPTOR_RANGE
{
	DXUAVRange(UINT numDescriptors, UINT baseShaderRegister, UINT registerSpace = 0, UINT offsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND);
};

struct DXSamplerRange : public D3D12_DESCRIPTOR_RANGE
{
	DXSamplerRange(UINT numDescriptors, UINT baseShaderRegister, UINT registerSpace = 0, UINT offsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND);
};

struct DXRootDescriptorTableParameter : public D3D12_ROOT_PARAMETER
{
	DXRootDescriptorTableParameter();
	DXRootDescriptorTableParameter(UINT numDescriptorRanges, const D3D12_DESCRIPTOR_RANGE* pDescriptorRanges, D3D12_SHADER_VISIBILITY shaderVisibility);
};

struct DXRootCBVParameter : public D3D12_ROOT_PARAMETER
{
	DXRootCBVParameter(UINT shaderRegister, D3D12_SHADER_VISIBILITY shaderVisibility, UINT registerSpace = 0);
};

struct DXRootSRVParameter : public D3D12_ROOT_PARAMETER
{
	DXRootSRVParameter(UINT shaderRegister, D3D12_SHADER_VISIBILITY shaderVisibility, UINT registerSpace = 0);
};

struct DXRootUAVParameter : public D3D12_ROOT_PARAMETER
{
	DXRootUAVParameter(UINT shaderRegister, D3D12_SHADER_VISIBILITY shaderVisibility, UINT registerSpace = 0);
};

struct DXRootSignatureDesc : public D3D12_ROOT_SIGNATURE_DESC
{
	DXRootSignatureDesc();

	DXRootSignatureDesc(UINT numParameters, const D3D12_ROOT_PARAMETER* pFirstParameter,
		D3D12_ROOT_SIGNATURE_FLAGS flags = D3D12_ROOT_SIGNATURE_FLAG_NONE);

	DXRootSignatureDesc(UINT numStaticSamplers, const D3D12_STATIC_SAMPLER_DESC* pFirstStaticSampler,
		D3D12_ROOT_SIGNATURE_FLAGS flags = D3D12_ROOT_SIGNATURE_FLAG_NONE);
	
	DXRootSignatureDesc(D3D12_ROOT_SIGNATURE_FLAGS flags);

	DXRootSignatureDesc(UINT numParameters, const D3D12_ROOT_PARAMETER* pFirstParameter,
		UINT numStaticSamplers, const D3D12_STATIC_SAMPLER_DESC* pFirstStaticSampler,
		D3D12_ROOT_SIGNATURE_FLAGS flags = D3D12_ROOT_SIGNATURE_FLAG_NONE);
};

class DXRootSignature : public DXObject<ID3D12RootSignature>
{
public:
	DXRootSignature(DXDevice* pDevice, const DXRootSignatureDesc* pDesc, LPCWSTR pName);
};

