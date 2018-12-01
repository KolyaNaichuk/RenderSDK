#include "RenderPasses/SpotLightShadowMapRenderer.h"
#include "RenderPasses/RenderSpotLightShadowMapPass.h"
#include "RenderPasses/CreateExpShadowMapPass.h"
#include "RenderPasses/FilterExpShadowMapPass.h"
#include "RenderPasses/Utils.h"
#include "RenderPasses/MeshRenderResources.h"
#include "D3DWrapper/RenderEnv.h"
#include "D3DWrapper/CommandSignature.h"
#include "Math/Frustum.h"
#include "Math/OverlapTest.h"
#include "Math/Transform.h"
#include "Scene/Light.h"
#include "Scene/MeshBatch.h"

SpotLightShadowMapRenderer::SpotLightShadowMapRenderer(InitParams* pParams)
{
	InitResources(pParams);	
	InitRenderSpotLightShadowMapPass(pParams);
	InitCreateExpShadowMapPass(pParams);
	InitFilterExpShadowMapPass(pParams);
}

SpotLightShadowMapRenderer::~SpotLightShadowMapRenderer()
{
	SafeDelete(m_pRenderSpotLightShadowMapPass);
	SafeDelete(m_pCreateExpShadowMapPass);
	SafeDelete(m_pFilterExpShadowMapPass);
	SafeDelete(m_pActiveShadowMaps);
	SafeDelete(m_pStaticMeshCommandBuffer);
	SafeDelete(m_pStaticMeshInstanceIndexBuffer);
	SafeDelete(m_pSpotLightShadowMaps);
	SafeDelete(m_pSpotLightViewProjMatrixBuffer);
	SafeDelete(m_pCreateExpShadowMapParamsBuffer);
}

void SpotLightShadowMapRenderer::Record(RenderParams* pParams)
{
	assert(pParams->m_NumActiveSpotLights <= m_OutdatedSpotLightShadowMapIndices.size());

	u32 numOutdatedShadowMaps = 0;
	for (u32 it = 0; it < pParams->m_NumActiveSpotLights; ++it)
	{
		u32 activeLightIndex = pParams->m_ActiveSpotLightIndices[it];
		if (m_SpotLightShadowMapStates[activeLightIndex] == ShadowMapState::Outdated)
			m_OutdatedSpotLightShadowMapIndices[numOutdatedShadowMaps++] = activeLightIndex;
	}
	
	CommandList* pCommandList = pParams->m_pCommandList;
	pCommandList->Begin();

	for (u32 it = 0; it < numOutdatedShadowMaps; ++it)
	{
		const u32 shadowMapIndex = m_OutdatedSpotLightShadowMapIndices[it];
		const CommandRange& commandRange = m_StaticMeshCommandRanges[shadowMapIndex];

		{
			RenderSpotLightShadowMapPass::RenderParams params;
			params.m_pRenderEnv = pParams->m_pRenderEnv;
			params.m_pCommandList = pCommandList;
			params.m_pMeshRenderResources = pParams->m_pStaticMeshRenderResources;
			params.m_pSpotLightShadowMaps = m_pActiveShadowMaps;
			params.m_pRenderCommandBuffer = m_pStaticMeshCommandBuffer;
			params.m_FirstRenderCommand = commandRange.m_FirstCommand;
			params.m_NumRenderCommands = commandRange.m_NumCommands;
			params.m_SpotLightIndex = shadowMapIndex;
			params.m_ShadowMapIndex = it;

			m_pRenderSpotLightShadowMapPass->Record(&params);
		}
		{
			CreateExpShadowMapPass::RenderParams params;
			params.m_pRenderEnv = pParams->m_pRenderEnv;
			params.m_pCommandList = pCommandList;
			params.m_pStandardShadowMaps = m_pActiveShadowMaps;
			params.m_pExpShadowMaps = m_pSpotLightShadowMaps;
			params.m_StandardShadowMapIndex = it;
			params.m_ExpShadowMapIndex = shadowMapIndex;

			m_pCreateExpShadowMapPass->Record(&params);
		}
		{
			FilterExpShadowMapPass::RenderParams params;
			params.m_pRenderEnv = pParams->m_pRenderEnv;
			params.m_pCommandList = pCommandList;
			params.m_pExpShadowMaps = m_pSpotLightShadowMaps;
			params.m_ExpShadowMapIndex = shadowMapIndex;
			params.m_IntermediateResultIndex = it;

			m_pFilterExpShadowMapPass->Record(&params);
		}
				
		m_SpotLightShadowMapStates[shadowMapIndex] = ShadowMapState::UpToDate;
	}

	pCommandList->End();
}

void SpotLightShadowMapRenderer::InitResources(InitParams* pParams)
{
	RenderEnv* pRenderEnv = pParams->m_pRenderEnv;
	m_OutputResourceStates.m_SpotLightShadowMapsState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;

	assert(m_pActiveShadowMaps == nullptr);
	const DepthStencilValue optimizedClearDepth(1.0f);
	DepthTexture2DDesc activeShadowMapsDesc(DXGI_FORMAT_R32_TYPELESS, pParams->m_ShadowMapSize, pParams->m_ShadowMapSize,
		true/*createDSV*/, true/*createSRV*/, 1/*mipLevels*/, pParams->m_MaxNumActiveSpotLights/*arraySize*/);
	m_pActiveShadowMaps = new DepthTexture(pRenderEnv, pRenderEnv->m_pDefaultHeapProps, &activeShadowMapsDesc,
		D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, &optimizedClearDepth, L"SpotLightShadowMapRenderer::m_pActiveShadowMaps");
	
	assert(m_pSpotLightShadowMaps == nullptr);
	ColorTexture2DDesc shadowMapsDesc(DXGI_FORMAT_R32_FLOAT, pParams->m_ShadowMapSize, pParams->m_ShadowMapSize,
		false/*createRTV*/, true/*createSRV*/, true/*createUAV*/, 1/*mipLevels*/, pParams->m_NumSpotLights/*arraySize*/);
	m_pSpotLightShadowMaps = new ColorTexture(pRenderEnv, pRenderEnv->m_pDefaultHeapProps, &shadowMapsDesc,
		pParams->m_InputResourceStates.m_SpotLightShadowMapsState, nullptr/*optimizedClearColor*/, L"SpotLightShadowMapRenderer::m_pShadowMaps");

	m_SpotLightShadowMapStates.resize(pParams->m_NumSpotLights);
	for (u32 lightIndex = 0; lightIndex < pParams->m_NumSpotLights; ++lightIndex)
		m_SpotLightShadowMapStates[lightIndex] = ShadowMapState::Outdated;
	m_OutdatedSpotLightShadowMapIndices.resize(pParams->m_MaxNumActiveSpotLights);

	assert(pParams->m_NumStaticMeshTypes == 1);
	u32 staticMeshType = 0;
	MeshBatch* pStaticMeshBatch = pParams->m_ppStaticMeshBatches[staticMeshType];

	const u32 numMeshes = pStaticMeshBatch->GetNumMeshes();
	const MeshInfo* meshInfos = pStaticMeshBatch->GetMeshInfos();
	const AxisAlignedBox* meshInstanceWorldAABBs = pStaticMeshBatch->GetMeshInstanceWorldAABBs();

	std::vector<u32> visibleMeshInstanceIndices;
	std::vector<ShadowMapCommand> staticMeshCommands;
	assert(m_StaticMeshCommandRanges.empty());

	std::vector<Matrix4f> spotLightViewProjMatrices(pParams->m_NumSpotLights);
	std::vector<CreateExpShadowMapParams> createExpShadowMapParams(pParams->m_NumSpotLights);
	
	for (u32 lightIndex = 0; lightIndex < pParams->m_NumSpotLights; ++lightIndex)
	{
		const SpotLight* pLight = pParams->m_ppSpotLights[lightIndex];
				
		const Matrix4f viewMatrix = CreateLookAtMatrix(pLight->GetWorldPosition(), pLight->GetWorldOrientation());
		const Matrix4f projMatrix = CreatePerspectiveFovProjMatrix(pLight->GetOuterConeAngle(), 1.0f, pLight->GetShadowNearPlane(), pLight->GetRange());

		const Matrix4f viewProjMatrix = viewMatrix * projMatrix;
		spotLightViewProjMatrices[lightIndex] = viewProjMatrix;

		createExpShadowMapParams[lightIndex].m_LightProjMatrix43 = projMatrix.m_32;
		createExpShadowMapParams[lightIndex].m_LightProjMatrix33 = projMatrix.m_22;
		createExpShadowMapParams[lightIndex].m_LightViewNearPlane = pLight->GetShadowNearPlane();
		createExpShadowMapParams[lightIndex].m_LightRcpViewClipRange = Rcp(pLight->GetRange() - pLight->GetShadowNearPlane());
		createExpShadowMapParams[lightIndex].m_ExpShadowMapConstant = pLight->GetExpShadowMapConstant();

		const Frustum lightWorldFrustum(viewProjMatrix);

		CommandRange commandRange;
		commandRange.m_FirstCommand = staticMeshCommands.size();
		commandRange.m_NumCommands = 0;

		for (u32 meshIndex = 0; meshIndex < numMeshes; ++meshIndex)
		{
			u32 numVisibleMeshInstances = 0;

			const MeshInfo& meshInfo = meshInfos[meshIndex];
			const u32 meshLastInstanceIndex = meshInfo.m_InstanceOffset + meshInfo.m_InstanceCount;

			for (u32 meshInstanceIndex = meshInfo.m_InstanceOffset; meshInstanceIndex < meshLastInstanceIndex; ++meshInstanceIndex)
			{
				const AxisAlignedBox& meshInstanceWorldAABB = meshInstanceWorldAABBs[meshInstanceIndex];
				if (TestAABBAgainstFrustum(lightWorldFrustum, meshInstanceWorldAABB))
				{
					++numVisibleMeshInstances;
					visibleMeshInstanceIndices.push_back(meshInstanceIndex);
				}
			}
			if (numVisibleMeshInstances > 0)
			{
				ShadowMapCommand shadowMapCommand;
				shadowMapCommand.m_InstanceOffset = visibleMeshInstanceIndices.size() - numVisibleMeshInstances;
				shadowMapCommand.m_Args.m_IndexCountPerInstance = meshInfo.m_IndexCountPerInstance;
				shadowMapCommand.m_Args.m_InstanceCount = numVisibleMeshInstances;
				shadowMapCommand.m_Args.m_StartIndexLocation = meshInfo.m_StartIndexLocation;
				shadowMapCommand.m_Args.m_BaseVertexLocation = meshInfo.m_BaseVertexLocation;
				shadowMapCommand.m_Args.m_StartInstanceLocation = 0;

				staticMeshCommands.push_back(shadowMapCommand);
			}
		}

		commandRange.m_NumCommands = staticMeshCommands.size() - commandRange.m_FirstCommand;
		m_StaticMeshCommandRanges.push_back(commandRange);
	}

	assert(m_pSpotLightViewProjMatrixBuffer == nullptr);
	StructuredBufferDesc spotLightViewProjMatrixBufferDesc(spotLightViewProjMatrices.size(), sizeof(spotLightViewProjMatrices[0]), true/*createSRV*/, false/*createUAV*/);
	m_pSpotLightViewProjMatrixBuffer = new Buffer(pRenderEnv, pRenderEnv->m_pDefaultHeapProps, &spotLightViewProjMatrixBufferDesc,
		D3D12_RESOURCE_STATE_COPY_DEST, L"SpotLightShadowMapRenderer::m_pSpotLightViewProjMatrixBuffer");
		
	UploadData(pRenderEnv, m_pSpotLightViewProjMatrixBuffer, spotLightViewProjMatrixBufferDesc,
		D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, spotLightViewProjMatrices.data(), spotLightViewProjMatrices.size() * sizeof(spotLightViewProjMatrices[0]));

	assert(m_pCreateExpShadowMapParamsBuffer == nullptr);
	StructuredBufferDesc createExpShadowMapParamsBufferDesc(createExpShadowMapParams.size(), sizeof(createExpShadowMapParams[0]), true/*createSRV*/, false/*createUAV*/);
	m_pCreateExpShadowMapParamsBuffer = new Buffer(pRenderEnv, pRenderEnv->m_pDefaultHeapProps, &createExpShadowMapParamsBufferDesc,
		D3D12_RESOURCE_STATE_COPY_DEST, L"SpotLightShadowMapRenderer::m_pCreateExpShadowMapParamsBuffer");

	UploadData(pRenderEnv, m_pCreateExpShadowMapParamsBuffer, createExpShadowMapParamsBufferDesc,
		D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, createExpShadowMapParams.data(), createExpShadowMapParams.size() * sizeof(createExpShadowMapParams[0]));

	assert(m_pStaticMeshCommandBuffer == nullptr);
	StructuredBufferDesc staticMeshCommandBufferDesc(staticMeshCommands.size(), sizeof(staticMeshCommands[0]), false/*createSRV*/, false/*createUAV*/);
	m_pStaticMeshCommandBuffer = new Buffer(pRenderEnv, pRenderEnv->m_pDefaultHeapProps, &staticMeshCommandBufferDesc,
		D3D12_RESOURCE_STATE_COPY_DEST, L"SpotLightShadowMapRenderer::m_pStaticMeshCommandBuffer");

	UploadData(pRenderEnv, m_pStaticMeshCommandBuffer, staticMeshCommandBufferDesc,
		D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT, staticMeshCommands.data(), staticMeshCommands.size() * sizeof(staticMeshCommands[0]));

	assert(m_pStaticMeshInstanceIndexBuffer == nullptr);
	FormattedBufferDesc staticMeshInstanceIndexBufferDesc(visibleMeshInstanceIndices.size(), DXGI_FORMAT_R32_UINT, true/*createSRV*/, false/*createUAV*/);
	m_pStaticMeshInstanceIndexBuffer = new Buffer(pRenderEnv, pRenderEnv->m_pDefaultHeapProps, &staticMeshInstanceIndexBufferDesc,
		D3D12_RESOURCE_STATE_COPY_DEST, L"SpotLightShadowMapRenderer::m_pStaticMeshInstanceIndexBuffer");

	UploadData(pRenderEnv, m_pStaticMeshInstanceIndexBuffer, staticMeshInstanceIndexBufferDesc,
		D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, visibleMeshInstanceIndices.data(), visibleMeshInstanceIndices.size() * sizeof(visibleMeshInstanceIndices[0]));
}

void SpotLightShadowMapRenderer::InitRenderSpotLightShadowMapPass(InitParams* pParams)
{
	assert(m_pRenderSpotLightShadowMapPass == nullptr);
	
	RenderSpotLightShadowMapPass::InitParams params;
	params.m_pName = "RenderSpotLightShadowMapPass";
	params.m_pRenderEnv = pParams->m_pRenderEnv;
	
	params.m_InputResourceStates.m_RenderCommandBufferState = D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT;
	params.m_InputResourceStates.m_MeshInstanceIndexBufferState = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
	params.m_InputResourceStates.m_MeshInstanceWorldMatrixBufferState = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
	params.m_InputResourceStates.m_SpotLightViewProjMatrixBufferState = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
	params.m_InputResourceStates.m_SpotLightShadowMapsState = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
		
	params.m_pMeshRenderResources = pParams->m_pStaticMeshRenderResources;
	params.m_pRenderCommandBuffer = m_pStaticMeshCommandBuffer;
	params.m_pMeshInstanceIndexBuffer = m_pStaticMeshInstanceIndexBuffer;
	params.m_pMeshInstanceWorldMatrixBuffer = pParams->m_pStaticMeshRenderResources->GetInstanceWorldMatrixBuffer();
	params.m_pSpotLightViewProjMatrixBuffer = m_pSpotLightViewProjMatrixBuffer;
	params.m_pSpotLightShadowMaps = m_pActiveShadowMaps;

	m_pRenderSpotLightShadowMapPass = new RenderSpotLightShadowMapPass(&params);
}

void SpotLightShadowMapRenderer::InitCreateExpShadowMapPass(InitParams* pParams)
{
	assert(m_pCreateExpShadowMapPass == nullptr);
	assert(m_pRenderSpotLightShadowMapPass != nullptr);

	const RenderSpotLightShadowMapPass::ResourceStates* pRenderSpotLightShadowMapPassStates =
		m_pRenderSpotLightShadowMapPass->GetOutputResourceStates();
	
	CreateExpShadowMapPass::InitParams params;
	params.m_pName = "CreateExpShadowMapPass";
	params.m_pRenderEnv = pParams->m_pRenderEnv;
	
	params.m_InputResourceStates.m_StandardShadowMapsState = pRenderSpotLightShadowMapPassStates->m_SpotLightShadowMapsState;
	params.m_InputResourceStates.m_CreateExpShadowMapParamsBufferState = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
	params.m_InputResourceStates.m_ExpShadowMapsState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	
	params.m_pStandardShadowMaps = m_pActiveShadowMaps;
	params.m_pCreateExpShadowMapParamsBuffer = m_pCreateExpShadowMapParamsBuffer;
	params.m_pExpShadowMaps = m_pSpotLightShadowMaps;

	m_pCreateExpShadowMapPass = new CreateExpShadowMapPass(&params);
}

void SpotLightShadowMapRenderer::InitFilterExpShadowMapPass(InitParams* pParams)
{
	assert(m_pFilterExpShadowMapPass == nullptr);
	assert(m_pCreateExpShadowMapPass != nullptr);

	const CreateExpShadowMapPass::ResourceStates* pCreateExpShadowMapPassStates =
		m_pCreateExpShadowMapPass->GetOutputResourceStates();

	FilterExpShadowMapPass::InitParams params;
	params.m_pName = "FilterExpShadowMapPass";
	params.m_pRenderEnv = pParams->m_pRenderEnv;
	params.m_InputResourceStates.m_ExpShadowMapsState = pCreateExpShadowMapPassStates->m_ExpShadowMapsState;
	params.m_MaxNumActiveExpShadowMaps = pParams->m_MaxNumActiveSpotLights;
	params.m_pExpShadowMaps = m_pSpotLightShadowMaps;

	m_pFilterExpShadowMapPass = new FilterExpShadowMapPass(&params);
}