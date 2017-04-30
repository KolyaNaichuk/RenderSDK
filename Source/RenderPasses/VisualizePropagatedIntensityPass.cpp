#include "RenderPasses/VisualizePropagatedIntensityPass.h"
#include "D3DWrapper/RootSignature.h"
#include "D3DWrapper/PipelineState.h"
#include "D3DWrapper/RenderEnv.h"
#include "D3DWrapper/GraphicsResource.h"
#include "D3DWrapper/CommandList.h"
#include "D3DWrapper/GraphicsUtils.h"
#include <string>
#include <cassert>

enum RootParams
{
	kSRVRootParam = 0,
	kNumRootParams
};

VisualizePropagatedIntensityPass::VisualizePropagatedIntensityPass(InitParams* pParams)
	: m_pRootSignature(nullptr)
	, m_pVerticalBoundaryState(nullptr)
	, m_pHorizontalBoundaryState(nullptr)
	, m_pIntensityDistributionState(nullptr)
	, m_NumGridCellsPerSlice(0)
	, m_NumIntensityVerticesPerCell(0)
	, m_NumHorizontalBoundariesPerSlice(0)
	, m_NumVerticalBoundariesPerSlice(0)
{
	RenderEnv* pRenderEnv = pParams->m_pRenderEnv;

	switch (pParams->m_ViewDir)
	{
		case ViewDirection_X:
		{
			m_NumGridCellsPerSlice = pParams->m_NumGridCellsZ * pParams->m_NumGridCellsY;
			m_NumHorizontalBoundariesPerSlice = 1 + pParams->m_NumGridCellsY;
			m_NumVerticalBoundariesPerSlice = 1 + pParams->m_NumGridCellsZ;

			break;
		}
		case ViewDirection_Y:
		{
			m_NumGridCellsPerSlice = pParams->m_NumGridCellsX * pParams->m_NumGridCellsZ;
			m_NumHorizontalBoundariesPerSlice = 1 + pParams->m_NumGridCellsZ;
			m_NumVerticalBoundariesPerSlice = 1 + pParams->m_NumGridCellsX;

			break;
		}
		case ViewDirection_Z:
		{
			m_NumGridCellsPerSlice = pParams->m_NumGridCellsX * pParams->m_NumGridCellsY;
			m_NumHorizontalBoundariesPerSlice = 1 + pParams->m_NumGridCellsY;
			m_NumVerticalBoundariesPerSlice = 1 + pParams->m_NumGridCellsX;

			break;
		}
		default:
		{
			assert(false);
		}
	}

	D3D12_DESCRIPTOR_RANGE srvDescriptorRanges[] =
	{
		CBVDescriptorRange(1, 0),
		SRVDescriptorRange(1, 0)
	};
	D3D12_ROOT_PARAMETER rootParams[kNumRootParams];
	rootParams[kSRVRootParam] = RootDescriptorTableParameter(ARRAYSIZE(srvDescriptorRanges), srvDescriptorRanges, D3D12_SHADER_VISIBILITY_VERTEX);
	
	RootSignatureDesc rootSignatureDesc(kNumRootParams, rootParams);
	m_pRootSignature = new RootSignature(pRenderEnv->m_pDevice, &rootSignatureDesc, L"VisualizePropagatedIntensityPass::m_pRootSignature");

	std::string viewDirStr = std::to_string(pParams->m_ViewDir);
	std::string sliceToVisualizeStr = std::to_string(pParams->m_SliceToVisualize);

	assert(pParams->m_NumIntensitySamples > 2);
	std::string numIntensityPolygonSidesStr = std::to_string(pParams->m_NumIntensitySamples);
	m_NumIntensityVerticesPerCell = 1 + pParams->m_NumIntensitySamples;
	
	Shader pixelShader(L"Shaders//VisualizePropagatedIntensityPS.hlsl", "Main", "vs_4_0");
	{
		const ShaderMacro shaderDefines[] =
		{
			ShaderMacro("HORIZONTAL_BOUNDARY", "1"),
			ShaderMacro("VIEW_DIRECTION", viewDirStr.c_str()),
			ShaderMacro()
		};
		Shader vertexShader(L"Shaders//VisualizePropagatedIntensityVS.hlsl", "Main", "vs_4_0", shaderDefines);

		GraphicsPipelineStateDesc pipelineStateDesc;
		pipelineStateDesc.SetRootSignature(m_pRootSignature);
		pipelineStateDesc.SetVertexShader(&vertexShader);
		pipelineStateDesc.SetPixelShader(&pixelShader);
		pipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
		pipelineStateDesc.SetRenderTargetFormat(pParams->m_RTVFormat);

		m_pHorizontalBoundaryState = new PipelineState(pRenderEnv->m_pDevice, &pipelineStateDesc, L"VisualizePropagatedIntensityPass::m_pHorizontalBoundaryState");
	}
	{
		const ShaderMacro shaderDefines[] =
		{
			ShaderMacro("VERTICAL_BOUNDARY", "1"),
			ShaderMacro("VIEW_DIRECTION", viewDirStr.c_str()),
			ShaderMacro()
		};
		Shader vertexShader(L"Shaders//VisualizePropagatedIntensityVS.hlsl", "Main", "vs_4_0", shaderDefines);

		GraphicsPipelineStateDesc pipelineStateDesc;
		pipelineStateDesc.SetRootSignature(m_pRootSignature);
		pipelineStateDesc.SetVertexShader(&vertexShader);
		pipelineStateDesc.SetPixelShader(&pixelShader);
		pipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
		pipelineStateDesc.SetRenderTargetFormat(pParams->m_RTVFormat);

		m_pVerticalBoundaryState = new PipelineState(pRenderEnv->m_pDevice, &pipelineStateDesc, L"VisualizePropagatedIntensityPass::m_pVerticalBoundaryState");
	}
	{
		const ShaderMacro shaderDefines[] =
		{
			ShaderMacro("INTENSITY_DISTRIBUTION", "1"),
			ShaderMacro("VIEW_DIRECTION", viewDirStr.c_str()),
			ShaderMacro("SLICE_TO_VISUALIZE", sliceToVisualizeStr.c_str()),
			ShaderMacro("NUM_INTENSITY_POLYGON_SIDES", numIntensityPolygonSidesStr.c_str()),
			ShaderMacro()
		};
		Shader vertexShader(L"Shaders//VisualizePropagatedIntensityVS.hlsl", "Main", "vs_4_0", shaderDefines);

		GraphicsPipelineStateDesc pipelineStateDesc;
		pipelineStateDesc.SetRootSignature(m_pRootSignature);
		pipelineStateDesc.SetVertexShader(&vertexShader);
		pipelineStateDesc.SetPixelShader(&pixelShader);
		pipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
		pipelineStateDesc.SetRenderTargetFormat(pParams->m_RTVFormat);

		m_pIntensityDistributionState = new PipelineState(pRenderEnv->m_pDevice, &pipelineStateDesc, L"VisualizePropagatedIntensityPass::m_pIntensityDistributionState");
	}
}

VisualizePropagatedIntensityPass::~VisualizePropagatedIntensityPass()
{
	SafeDelete(m_pVerticalBoundaryState);
	SafeDelete(m_pHorizontalBoundaryState);
	SafeDelete(m_pIntensityDistributionState);
	SafeDelete(m_pRootSignature);
}

void VisualizePropagatedIntensityPass::Record(RenderParams* pParams)
{
	RenderEnv* pRenderEnv = pParams->m_pRenderEnv;
	CommandList* pCommandList = pParams->m_pCommandList;
	BindingResourceList* pResources = pParams->m_pResources;

	pCommandList->Begin();

	pCommandList->SetGraphicsRootSignature(m_pRootSignature);
	pCommandList->SetRequiredResourceStates(&pResources->m_RequiredResourceStates);
	pCommandList->SetDescriptorHeaps(pRenderEnv->m_pShaderVisibleSRVHeap);
	pCommandList->SetGraphicsRootDescriptorTable(kSRVRootParam, pResources->m_SRVHeapStart);

	D3D12_CPU_DESCRIPTOR_HANDLE rtvHeapStart = pResources->m_RTVHeapStart;
	pCommandList->OMSetRenderTargets(1, &rtvHeapStart);
		
	Rect scissorRect(ExtractRect(pParams->m_pViewport));
	pCommandList->RSSetViewports(1, pParams->m_pViewport);
	pCommandList->RSSetScissorRects(1, &scissorRect);
	
	pCommandList->IASetVertexBuffers(0, 1, nullptr);
	pCommandList->IASetIndexBuffer(nullptr);

	pCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);
	pCommandList->SetPipelineState(m_pHorizontalBoundaryState);
	pCommandList->DrawInstanced(2, m_NumHorizontalBoundariesPerSlice, 0, 0);
	pCommandList->SetPipelineState(m_pVerticalBoundaryState);
	pCommandList->DrawInstanced(2, m_NumVerticalBoundariesPerSlice, 0, 0);
	
	pCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINESTRIP);
	pCommandList->SetPipelineState(m_pIntensityDistributionState);
	pCommandList->DrawInstanced(m_NumIntensityVerticesPerCell, m_NumGridCellsPerSlice, 0, 0);

	pCommandList->End();
}
