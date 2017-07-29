#include "RenderPasses/VisualizeIntensityPass.h"
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
	kConstant32BitRootParam,
	kNumRootParams
};

VisualizeIntensityPass::VisualizeIntensityPass(InitParams* pParams)
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
		case ViewDirection_WorldSpaceX:
		{
			assert(pParams->m_NumGridCellsX > 0);
			assert(pParams->m_SliceToVisualize < pParams->m_NumGridCellsX);

			m_NumGridCellsPerSlice = pParams->m_NumGridCellsZ * pParams->m_NumGridCellsY;
			m_NumHorizontalBoundariesPerSlice = 1 + pParams->m_NumGridCellsY;
			m_NumVerticalBoundariesPerSlice = 1 + pParams->m_NumGridCellsZ;

			break;
		}
		case ViewDirection_WorldSpaceY:
		{
			assert(pParams->m_NumGridCellsY > 0);
			assert(pParams->m_SliceToVisualize < pParams->m_NumGridCellsY);

			m_NumGridCellsPerSlice = pParams->m_NumGridCellsX * pParams->m_NumGridCellsZ;
			m_NumHorizontalBoundariesPerSlice = 1 + pParams->m_NumGridCellsZ;
			m_NumVerticalBoundariesPerSlice = 1 + pParams->m_NumGridCellsX;

			break;
		}
		case ViewDirection_WorldSpaceZ:
		{
			assert(pParams->m_NumGridCellsZ > 0);
			assert(pParams->m_SliceToVisualize < pParams->m_NumGridCellsZ);

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

	D3D12_DESCRIPTOR_RANGE srvDescriptorRanges[] = {SRVDescriptorRange(9, 0)};
	D3D12_ROOT_PARAMETER rootParams[kNumRootParams];
	rootParams[kSRVRootParam] = RootDescriptorTableParameter(ARRAYSIZE(srvDescriptorRanges), srvDescriptorRanges, D3D12_SHADER_VISIBILITY_VERTEX);
	rootParams[kConstant32BitRootParam] = Root32BitConstantsParameter(0, D3D12_SHADER_VISIBILITY_VERTEX, 1);
	
	RootSignatureDesc rootSignatureDesc(kNumRootParams, rootParams);
	m_pRootSignature = new RootSignature(pRenderEnv->m_pDevice, &rootSignatureDesc, L"VisualizeIntensityPass::m_pRootSignature");

	std::string viewDirStr = std::to_string(pParams->m_ViewDir);
	std::string sliceToVisualizeStr = std::to_string(pParams->m_SliceToVisualize);

	assert(pParams->m_NumIntensitySamples > 2);
	std::string numIntensityPolygonSidesStr = std::to_string(pParams->m_NumIntensitySamples);
	m_NumIntensityVerticesPerCell = 1 + pParams->m_NumIntensitySamples;

	std::string numGridCellsXStr = std::to_string(pParams->m_NumGridCellsX);
	std::string numGridCellsYStr = std::to_string(pParams->m_NumGridCellsY);
	std::string numGridCellsZStr = std::to_string(pParams->m_NumGridCellsZ);
	
	Shader pixelShader(L"Shaders//VisualizeIntensityPS.hlsl", "Main", "ps_4_0");
	{
		const ShaderMacro shaderDefines[] =
		{
			ShaderMacro("HORIZONTAL_BOUNDARY", "1"),
			ShaderMacro("VIEW_DIRECTION", viewDirStr.c_str()),
			ShaderMacro("NUM_GRID_CELLS_X", numGridCellsXStr.c_str()),
			ShaderMacro("NUM_GRID_CELLS_Y", numGridCellsYStr.c_str()),
			ShaderMacro("NUM_GRID_CELLS_Z", numGridCellsZStr.c_str()),
			ShaderMacro()
		};
		Shader vertexShader(L"Shaders//VisualizeIntensityVS.hlsl", "Main", "vs_5_1", shaderDefines);

		GraphicsPipelineStateDesc pipelineStateDesc;
		pipelineStateDesc.SetRootSignature(m_pRootSignature);
		pipelineStateDesc.SetVertexShader(&vertexShader);
		pipelineStateDesc.SetPixelShader(&pixelShader);
		pipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
		pipelineStateDesc.SetRenderTargetFormat(pParams->m_RTVFormat);

		m_pHorizontalBoundaryState = new PipelineState(pRenderEnv->m_pDevice, &pipelineStateDesc, L"VisualizeIntensityPass::m_pHorizontalBoundaryState");
	}
	{
		const ShaderMacro shaderDefines[] =
		{
			ShaderMacro("VERTICAL_BOUNDARY", "1"),
			ShaderMacro("VIEW_DIRECTION", viewDirStr.c_str()),
			ShaderMacro("NUM_GRID_CELLS_X", numGridCellsXStr.c_str()),
			ShaderMacro("NUM_GRID_CELLS_Y", numGridCellsYStr.c_str()),
			ShaderMacro("NUM_GRID_CELLS_Z", numGridCellsZStr.c_str()),
			ShaderMacro()
		};
		Shader vertexShader(L"Shaders//VisualizeIntensityVS.hlsl", "Main", "vs_5_1", shaderDefines);

		GraphicsPipelineStateDesc pipelineStateDesc;
		pipelineStateDesc.SetRootSignature(m_pRootSignature);
		pipelineStateDesc.SetVertexShader(&vertexShader);
		pipelineStateDesc.SetPixelShader(&pixelShader);
		pipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
		pipelineStateDesc.SetRenderTargetFormat(pParams->m_RTVFormat);

		m_pVerticalBoundaryState = new PipelineState(pRenderEnv->m_pDevice, &pipelineStateDesc, L"VisualizeIntensityPass::m_pVerticalBoundaryState");
	}
	{
		const ShaderMacro shaderDefines[] =
		{
			ShaderMacro("INTENSITY_DISTRIBUTION", "1"),
			ShaderMacro("VIEW_DIRECTION", viewDirStr.c_str()),
			ShaderMacro("NUM_GRID_CELLS_X", numGridCellsXStr.c_str()),
			ShaderMacro("NUM_GRID_CELLS_Y", numGridCellsYStr.c_str()),
			ShaderMacro("NUM_GRID_CELLS_Z", numGridCellsZStr.c_str()),
			ShaderMacro("SLICE_TO_VISUALIZE", sliceToVisualizeStr.c_str()),
			ShaderMacro("NUM_INTENSITY_POLYGON_SIDES", numIntensityPolygonSidesStr.c_str()),
			ShaderMacro()
		};
		Shader vertexShader(L"Shaders//VisualizeIntensityVS.hlsl", "Main", "vs_5_1", shaderDefines);

		GraphicsPipelineStateDesc pipelineStateDesc;
		pipelineStateDesc.SetRootSignature(m_pRootSignature);
		pipelineStateDesc.SetVertexShader(&vertexShader);
		pipelineStateDesc.SetPixelShader(&pixelShader);
		pipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
		pipelineStateDesc.SetRenderTargetFormat(pParams->m_RTVFormat);

		m_pIntensityDistributionState = new PipelineState(pRenderEnv->m_pDevice, &pipelineStateDesc, L"VisualizeIntensityPass::m_pIntensityDistributionState");
	}
}

VisualizeIntensityPass::~VisualizeIntensityPass()
{
	SafeDelete(m_pVerticalBoundaryState);
	SafeDelete(m_pHorizontalBoundaryState);
	SafeDelete(m_pIntensityDistributionState);
	SafeDelete(m_pRootSignature);
}

void VisualizeIntensityPass::Record(RenderParams* pParams)
{
	assert(false && "Kolya. Fix me");
	/*
	RenderEnv* pRenderEnv = pParams->m_pRenderEnv;
	CommandList* pCommandList = pParams->m_pCommandList;
	ResourceList* pResources = pParams->m_pResources;

	pCommandList->Begin();

	pCommandList->SetGraphicsRootSignature(m_pRootSignature);
	pCommandList->SetRequiredResourceStates(&pResources->m_RequiredResourceStates);
	pCommandList->SetDescriptorHeaps(pRenderEnv->m_pShaderVisibleSRVHeap);
	pCommandList->SetGraphicsRootDescriptorTable(kSRVRootParam, pResources->m_SRVHeapStart);
	pCommandList->SetGraphicsRoot32BitConstant(kConstant32BitRootParam, pParams->m_TextureIndexToVisualize, 0);
	
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
	*/
}
