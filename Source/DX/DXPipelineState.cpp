#include "DX/DXPipelineState.h"
#include "DX/DXDevice.h"
#include "DX/DXRootSignature.h"

DXShaderBytecode::DXShaderBytecode(const void* pBytecode, SIZE_T bytecodeLength)
{
	pShaderBytecode = pBytecode;
	BytecodeLength = bytecodeLength;
}

DXShaderMacro::DXShaderMacro()
{
	Name = nullptr;
	Definition = nullptr;
}

DXShaderMacro::DXShaderMacro(LPCSTR pName, LPCSTR pDefinition)
{
	Name = pName;
	Definition = pDefinition;
}

DXShader::DXShader(LPCWSTR pFileName, LPCSTR pEntryPoint, LPCSTR pShaderModel, const DXShaderMacro* pDefines)
{
	UINT compileFlags = 0;
	compileFlags |= D3DCOMPILE_ENABLE_STRICTNESS;
	compileFlags |= D3DCOMPILE_PACK_MATRIX_ROW_MAJOR;

#ifdef _DEBUG
	compileFlags |= D3DCOMPILE_DEBUG;
	compileFlags |= D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

	ComPtr<ID3DBlob> errorBlob;
	HRESULT result = D3DCompileFromFile(pFileName, pDefines, D3D_COMPILE_STANDARD_FILE_INCLUDE,
		pEntryPoint, pShaderModel, compileFlags, 0, GetDXObjectAddress(), &errorBlob);

	if (FAILED(result))
	{
		OutputDebugStringA((LPCSTR)errorBlob->GetBufferPointer());
		assert(false);
	}
}

DXShaderBytecode DXShader::GetBytecode()
{
	return DXShaderBytecode(GetDXObject()->GetBufferPointer(), GetDXObject()->GetBufferSize());
}

DXCachedPipelineState::DXCachedPipelineState(const void* pBlob, SIZE_T blobSizeInBytes)
{
	pCachedBlob = pBlob;
	CachedBlobSizeInBytes = blobSizeInBytes;
}

DXSampleDesc::DXSampleDesc(UINT count, UINT quality)
{
	Count = count;
	Quality = quality;
}

DXStreamOutputDesc::DXStreamOutputDesc()
{
	pSODeclaration = nullptr;
	NumEntries = 0;
	pBufferStrides = nullptr;
	NumStrides = 0;
	RasterizedStream = 0;
}

DXBlendDesc::DXBlendDesc(Id id)
{
	if (id == DXBlendDesc::Disabled)
	{
		AlphaToCoverageEnable = FALSE;
		IndependentBlendEnable = FALSE;
		
		RenderTarget[0].BlendEnable = FALSE;
		RenderTarget[0].SrcBlend = D3D12_BLEND_ONE;
		RenderTarget[0].DestBlend = D3D12_BLEND_ZERO;
		RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
		RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
		RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
		RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
		
		RenderTarget[0].LogicOpEnable = FALSE;
		RenderTarget[0].LogicOp = D3D12_LOGIC_OP_NOOP;
		
		RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
	}
	else
	{
		assert(false);
	}
}

DXRasterizerDesc::DXRasterizerDesc(Id id)
{
	if (id == DXRasterizerDesc::Default)
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
	else if (id == DXRasterizerDesc::CullNoneConservative)
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

DXDepthStencilDesc::DXDepthStencilDesc(Id id)
{
	if (id == DXDepthStencilDesc::Disabled)
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
	else if (id == DXDepthStencilDesc::Enabled)
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

DXSamplerDesc::DXSamplerDesc(Id id)
{
	if (id == DXSamplerDesc::Linear)
	{
		Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
		AddressU = AddressV = AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		MipLODBias = 0.0f;
		MaxAnisotropy = 1;
		ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
		BorderColor[0] = BorderColor[1] = BorderColor[2] = BorderColor[3] = 1.0f;
		MinLOD = -D3D12_FLOAT32_MAX;
		MaxLOD = D3D12_FLOAT32_MAX;
	}
	else if (id == DXSamplerDesc::Point)
	{
		Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
		AddressU = AddressV = AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		MipLODBias = 0.0f;
		MaxAnisotropy = 1;
		ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
		BorderColor[0] = BorderColor[1] = BorderColor[2] = BorderColor[3] = 1.0f;
		MinLOD = -D3D12_FLOAT32_MAX;
		MaxLOD = D3D12_FLOAT32_MAX;
	}
	else
	{
		assert(false);
	}
}

DXInputLayoutDesc::DXInputLayoutDesc(UINT numElements, const DXInputElementDesc* pFirstInputElementDesc)
{
	pInputElementDescs = pFirstInputElementDesc;
	NumElements = numElements;
}

DXGraphicsPipelineStateDesc::DXGraphicsPipelineStateDesc()
{
	pRootSignature = nullptr;

	VS = DXShaderBytecode();
	PS = DXShaderBytecode();
	DS = DXShaderBytecode();
	HS = DXShaderBytecode();
	GS = DXShaderBytecode();

	StreamOutput = DXStreamOutputDesc();

	BlendState = DXBlendDesc();
	SampleMask = UINT_MAX;
	
	RasterizerState = DXRasterizerDesc();
	DepthStencilState = DXDepthStencilDesc();
	
	InputLayout = DXInputLayoutDesc();
	
	IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;
	PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED;

	SetRenderTargetFormats(0, nullptr);
	
	SampleDesc = DXSampleDesc();
	CachedPSO = DXCachedPipelineState();
	
	NodeMask = 0;
	Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
}

void DXGraphicsPipelineStateDesc::SetRootSignature(DXRootSignature* pSignature)
{
	pRootSignature = pSignature->GetDXObject();
}

void DXGraphicsPipelineStateDesc::SetVertexShader(DXShader* pShader)
{
	VS = pShader->GetBytecode();
}

void DXGraphicsPipelineStateDesc::SetPixelShader(DXShader* pShader)
{
	PS = pShader->GetBytecode();
}

void DXGraphicsPipelineStateDesc::SetDomainShader(DXShader* pShader)
{
	DS = pShader->GetBytecode();
}

void DXGraphicsPipelineStateDesc::SetHullShader(DXShader* pShader)
{
	HS = pShader->GetBytecode();
}

void DXGraphicsPipelineStateDesc::SetGeometryShader(DXShader* pShader)
{
	GS = pShader->GetBytecode();
}

void DXGraphicsPipelineStateDesc::SetInputLayout(UINT numElements, const DXInputElementDesc* pFirstElementDesc)
{
	InputLayout = DXInputLayoutDesc(numElements, pFirstElementDesc);
}

void DXGraphicsPipelineStateDesc::SetRenderTargetFormat(DXGI_FORMAT rtvFormat, DXGI_FORMAT dsvFormat)
{
	SetRenderTargetFormats(1, &rtvFormat, dsvFormat);
}

void DXGraphicsPipelineStateDesc::SetRenderTargetFormats(UINT numRenderTargets, const DXGI_FORMAT* rtvFormats, DXGI_FORMAT dsvFormat)
{
	NumRenderTargets = numRenderTargets;
	for (UINT i = 0; i < numRenderTargets; ++i)
		RTVFormats[i] = rtvFormats[i];
	for (UINT i = numRenderTargets; i < 8; ++i)
		RTVFormats[i] = DXGI_FORMAT_UNKNOWN;

	DSVFormat = dsvFormat;
}

DXComputePipelineStateDesc::DXComputePipelineStateDesc()
{
	pRootSignature = nullptr;
	CS = DXShaderBytecode();
	CachedPSO = DXCachedPipelineState();
	NodeMask = 0;
	Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
}

void DXComputePipelineStateDesc::SetRootSignature(DXRootSignature* pSignature)
{
	pRootSignature = pSignature->GetDXObject();
}

void DXComputePipelineStateDesc::SetComputeShader(DXShader* pShader)
{
	CS = pShader->GetBytecode();
}

DXInputElementDesc::DXInputElementDesc(LPCSTR pSemanticName, UINT semanticIndex, DXGI_FORMAT format, UINT inputSlot, UINT alignedByteOffset, bool perVertexData)
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

DXPipelineState::DXPipelineState(DXDevice* pDevice, const DXGraphicsPipelineStateDesc* pDesc, LPCWSTR pName)
{
	DXVerify(pDevice->GetDXObject()->CreateGraphicsPipelineState(pDesc, IID_PPV_ARGS(GetDXObjectAddress())));

#ifdef _DEBUG
	SetName(pName);
#endif
}

DXPipelineState::DXPipelineState(DXDevice* pDevice, const DXComputePipelineStateDesc* pDesc, LPCWSTR pName)
{
	DXVerify(pDevice->GetDXObject()->CreateComputePipelineState(pDesc, IID_PPV_ARGS(GetDXObjectAddress())));

#ifdef _DEBUG
	SetName(pName);
#endif
}
