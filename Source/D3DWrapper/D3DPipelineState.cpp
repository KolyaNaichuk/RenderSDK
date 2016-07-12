#include "D3DWrapper/D3DPipelineState.h"
#include "D3DWrapper/D3DDevice.h"
#include "D3DWrapper/D3DRootSignature.h"

D3DShaderBytecode::D3DShaderBytecode(const void* pBytecode, SIZE_T bytecodeLength)
{
	pShaderBytecode = pBytecode;
	BytecodeLength = bytecodeLength;
}

D3DShaderMacro::D3DShaderMacro()
{
	Name = nullptr;
	Definition = nullptr;
}

D3DShaderMacro::D3DShaderMacro(LPCSTR pName, LPCSTR pDefinition)
{
	Name = pName;
	Definition = pDefinition;
}

D3DShader::D3DShader(LPCWSTR pFileName, LPCSTR pEntryPoint, LPCSTR pShaderModel, const D3DShaderMacro* pDefines)
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
		pEntryPoint, pShaderModel, compileFlags, 0, GetDXObjectAddress(), &d3dErrorBlob);

	if (FAILED(result))
	{
		OutputDebugStringA((LPCSTR)d3dErrorBlob->GetBufferPointer());
		assert(false);
	}
}

D3DShaderBytecode D3DShader::GetBytecode()
{
	return D3DShaderBytecode(GetDXObject()->GetBufferPointer(), GetDXObject()->GetBufferSize());
}

D3DCachedPipelineState::D3DCachedPipelineState(const void* pBlob, SIZE_T blobSizeInBytes)
{
	pCachedBlob = pBlob;
	CachedBlobSizeInBytes = blobSizeInBytes;
}

D3DSampleDesc::D3DSampleDesc(UINT count, UINT quality)
{
	Count = count;
	Quality = quality;
}

D3DStreamOutputDesc::D3DStreamOutputDesc()
{
	pSODeclaration = nullptr;
	NumEntries = 0;
	pBufferStrides = nullptr;
	NumStrides = 0;
	RasterizedStream = 0;
}

D3DBlendDesc::D3DBlendDesc(Id id)
{
	if (id == D3DBlendDesc::Disabled)
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

D3DRasterizerDesc::D3DRasterizerDesc(Id id)
{
	if (id == D3DRasterizerDesc::Default)
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
	else if (id == D3DRasterizerDesc::CullNoneConservative)
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
		ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
	}
	else
	{
		assert(false);
	}
}

D3DDepthStencilDesc::D3DDepthStencilDesc(Id id)
{
	if (id == D3DDepthStencilDesc::Disabled)
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
	else if (id == D3DDepthStencilDesc::Enabled)
	{
		DepthEnable = TRUE;
		DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
		DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
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

D3DSamplerDesc::D3DSamplerDesc(Id id)
{
	if (id == D3DSamplerDesc::Linear)
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
	else if (id == D3DSamplerDesc::Point)
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
	else if (id == D3DSamplerDesc::Anisotropic)
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

D3DStaticSamplerDesc::D3DStaticSamplerDesc(Id id, UINT shaderRegister, D3D12_SHADER_VISIBILITY shaderVisibility, UINT registerSpace)
{
	if (id == D3DStaticSamplerDesc::Linear)
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
	else if (id == D3DStaticSamplerDesc::Point)
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
	else if (id == D3DStaticSamplerDesc::Anisotropic)
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
	else
	{
		assert(false);
	}
	ShaderRegister = shaderRegister;
	ShaderVisibility = shaderVisibility;
	RegisterSpace = registerSpace;
}

D3DInputLayoutDesc::D3DInputLayoutDesc(UINT numElements, const D3DInputElementDesc* pFirstInputElementDesc)
{
	pInputElementDescs = pFirstInputElementDesc;
	NumElements = numElements;
}

D3DGraphicsPipelineStateDesc::D3DGraphicsPipelineStateDesc()
{
	pRootSignature = nullptr;

	VS = D3DShaderBytecode();
	PS = D3DShaderBytecode();
	DS = D3DShaderBytecode();
	HS = D3DShaderBytecode();
	GS = D3DShaderBytecode();

	StreamOutput = D3DStreamOutputDesc();

	BlendState = D3DBlendDesc();
	SampleMask = UINT_MAX;
	
	RasterizerState = D3DRasterizerDesc();
	DepthStencilState = D3DDepthStencilDesc();
	
	InputLayout = D3DInputLayoutDesc();
	
	IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;
	PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED;

	SetRenderTargetFormats(0, nullptr);
	
	SampleDesc = D3DSampleDesc();
	CachedPSO = D3DCachedPipelineState();
	
	NodeMask = 0;
	Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
}

void D3DGraphicsPipelineStateDesc::SetRootSignature(D3DRootSignature* pSignature)
{
	pRootSignature = pSignature->GetDXObject();
}

void D3DGraphicsPipelineStateDesc::SetVertexShader(D3DShader* pShader)
{
	VS = pShader->GetBytecode();
}

void D3DGraphicsPipelineStateDesc::SetPixelShader(D3DShader* pShader)
{
	PS = pShader->GetBytecode();
}

void D3DGraphicsPipelineStateDesc::SetDomainShader(D3DShader* pShader)
{
	DS = pShader->GetBytecode();
}

void D3DGraphicsPipelineStateDesc::SetHullShader(D3DShader* pShader)
{
	HS = pShader->GetBytecode();
}

void D3DGraphicsPipelineStateDesc::SetGeometryShader(D3DShader* pShader)
{
	GS = pShader->GetBytecode();
}

void D3DGraphicsPipelineStateDesc::SetRenderTargetFormat(DXGI_FORMAT rtvFormat, DXGI_FORMAT dsvFormat)
{
	SetRenderTargetFormats(1, &rtvFormat, dsvFormat);
}

void D3DGraphicsPipelineStateDesc::SetRenderTargetFormats(UINT numRenderTargets, const DXGI_FORMAT* rtvFormats, DXGI_FORMAT dsvFormat)
{
	NumRenderTargets = numRenderTargets;
	for (UINT i = 0; i < numRenderTargets; ++i)
		RTVFormats[i] = rtvFormats[i];
	for (UINT i = numRenderTargets; i < 8; ++i)
		RTVFormats[i] = DXGI_FORMAT_UNKNOWN;

	DSVFormat = dsvFormat;
}

D3DComputePipelineStateDesc::D3DComputePipelineStateDesc()
{
	pRootSignature = nullptr;
	CS = D3DShaderBytecode();
	CachedPSO = D3DCachedPipelineState();
	NodeMask = 0;
	Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
}

void D3DComputePipelineStateDesc::SetRootSignature(D3DRootSignature* pSignature)
{
	pRootSignature = pSignature->GetDXObject();
}

void D3DComputePipelineStateDesc::SetComputeShader(D3DShader* pShader)
{
	CS = pShader->GetBytecode();
}

D3DInputElementDesc::D3DInputElementDesc(LPCSTR pSemanticName, UINT semanticIndex, DXGI_FORMAT format, UINT inputSlot, UINT alignedByteOffset, bool perVertexData)
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

D3DPipelineState::D3DPipelineState(D3DDevice* pDevice, const D3DGraphicsPipelineStateDesc* pDesc, LPCWSTR pName)
{
	DXVerify(pDevice->GetDXObject()->CreateGraphicsPipelineState(pDesc, IID_PPV_ARGS(GetDXObjectAddress())));

#ifdef _DEBUG
	SetName(pName);
#endif
}

D3DPipelineState::D3DPipelineState(D3DDevice* pDevice, const D3DComputePipelineStateDesc* pDesc, LPCWSTR pName)
{
	DXVerify(pDevice->GetDXObject()->CreateComputePipelineState(pDesc, IID_PPV_ARGS(GetDXObjectAddress())));

#ifdef _DEBUG
	SetName(pName);
#endif
}
