#pragma once

#include "D3DWrapper/Common.h"

struct RenderEnv;
struct Viewport;
class CommandList;
class RootSignature;
class PipelineState;

class VisualizeIntensityPass
{
public:
	enum ViewDirection
	{
		ViewDirection_WorldSpaceX = 1,
		ViewDirection_WorldSpaceY = 2,
		ViewDirection_WorldSpaceZ = 3
	};
	struct InitParams
	{
		RenderEnv* m_pRenderEnv;
		u16 m_NumGridCellsX;
		u16 m_NumGridCellsY;
		u16 m_NumGridCellsZ;
		ViewDirection m_ViewDir;
		u16 m_SliceToVisualize;
		u16 m_NumIntensitySamples;
		DXGI_FORMAT m_RTVFormat;
	};
	struct RenderParams
	{
		RenderEnv* m_pRenderEnv;
		CommandList* m_pCommandList;
		UINT m_TextureIndexToVisualize;
		Viewport* m_pViewport;
	};

	VisualizeIntensityPass(InitParams* pParams);
	~VisualizeIntensityPass();

	void Record(RenderParams* pParams);

private:
	RootSignature* m_pRootSignature;
	PipelineState* m_pVerticalBoundaryState;
	PipelineState* m_pHorizontalBoundaryState;
	PipelineState* m_pIntensityDistributionState;

	u32 m_NumGridCellsPerSlice;
	u16 m_NumIntensityVerticesPerCell;
	u16 m_NumHorizontalBoundariesPerSlice;
	u16 m_NumVerticalBoundariesPerSlice;
};