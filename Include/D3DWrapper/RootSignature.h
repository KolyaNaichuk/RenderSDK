#pragma once

#include "D3DWrapper/Common.h"

class GraphicsDevice;

struct CBVDescriptorRange : public D3D12_DESCRIPTOR_RANGE
{
	CBVDescriptorRange(UINT numDescriptors, UINT baseShaderRegister, UINT registerSpace = 0, UINT offsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND);
};

struct SRVDescriptorRange : public D3D12_DESCRIPTOR_RANGE
{
	SRVDescriptorRange(UINT numDescriptors, UINT baseShaderRegister, UINT registerSpace = 0, UINT offsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND);
};

struct UAVDescriptorRange : public D3D12_DESCRIPTOR_RANGE
{
	UAVDescriptorRange(UINT numDescriptors, UINT baseShaderRegister, UINT registerSpace = 0, UINT offsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND);
};

struct SamplerRange : public D3D12_DESCRIPTOR_RANGE
{
	SamplerRange(UINT numDescriptors, UINT baseShaderRegister, UINT registerSpace = 0, UINT offsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND);
};

struct RootDescriptorTableParameter : public D3D12_ROOT_PARAMETER
{
	RootDescriptorTableParameter();
	RootDescriptorTableParameter(UINT numDescriptorRanges, const D3D12_DESCRIPTOR_RANGE* pDescriptorRanges, D3D12_SHADER_VISIBILITY shaderVisibility);
};

struct RootCBVParameter : public D3D12_ROOT_PARAMETER
{
	RootCBVParameter(UINT shaderRegister, D3D12_SHADER_VISIBILITY shaderVisibility, UINT registerSpace = 0);
};

struct RootSRVParameter : public D3D12_ROOT_PARAMETER
{
	RootSRVParameter(UINT shaderRegister, D3D12_SHADER_VISIBILITY shaderVisibility, UINT registerSpace = 0);
};

struct RootUAVParameter : public D3D12_ROOT_PARAMETER
{
	RootUAVParameter(UINT shaderRegister, D3D12_SHADER_VISIBILITY shaderVisibility, UINT registerSpace = 0);
};

struct Root32BitConstantsParameter : public D3D12_ROOT_PARAMETER
{
	Root32BitConstantsParameter(UINT shaderRegister, D3D12_SHADER_VISIBILITY shaderVisibility, UINT num32BitValues, UINT registerSpace = 0);
};

struct RootSignatureDesc : public D3D12_ROOT_SIGNATURE_DESC
{
	RootSignatureDesc();

	RootSignatureDesc(UINT numParameters, const D3D12_ROOT_PARAMETER* pFirstParameter,
		D3D12_ROOT_SIGNATURE_FLAGS flags = D3D12_ROOT_SIGNATURE_FLAG_NONE);

	RootSignatureDesc(UINT numStaticSamplers, const D3D12_STATIC_SAMPLER_DESC* pFirstStaticSampler,
		D3D12_ROOT_SIGNATURE_FLAGS flags = D3D12_ROOT_SIGNATURE_FLAG_NONE);
	
	RootSignatureDesc(D3D12_ROOT_SIGNATURE_FLAGS flags);

	RootSignatureDesc(UINT numParameters, const D3D12_ROOT_PARAMETER* pFirstParameter,
		UINT numStaticSamplers, const D3D12_STATIC_SAMPLER_DESC* pFirstStaticSampler,
		D3D12_ROOT_SIGNATURE_FLAGS flags = D3D12_ROOT_SIGNATURE_FLAG_NONE);
};

class RootSignature
{
public:
	RootSignature(GraphicsDevice* pDevice, const RootSignatureDesc* pDesc, LPCWSTR pName);
	ID3D12RootSignature* GetD3DObject() { return m_D3DRootSignature.Get(); }

private:
	ComPtr<ID3D12RootSignature> m_D3DRootSignature;
};

