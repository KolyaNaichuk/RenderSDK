#pragma once

#include "D3DWrapper/Common.h"

class GraphicsDevice;
class PipelineState;
class RootSignature;
class CommandList;
class CommandAllocator;
class GraphicsResource;
class DescriptorHeap;

class CalcTextureLuminancePass
{
public:
	CalcTextureLuminancePass(GraphicsDevice* pDevice, DXGI_FORMAT rtvFormat, bool logLuminance = false);
	~CalcTextureLuminancePass();

	void Record(CommandList* pCommandList, CommandAllocator* pCommandAllocator,
		GraphicsResource* pRTVTexture, D3D12_CPU_DESCRIPTOR_HANDLE rtvDescriptor,
		DescriptorHeap* pSRVDescriptorHeap, GraphicsResource* pSRVTexture, D3D12_GPU_DESCRIPTOR_HANDLE srvDescriptor,
		DescriptorHeap* pSamplerDescriptorHeap, D3D12_GPU_DESCRIPTOR_HANDLE samplerDescriptor,
		const D3D12_RESOURCE_STATES* pRTVEndState = nullptr, const D3D12_RESOURCE_STATES* pSRVEndState = nullptr);

private:
	RootSignature* m_pRootSignature;
	PipelineState* m_pPipelineState;
};