#pragma once

#include "DXObject.h"

class DXDevice;
class DXRootSignature;

struct DXShaderBytecode : public D3D12_SHADER_BYTECODE
{
	DXShaderBytecode(const void* pBytecode = nullptr, SIZE_T bytecodeLength = 0);
};

struct DXShaderMacro : public D3D_SHADER_MACRO
{
	DXShaderMacro();
	DXShaderMacro(LPCSTR pName, LPCSTR pDefinition);
};

class DXShader : public DXObject<ID3DBlob>
{
public:
	DXShader(LPCWSTR pFileName, LPCSTR pEntryPoint, LPCSTR pShaderModel, const DXShaderMacro* pDefines = nullptr);
	DXShaderBytecode GetBytecode();
};

struct DXCachedPipelineState : public D3D12_CACHED_PIPELINE_STATE
{
	DXCachedPipelineState(const void* pBlob = nullptr, SIZE_T blobSizeInBytes = 0);
};

struct DXSampleDesc : public DXGI_SAMPLE_DESC
{
	DXSampleDesc(UINT count = 1, UINT quality = 0);
};

struct DXStreamOutputDesc : public D3D12_STREAM_OUTPUT_DESC
{
	DXStreamOutputDesc();
};

struct DXBlendDesc : public D3D12_BLEND_DESC
{
	enum Id
	{
		Disabled
	};
	DXBlendDesc(Id id = Disabled);
};

struct DXRasterizerDesc : public D3D12_RASTERIZER_DESC
{
	enum Id
	{
		Default
	};
	DXRasterizerDesc(Id id = Default);
};

struct DXDepthStencilDesc : public D3D12_DEPTH_STENCIL_DESC
{
	enum Id
	{
		Disabled,
		Enabled
	};
	DXDepthStencilDesc(Id id = Disabled);
};

struct DXSamplerDesc : public D3D12_SAMPLER_DESC
{
	enum Id
	{
		Point,
		Linear
	};
	DXSamplerDesc(Id id);
};

struct DXInputElementDesc : public D3D12_INPUT_ELEMENT_DESC
{
	DXInputElementDesc(LPCSTR pSemanticName, UINT semanticIndex, DXGI_FORMAT format, UINT inputSlot, UINT alignedByteOffset, bool perVertexData = true);
};

struct DXInputLayoutDesc : public D3D12_INPUT_LAYOUT_DESC
{
	DXInputLayoutDesc(UINT numElements = 0, const DXInputElementDesc* pFirstInputElementDesc = nullptr);
};

struct DXGraphicsPipelineStateDesc : D3D12_GRAPHICS_PIPELINE_STATE_DESC
{
	DXGraphicsPipelineStateDesc();

	void SetRootSignature(DXRootSignature* pSignature);

	void SetVertexShader(DXShader* pShader);
	void SetPixelShader(DXShader* pShader);
	void SetDomainShader(DXShader* pShader);
	void SetHullShader(DXShader* pShader);
	void SetGeometryShader(DXShader* pShader);

	void SetInputLayout(UINT numElements, const DXInputElementDesc* pFirstElementDesc);

	void SetRenderTargetFormat(DXGI_FORMAT rtvFormat, DXGI_FORMAT dsvFormat = DXGI_FORMAT_UNKNOWN);
	void SetRenderTargetFormats(UINT numRenderTargets, const DXGI_FORMAT* rtvFormats, DXGI_FORMAT dsvFormat = DXGI_FORMAT_UNKNOWN);
};

struct DXComputePipelineStateDesc : D3D12_COMPUTE_PIPELINE_STATE_DESC
{
	DXComputePipelineStateDesc();

	void SetRootSignature(DXRootSignature* pSignature);
	void SetComputeShader(DXShader* pShader);
};

class DXPipelineState : public DXObject<ID3D12PipelineState>
{
public:
	DXPipelineState(DXDevice* pDevice, const DXGraphicsPipelineStateDesc* pDesc, LPCWSTR pName);
	DXPipelineState(DXDevice* pDevice, const DXComputePipelineStateDesc* pDesc, LPCWSTR pName);
};
