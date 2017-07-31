#pragma once

#include "D3DWrapper/Common.h"

class GraphicsDevice;
class RootSignature;

struct ShaderBytecode : public D3D12_SHADER_BYTECODE
{
	ShaderBytecode(const void* pBytecode = nullptr, SIZE_T bytecodeLength = 0);
};

struct ShaderMacro : public D3D_SHADER_MACRO
{
	ShaderMacro();
	ShaderMacro(LPCSTR pName, LPCSTR pDefinition);
};

class Shader
{
public:
	Shader(LPCWSTR pFileName, LPCSTR pEntryPoint, LPCSTR pShaderModel, const ShaderMacro* pDefines = nullptr);
	ShaderBytecode GetBytecode();

private:
	ComPtr<ID3DBlob> m_D3DBytecodeBlob;
};

struct CachedPipelineState : public D3D12_CACHED_PIPELINE_STATE
{
	CachedPipelineState(const void* pBlob = nullptr, SIZE_T blobSizeInBytes = 0);
};

struct StreamOutputDesc : public D3D12_STREAM_OUTPUT_DESC
{
	StreamOutputDesc();
};

struct BlendDesc : public D3D12_BLEND_DESC
{
	enum Id
	{
		Disabled
	};
	BlendDesc(Id id = Disabled);
};

struct RasterizerDesc : public D3D12_RASTERIZER_DESC
{
	enum Id
	{
		Default,
		CullNoneConservative
	};
	RasterizerDesc(Id id = Default);
};

struct DepthStencilDesc : public D3D12_DEPTH_STENCIL_DESC
{
	enum Id
	{
		Disabled,
		Enabled,
		Always
	};
	DepthStencilDesc(Id id = Disabled);
};

struct SamplerDesc : public D3D12_SAMPLER_DESC
{
	enum Id
	{
		Point,
		Linear,
		Anisotropic
	};
	SamplerDesc(Id id);
};

struct StaticSamplerDesc : public D3D12_STATIC_SAMPLER_DESC
{
	enum Id
	{
		Point,
		Linear,
		Anisotropic,
		MaxPoint
	};
	StaticSamplerDesc(Id id, UINT shaderRegister, D3D12_SHADER_VISIBILITY shaderVisibility, UINT registerSpace = 0);
};

struct InputElementDesc : public D3D12_INPUT_ELEMENT_DESC
{
	InputElementDesc(LPCSTR pSemanticName, UINT semanticIndex, DXGI_FORMAT format, UINT inputSlot, UINT alignedByteOffset, bool perVertexData = true);
};

struct InputLayoutDesc : public D3D12_INPUT_LAYOUT_DESC
{
	InputLayoutDesc(UINT numElements = 0, const InputElementDesc* pFirstInputElementDesc = nullptr);
};

struct GraphicsPipelineStateDesc : D3D12_GRAPHICS_PIPELINE_STATE_DESC
{
	GraphicsPipelineStateDesc();

	void SetRootSignature(RootSignature* pSignature);

	void SetVertexShader(Shader* pShader);
	void SetPixelShader(Shader* pShader);
	void SetDomainShader(Shader* pShader);
	void SetHullShader(Shader* pShader);
	void SetGeometryShader(Shader* pShader);
	
	void SetRenderTargetFormat(DXGI_FORMAT rtvFormat, DXGI_FORMAT dsvFormat = DXGI_FORMAT_UNKNOWN);
	void SetRenderTargetFormats(UINT numRenderTargets, const DXGI_FORMAT* rtvFormats, DXGI_FORMAT dsvFormat = DXGI_FORMAT_UNKNOWN);
};

struct ComputePipelineStateDesc : D3D12_COMPUTE_PIPELINE_STATE_DESC
{
	ComputePipelineStateDesc();

	void SetRootSignature(RootSignature* pSignature);
	void SetComputeShader(Shader* pShader);
};

class PipelineState
{
public:
	PipelineState(GraphicsDevice* pDevice, const GraphicsPipelineStateDesc* pDesc, LPCWSTR pName);
	PipelineState(GraphicsDevice* pDevice, const ComputePipelineStateDesc* pDesc, LPCWSTR pName);

	ID3D12PipelineState* GetD3DObject() { return m_D3DPipelineState.Get(); }

private:
	ComPtr<ID3D12PipelineState> m_D3DPipelineState;
};
