#pragma once

#include "D3DWrapper/Common.h"

class CommandList;
class RootSignature;
class PipelineState;

struct RenderEnv;
struct BindingResourceList;
struct Viewport;

class VisualizeIntensityPass
{
public:
	enum ViewDirection
	{
		ViewDirection_Z = 1,
		ViewDirection_Y = 2,
		ViewDirection_X = 3
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
		BindingResourceList* m_pResources;
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