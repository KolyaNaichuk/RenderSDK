#pragma once

#include "DXObject.h"

class D3DDevice;

struct D3DCBVRange : public D3D12_DESCRIPTOR_RANGE
{
	D3DCBVRange(UINT numDescriptors, UINT baseShaderRegister, UINT registerSpace = 0, UINT offsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND);
};

struct D3DSRVRange : public D3D12_DESCRIPTOR_RANGE
{
	D3DSRVRange(UINT numDescriptors, UINT baseShaderRegister, UINT registerSpace = 0, UINT offsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND);
};

struct D3DUAVRange : public D3D12_DESCRIPTOR_RANGE
{
	D3DUAVRange(UINT numDescriptors, UINT baseShaderRegister, UINT registerSpace = 0, UINT offsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND);
};

struct D3DSamplerRange : public D3D12_DESCRIPTOR_RANGE
{
	D3DSamplerRange(UINT numDescriptors, UINT baseShaderRegister, UINT registerSpace = 0, UINT offsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND);
};

struct D3DRootDescriptorTableParameter : public D3D12_ROOT_PARAMETER
{
	D3DRootDescriptorTableParameter();
	D3DRootDescriptorTableParameter(UINT numDescriptorRanges, const D3D12_DESCRIPTOR_RANGE* pDescriptorRanges, D3D12_SHADER_VISIBILITY shaderVisibility);
};

struct D3DRootCBVParameter : public D3D12_ROOT_PARAMETER
{
	D3DRootCBVParameter(UINT shaderRegister, D3D12_SHADER_VISIBILITY shaderVisibility, UINT registerSpace = 0);
};

struct D3DRootSRVParameter : public D3D12_ROOT_PARAMETER
{
	D3DRootSRVParameter(UINT shaderRegister, D3D12_SHADER_VISIBILITY shaderVisibility, UINT registerSpace = 0);
};

struct D3DRootUAVParameter : public D3D12_ROOT_PARAMETER
{
	D3DRootUAVParameter(UINT shaderRegister, D3D12_SHADER_VISIBILITY shaderVisibility, UINT registerSpace = 0);
};

struct D3DRoot32BitConstantsParameter : public D3D12_ROOT_PARAMETER
{
	D3DRoot32BitConstantsParameter(UINT shaderRegister, D3D12_SHADER_VISIBILITY shaderVisibility, UINT num32BitValues, UINT registerSpace = 0);
};

struct D3DRootSignatureDesc : public D3D12_ROOT_SIGNATURE_DESC
{
	D3DRootSignatureDesc();

	D3DRootSignatureDesc(UINT numParameters, const D3D12_ROOT_PARAMETER* pFirstParameter,
		D3D12_ROOT_SIGNATURE_FLAGS flags = D3D12_ROOT_SIGNATURE_FLAG_NONE);

	D3DRootSignatureDesc(UINT numStaticSamplers, const D3D12_STATIC_SAMPLER_DESC* pFirstStaticSampler,
		D3D12_ROOT_SIGNATURE_FLAGS flags = D3D12_ROOT_SIGNATURE_FLAG_NONE);
	
	D3DRootSignatureDesc(D3D12_ROOT_SIGNATURE_FLAGS flags);

	D3DRootSignatureDesc(UINT numParameters, const D3D12_ROOT_PARAMETER* pFirstParameter,
		UINT numStaticSamplers, const D3D12_STATIC_SAMPLER_DESC* pFirstStaticSampler,
		D3D12_ROOT_SIGNATURE_FLAGS flags = D3D12_ROOT_SIGNATURE_FLAG_NONE);
};

class D3DRootSignature : public DXObject<ID3D12RootSignature>
{
public:
	D3DRootSignature(D3DDevice* pDevice, const D3DRootSignatureDesc* pDesc, LPCWSTR pName);
};

