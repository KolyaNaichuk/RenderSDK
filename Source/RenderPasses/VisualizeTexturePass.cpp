#include "RenderPasses/VisualizeTexturePass.h"
#include "D3DWrapper/CommandList.h"
#include "D3DWrapper/RenderEnv.h"
#include "D3DWrapper/PipelineState.h"
#include "D3DWrapper/RootSignature.h"
#include "D3DWrapper/GraphicsUtils.h"

enum RootParams
{
	kSRVRootParam = 0,
	kNumRootParams
};

VisualizeTexturePass::VisualizeTexturePass(InitParams* pParams)
	: m_pRootSignature(nullptr)
	, m_pPipelineState(nullptr)
{
	RenderEnv* pRenderEnv = pParams->m_pRenderEnv;
	
	std::string textureTypeStr = std::to_string(pParams->m_TextureType);
	const ShaderMacro shaderDefines[] =
	{
		ShaderMacro("TEXTURE_TYPE", textureTypeStr.c_str()),
		ShaderMacro()
	};

	Shader vertexShader(L"Shaders//FullScreenTriangleVS.hlsl", "Main", "vs_4_0");
	Shader pixelShader(L"Shaders//VisualizeTexturePS.hlsl", "Main", "ps_4_0", shaderDefines);

	D3D12_DESCRIPTOR_RANGE srvDescriptorRanges[] = {CBVDescriptorRange(1, 0), SRVDescriptorRange(1, 0)};
	D3D12_ROOT_PARAMETER rootParams[kNumRootParams];
	rootParams[kSRVRootParam] = RootDescriptorTableParameter(ARRAYSIZE(srvDescriptorRanges), &srvDescriptorRanges[0], D3D12_SHADER_VISIBILITY_PIXEL);

	StaticSamplerDesc samplerDesc(StaticSamplerDesc::Point, 0, D3D12_SHADER_VISIBILITY_PIXEL);

	RootSignatureDesc rootSignatureDesc(kNumRootParams, rootParams, 1, &samplerDesc, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
	m_pRootSignature = new RootSignature(pRenderEnv->m_pDevice, &rootSignatureDesc, L"VisualizeTexturePass::m_pRootSignature");

	GraphicsPipelineStateDesc pipelineStateDesc;
	pipelineStateDesc.SetRootSignature(m_pRootSignature);
	pipelineStateDesc.SetVertexShader(&vertexShader);
	pipelineStateDesc.SetPixelShader(&pixelShader);
	pipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	pipelineStateDesc.SetRenderTargetFormat(pParams->m_RTVFormat);

	m_pPipelineState = new PipelineState(pRenderEnv->m_pDevice, &pipelineStateDesc, L"VisualizeTexturePass::m_pPipelineState");
}

VisualizeTexturePass::~VisualizeTexturePass()
{
	SafeDelete(m_pPipelineState);
	SafeDelete(m_pRootSignature);
}

void VisualizeTexturePass::Record(RenderParams* pParams)
{
	assert(false && "Kolya. Fix me");
	/*
	RenderEnv* pRenderEnv = pParams->m_pRenderEnv;
	CommandList* pCommandList = pParams->m_pCommandList;
	ResourceList* pResources = pParams->m_pResources;

	pCommandList->Begin(m_pPipelineState);
	pCommandList->SetGraphicsRootSignature(m_pRootSignature);
	pCommandList->SetRequiredResourceStates(&pResources->m_RequiredResourceStates);
	pCommandList->SetDescriptorHeaps(pRenderEnv->m_pShaderVisibleSRVHeap);
	pCommandList->SetGraphicsRootDescriptorTable(kSRVRootParam, pResources->m_SRVHeapStart);

	D3D12_CPU_DESCRIPTOR_HANDLE rtvHeapStart = pResources->m_RTVHeapStart;
	pCommandList->OMSetRenderTargets(1, &rtvHeapStart);

	pCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	pCommandList->IASetVertexBuffers(0, 1, nullptr);
	pCommandList->IASetIndexBuffer(nullptr);

	Rect scissorRect(ExtractRect(pParams->m_pViewport));
	pCommandList->RSSetViewports(1, pParams->m_pViewport);
	pCommandList->RSSetScissorRects(1, &scissorRect);

	pCommandList->DrawInstanced(3, 1, 0, 0);
	pCommandList->End();
	*/
}
