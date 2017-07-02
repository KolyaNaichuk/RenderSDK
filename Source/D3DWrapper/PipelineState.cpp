#include "D3DWrapper/PipelineState.h"
#include "D3DWrapper/GraphicsDevice.h"
#include "D3DWrapper/RootSignature.h"

ShaderBytecode::ShaderBytecode(const void* pBytecode, SIZE_T bytecodeLength)
{
	pShaderBytecode = pBytecode;
	BytecodeLength = bytecodeLength;
}

ShaderMacro::ShaderMacro()
{
	Name = nullptr;
	Definition = nullptr;
}

ShaderMacro::ShaderMacro(LPCSTR pName, LPCSTR pDefinition)
{
	Name = pName;
	Definition = pDefinition;
}

Shader::Shader(LPCWSTR pFileName, LPCSTR pEntryPoint, LPCSTR pShaderModel, const ShaderMacro* pDefines)
{
	UINT compileFlags = 0;
	compileFlags |= D3DCOMPILE_ENABLE_STRICTNESS;
	compileFlags |= D3DCOMPILE_PACK_MATRIX_ROW_MAJOR;

#ifdef _DEBUG
	compileFlags |= D3DCOMPILE_DEBUG;
	compileFlags |= D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

	ComPtr<ID3DBlob> d3dErrorBlob;
	HRESULT result = D3DCompileFromFile(pFileName, pDefines, D3D_COMPILE_STANDARD_FILE_INCLUDE,
		pEntryPoint, pShaderModel, compileFlags, 0, &m_D3DBytecodeBlob, &d3dErrorBlob);

	if (FAILED(result))
	{
		OutputDebugStringA((LPCSTR)d3dErrorBlob->GetBufferPointer());
		assert(false);
	}
}

ShaderBytecode Shader::GetBytecode()
{
	return ShaderBytecode(m_D3DBytecodeBlob->GetBufferPointer(), m_D3DBytecodeBlob->GetBufferSize());
}

CachedPipelineState::CachedPipelineState(const void* pBlob, SIZE_T blobSizeInBytes)
{
	pCachedBlob = pBlob;
	CachedBlobSizeInBytes = blobSizeInBytes;
}

StreamOutputDesc::StreamOutputDesc()
{
	pSODeclaration = nullptr;
	NumEntries = 0;
	pBufferStrides = nullptr;
	NumStrides = 0;
	RasterizedStream = 0;
}

BlendDesc::BlendDesc(Id id)
{
	if (id == BlendDesc::Disabled)
	{
		AlphaToCoverageEnable = FALSE;
		IndependentBlendEnable = FALSE;
		
		for (u8 index = 0; index < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; ++index)
		{
			RenderTarget[index].BlendEnable = FALSE;
			RenderTarget[index].SrcBlend = D3D12_BLEND_ONE;
			RenderTarget[index].DestBlend = D3D12_BLEND_ZERO;
			RenderTarget[index].BlendOp = D3D12_BLEND_OP_ADD;
			RenderTarget[index].SrcBlendAlpha = D3D12_BLEND_ONE;
			RenderTarget[index].DestBlendAlpha = D3D12_BLEND_ZERO;
			RenderTarget[index].BlendOpAlpha = D3D12_BLEND_OP_ADD;
			RenderTarget[index].LogicOpEnable = FALSE;
			RenderTarget[index].LogicOp = D3D12_LOGIC_OP_NOOP;
			RenderTarget[index].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
		}
	}
	else
	{
		assert(false);
	}
}

RasterizerDesc::RasterizerDesc(Id id)
{
	if (id == RasterizerDesc::Default)
	{
		FillMode = D3D12_FILL_MODE_SOLID;
		CullMode = D3D12_CULL_MODE_BACK;
		FrontCounterClockwise = FALSE;
		DepthBias = 0;
		DepthBiasClamp = 0.0f;
		SlopeScaledDepthBias = 0.0f;
		DepthClipEnable = TRUE;
		MultisampleEnable = FALSE;
		AntialiasedLineEnable = FALSE;
		ForcedSampleCount = 0;
		ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
	}
	else if (id == RasterizerDesc::CullNoneConservative)
	{
		FillMode = D3D12_FILL_MODE_SOLID;
		CullMode = D3D12_CULL_MODE_NONE;
		FrontCounterClockwise = FALSE;
		DepthBias = 0;
		DepthBiasClamp = 0.0f;
		SlopeScaledDepthBias = 0.0f;
		DepthClipEnable = TRUE;
		MultisampleEnable = FALSE;
		AntialiasedLineEnable = FALSE;
		ForcedSampleCount = 0;
		ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_ON;
	}
	else
	{
		assert(false);
	}
}

DepthStencilDesc::DepthStencilDesc(Id id)
{
	if (id == DepthStencilDesc::Disabled)
	{
		DepthEnable = FALSE;
		DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
		DepthFunc = D3D12_COMPARISON_FUNC_ALWAYS;
		StencilEnable = FALSE;
		StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
		StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;
		FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
		FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
		FrontFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
		FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;
		BackFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
		BackFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;		
		BackFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
		BackFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;
	}
	else if (id == DepthStencilDesc::Enabled)
	{
		DepthEnable = TRUE;
		DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
		DepthFunc = D3D12_COMPARISON_FUNC_LESS;
		StencilEnable = FALSE;
		StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
		StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;
		FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
		FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
		FrontFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
		FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;
		BackFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
		BackFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
		BackFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
		BackFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;
	}
	else if (id == DepthStencilDesc::Always)
	{
		DepthEnable = TRUE;
		DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
		DepthFunc = D3D12_COMPARISON_FUNC_ALWAYS;
		StencilEnable = FALSE;
		StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
		StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;
		FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
		FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
		FrontFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
		FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;
		BackFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
		BackFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
		BackFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
		BackFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;
	}
	else
	{
		assert(false);
	}
}

SamplerDesc::SamplerDesc(Id id)
{
	if (id == SamplerDesc::Linear)
	{
		Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
		AddressU = AddressV = AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		MipLODBias = 0.0f;
		MaxAnisotropy = 1;
		ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
		BorderColor[0] = BorderColor[1] = BorderColor[2] = BorderColor[3] = 1.0f;
		MinLOD = -D3D12_FLOAT32_MAX;
		MaxLOD = D3D12_FLOAT32_MAX;
	}
	else if (id == SamplerDesc::Point)
	{
		Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
		AddressU = AddressV = AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		MipLODBias = 0.0f;
		MaxAnisotropy = 1;
		ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
		BorderColor[0] = BorderColor[1] = BorderColor[2] = BorderColor[3] = 1.0f;
		MinLOD = -D3D12_FLOAT32_MAX;
		MaxLOD = D3D12_FLOAT32_MAX;
	}
	else if (id == SamplerDesc::Anisotropic)
	{
		Filter = D3D12_FILTER_ANISOTROPIC;
		AddressU = AddressV = AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		MipLODBias = 0.0f;
		MaxAnisotropy = 16;
		ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
		BorderColor[0] = BorderColor[1] = BorderColor[2] = BorderColor[3] = 1.0f;
		MinLOD = 0.0f;
		MaxLOD = D3D12_FLOAT32_MAX;
	}
	else
	{
		assert(false);
	}
}

StaticSamplerDesc::StaticSamplerDesc(Id id, UINT shaderRegister, D3D12_SHADER_VISIBILITY shaderVisibility, UINT registerSpace)
{
	if (id == StaticSamplerDesc::Linear)
	{
		Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
		AddressU = AddressV = AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		MipLODBias = 0.0f;
		MaxAnisotropy = 1;
		ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
		BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK;
		MinLOD = -D3D12_FLOAT32_MAX;
		MaxLOD = D3D12_FLOAT32_MAX;
	}
	else if (id == StaticSamplerDesc::Point)
	{
		Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
		AddressU = AddressV = AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		MipLODBias = 0.0f;
		MaxAnisotropy = 1;
		ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
		BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK;
		MinLOD = -D3D12_FLOAT32_MAX;
		MaxLOD = D3D12_FLOAT32_MAX;
	}
	else if (id == StaticSamplerDesc::Anisotropic)
	{
		Filter = D3D12_FILTER_ANISOTROPIC;
		AddressU = AddressV = AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		MipLODBias = 0.0f;
		MaxAnisotropy = 16;
		ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
		BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK;
		MinLOD = 0.0f;
		MaxLOD = D3D12_FLOAT32_MAX;
	}
	else if (id == StaticSamplerDesc::Max)
	{
		assert(false);
	}
	else
	{
		assert(false);
	}
	ShaderRegister = shaderRegister;
	ShaderVisibility = shaderVisibility;
	RegisterSpace = registerSpace;
}

InputLayoutDesc::InputLayoutDesc(UINT numElements, const InputElementDesc* pFirstInputElementDesc)
{
	pInputElementDescs = pFirstInputElementDesc;
	NumElements = numElements;
}

GraphicsPipelineStateDesc::GraphicsPipelineStateDesc()
{
	pRootSignature = nullptr;

	VS = ShaderBytecode();
	PS = ShaderBytecode();
	DS = ShaderBytecode();
	HS = ShaderBytecode();
	GS = ShaderBytecode();

	StreamOutput = StreamOutputDesc();

	BlendState = BlendDesc();
	SampleMask = UINT_MAX;
	
	RasterizerState = RasterizerDesc();
	DepthStencilState = DepthStencilDesc();
	
	InputLayout = InputLayoutDesc();
	
	IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;
	PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED;

	SetRenderTargetFormats(0, nullptr);
	
	SampleDesc.Count = 1;
	SampleDesc.Quality = 0;

	CachedPSO = CachedPipelineState();
	
	NodeMask = 0;
	Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
}

void GraphicsPipelineStateDesc::SetRootSignature(RootSignature* pSignature)
{
	pRootSignature = pSignature->GetD3DObject();
}

void GraphicsPipelineStateDesc::SetVertexShader(Shader* pShader)
{
	VS = pShader->GetBytecode();
}

void GraphicsPipelineStateDesc::SetPixelShader(Shader* pShader)
{
	PS = pShader->GetBytecode();
}

void GraphicsPipelineStateDesc::SetDomainShader(Shader* pShader)
{
	DS = pShader->GetBytecode();
}

void GraphicsPipelineStateDesc::SetHullShader(Shader* pShader)
{
	HS = pShader->GetBytecode();
}

void GraphicsPipelineStateDesc::SetGeometryShader(Shader* pShader)
{
	GS = pShader->GetBytecode();
}

void GraphicsPipelineStateDesc::SetRenderTargetFormat(DXGI_FORMAT rtvFormat, DXGI_FORMAT dsvFormat)
{
	SetRenderTargetFormats(1, &rtvFormat, dsvFormat);
}

void GraphicsPipelineStateDesc::SetRenderTargetFormats(UINT numRenderTargets, const DXGI_FORMAT* rtvFormats, DXGI_FORMAT dsvFormat)
{
	NumRenderTargets = numRenderTargets;
	for (UINT i = 0; i < numRenderTargets; ++i)
		RTVFormats[i] = rtvFormats[i];
	for (UINT i = numRenderTargets; i < 8; ++i)
		RTVFormats[i] = DXGI_FORMAT_UNKNOWN;

	DSVFormat = dsvFormat;
}

ComputePipelineStateDesc::ComputePipelineStateDesc()
{
	pRootSignature = nullptr;
	CS = ShaderBytecode();
	CachedPSO = CachedPipelineState();
	NodeMask = 0;
	Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
}

void ComputePipelineStateDesc::SetRootSignature(RootSignature* pSignature)
{
	pRootSignature = pSignature->GetD3DObject();
}

void ComputePipelineStateDesc::SetComputeShader(Shader* pShader)
{
	CS = pShader->GetBytecode();
}

InputElementDesc::InputElementDesc(LPCSTR pSemanticName, UINT semanticIndex, DXGI_FORMAT format, UINT inputSlot, UINT alignedByteOffset, bool perVertexData)
{
	SemanticName = pSemanticName;
	SemanticIndex = semanticIndex;
	Format = format;
	InputSlot = inputSlot;
	AlignedByteOffset = alignedByteOffset;

	if (perVertexData)
	{
		InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
		InstanceDataStepRate = 0;
	}
	else
	{
		InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA;
		InstanceDataStepRate = 1;
	}
}

PipelineState::PipelineState(GraphicsDevice* pDevice, const GraphicsPipelineStateDesc* pDesc, LPCWSTR pName)
{
	VerifyD3DResult(pDevice->GetD3DObject()->CreateGraphicsPipelineState(pDesc, IID_PPV_ARGS(&m_D3DPipelineState)));
#ifdef _DEBUG
	VerifyD3DResult(m_D3DPipelineState->SetName(pName));
#endif
}

PipelineState::PipelineState(GraphicsDevice* pDevice, const ComputePipelineStateDesc* pDesc, LPCWSTR pName)
{
	VerifyD3DResult(pDevice->GetD3DObject()->CreateComputePipelineState(pDesc, IID_PPV_ARGS(&m_D3DPipelineState)));
#ifdef _DEBUG
	VerifyD3DResult(m_D3DPipelineState->SetName(pName));
#endif
}
