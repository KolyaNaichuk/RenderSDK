#pragma once

#include "D3DWrapper/Common.h"

class RootSignature;
class PipelineState;
class CommandSignature;
class CommandList;
class Buffer;
class MeshBatch;

struct Viewport;
struct RenderEnv;
struct ResourceList;

class RenderGBufferPass
{
public:
	enum ShaderFlags
	{
		ShaderFlag_HasTexCoords = 1 << 0,
		ShaderFlag_HasDiffuseMap = 1 << 1,
		ShaderFlag_HasSpecularMap = 1 << 2,
		ShaderFlag_HasSpecularPowerMap = 1 << 3
	};
	struct InitParams
	{
		RenderEnv* m_pRenderEnv;
		DXGI_FORMAT m_NormalRTVFormat;
		DXGI_FORMAT m_DiffuseRTVFormat;
		DXGI_FORMAT m_SpecularRTVFormat;
		DXGI_FORMAT m_DSVFormat;
		u8 m_ShaderFlags;
		MeshBatch* m_pMeshBatch;
	};
	struct RenderParams
	{
		RenderEnv* m_pRenderEnv;
		CommandList* m_pCommandList;
		ResourceList* m_pResources;
		Viewport* m_pViewport;
		MeshBatch* m_pMeshBatch;
		Buffer* m_pDrawMeshCommandBuffer;
		Buffer* m_pNumDrawMeshesBuffer;
	};

	RenderGBufferPass(InitParams* pParams);
	~RenderGBufferPass();

	void Record(RenderParams* pParams);

private:
	RootSignature* m_pRootSignature;
	PipelineState* m_pPipelineState;
	CommandSignature* m_pCommandSignature;
};
