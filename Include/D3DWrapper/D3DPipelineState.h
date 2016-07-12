#pragma once

#include "DXObject.h"

class D3DDevice;
class D3DRootSignature;

struct D3DShaderBytecode : public D3D12_SHADER_BYTECODE
{
	D3DShaderBytecode(const void* pBytecode = nullptr, SIZE_T bytecodeLength = 0);
};

struct D3DShaderMacro : public D3D_SHADER_MACRO
{
	D3DShaderMacro();
	D3DShaderMacro(LPCSTR pName, LPCSTR pDefinition);
};

class D3DShader : public DXObject<ID3DBlob>
{
public:
	D3DShader(LPCWSTR pFileName, LPCSTR pEntryPoint, LPCSTR pShaderModel, const D3DShaderMacro* pDefines = nullptr);
	D3DShaderBytecode GetBytecode();
};

struct D3DCachedPipelineState : public D3D12_CACHED_PIPELINE_STATE
{
	D3DCachedPipelineState(const void* pBlob = nullptr, SIZE_T blobSizeInBytes = 0);
};

struct D3DSampleDesc : public DXGI_SAMPLE_DESC
{
	D3DSampleDesc(UINT count = 1, UINT quality = 0);
};

struct D3DStreamOutputDesc : public D3D12_STREAM_OUTPUT_DESC
{
	D3DStreamOutputDesc();
};

struct D3DBlendDesc : public D3D12_BLEND_DESC
{
	enum Id
	{
		Disabled
	};
	D3DBlendDesc(Id id = Disabled);
};

struct D3DRasterizerDesc : public D3D12_RASTERIZER_DESC
{
	enum Id
	{
		Default,
		CullNoneConservative
	};
	D3DRasterizerDesc(Id id = Default);
};

struct D3DDepthStencilDesc : public D3D12_DEPTH_STENCIL_DESC
{
	enum Id
	{
		Disabled,
		Enabled
	};
	D3DDepthStencilDesc(Id id = Disabled);
};

struct D3DSamplerDesc : public D3D12_SAMPLER_DESC
{
	enum Id
	{
		Point,
		Linear,
		Anisotropic
	};
	D3DSamplerDesc(Id id);
};

struct D3DStaticSamplerDesc : public D3D12_STATIC_SAMPLER_DESC
{
	enum Id
	{
		Point,
		Linear,
		Anisotropic
	};
	D3DStaticSamplerDesc(Id id, UINT shaderRegister, D3D12_SHADER_VISIBILITY shaderVisibility, UINT registerSpace = 0);
};

struct D3DInputElementDesc : public D3D12_INPUT_ELEMENT_DESC
{
	D3DInputElementDesc(LPCSTR pSemanticName, UINT semanticIndex, DXGI_FORMAT format, UINT inputSlot, UINT alignedByteOffset, bool perVertexData = true);
};

struct D3DInputLayoutDesc : public D3D12_INPUT_LAYOUT_DESC
{
	D3DInputLayoutDesc(UINT numElements = 0, const D3DInputElementDesc* pFirstInputElementDesc = nullptr);
};

struct D3DGraphicsPipelineStateDesc : D3D12_GRAPHICS_PIPELINE_STATE_DESC
{
	D3DGraphicsPipelineStateDesc();

	void SetRootSignature(D3DRootSignature* pSignature);

	void SetVertexShader(D3DShader* pShader);
	void SetPixelShader(D3DShader* pShader);
	void SetDomainShader(D3DShader* pShader);
	void SetHullShader(D3DShader* pShader);
	void SetGeometryShader(D3DShader* pShader);
	
	void SetRenderTargetFormat(DXGI_FORMAT rtvFormat, DXGI_FORMAT dsvFormat = DXGI_FORMAT_UNKNOWN);
	void SetRenderTargetFormats(UINT numRenderTargets, const DXGI_FORMAT* rtvFormats, DXGI_FORMAT dsvFormat = DXGI_FORMAT_UNKNOWN);
};

struct D3DComputePipelineStateDesc : D3D12_COMPUTE_PIPELINE_STATE_DESC
{
	D3DComputePipelineStateDesc();

	void SetRootSignature(D3DRootSignature* pSignature);
	void SetComputeShader(D3DShader* pShader);
};

class D3DPipelineState : public DXObject<ID3D12PipelineState>
{
public:
	D3DPipelineState(D3DDevice* pDevice, const D3DGraphicsPipelineStateDesc* pDesc, LPCWSTR pName);
	D3DPipelineState(D3DDevice* pDevice, const D3DComputePipelineStateDesc* pDesc, LPCWSTR pName);
};
