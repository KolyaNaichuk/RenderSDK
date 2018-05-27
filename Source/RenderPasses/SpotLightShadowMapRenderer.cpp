#include "RenderPasses/SpotLightShadowMapRenderer.h"
#include "RenderPasses/Utils.h"
#include "D3DWrapper/RenderEnv.h"
#include "D3DWrapper/CommandSignature.h"
#include "Math/Frustum.h"
#include "Math/OverlapTest.h"
#include "Scene/Light.h"
#include "Scene/MeshBatch.h"

namespace
{
	struct ShadowMapCommand
	{
		UINT m_InstanceOffset;
		DrawIndexedArguments m_Args;
	};
}

SpotLightShadowMapRenderer::SpotLightShadowMapRenderer(InitParams* pParams)
{
	InitResources(pParams);
	InitStaticMeshCommands(pParams);
}

SpotLightShadowMapRenderer::~SpotLightShadowMapRenderer()
{
	SafeDelete(m_pActiveShadowMaps);
	SafeDelete(m_pStaticMeshCommandBuffer);
	SafeDelete(m_pStaticMeshInstanceIndexBuffer);
	SafeDelete(m_pSpotLightShadowMaps);
	SafeDelete(m_pSpotLightViewProjMatrixBuffer);
	SafeDelete(m_pCreateExpShadowMapParamsBuffer);
}

void SpotLightShadowMapRenderer::Record(RenderParams* pParams)
{
	assert(pParams->m_NumActiveSpotLights <= m_SpotLightShadowMapStates.size());

	u32 numOutdatedShadowMaps = 0;
	for (u32 it = 0; it < pParams->m_NumActiveSpotLights; ++it)
	{
		u32 activeLightIndex = pParams->m_ActiveSpotLightIndices[it];
		if (m_SpotLightShadowMapStates[activeLightIndex] == ShadowMapState::Outdated)
			m_OutdatedSpotLightShadowMapIndices[numOutdatedShadowMaps++] = activeLightIndex;
	}
	assert(false);
}

void SpotLightShadowMapRenderer::InitResources(InitParams* pParams)
{
	RenderEnv* pRenderEnv = pParams->m_pRenderEnv;
	
	m_OutputResourceStates.m_SpotLightShadowMapsState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;

	assert(m_pActiveShadowMaps == nullptr);
	const DepthStencilValue optimizedClearDepth(1.0f);
	DepthTexture2DDesc activeShadowMapsDesc(DXGI_FORMAT_R32_TYPELESS, pParams->m_StandardShadowMapSize, pParams->m_StandardShadowMapSize,
		true/*createDSV*/, true/*createSRV*/, 1/*mipLevels*/, pParams->m_MaxNumActiveSpotLights/*arraySize*/);
	m_pActiveShadowMaps = new DepthTexture(pRenderEnv, pRenderEnv->m_pDefaultHeapProps, &activeShadowMapsDesc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE, &optimizedClearDepth, L"SpotLightShadowMapRenderer::m_pActiveShadowMaps");
	
	assert(m_pSpotLightShadowMaps == nullptr);
	u32 expShadowMapSize = pParams->m_Downscale2XExpShadowMap ? (pParams->m_StandardShadowMapSize / 2) : pParams->m_StandardShadowMapSize;
	ColorTexture2DDesc shadowMapsDesc(DXGI_FORMAT_R32_FLOAT, expShadowMapSize, expShadowMapSize,
		false/*createRTV*/, true/*createSRV*/, true/*createUAV*/, 1/*mipLevels*/, pParams->m_NumSpotLights/*arraySize*/);
	m_pSpotLightShadowMaps = new ColorTexture(pRenderEnv, pRenderEnv->m_pDefaultHeapProps, &shadowMapsDesc,
		pParams->m_InputResourceStates.m_SpotLightShadowMapsState, nullptr/*optimizedClearColor*/, L"SpotLightShadowMapRenderer::m_pShadowMaps");

	m_SpotLightShadowMapStates.resize(pParams->m_NumSpotLights);
	for (u32 lightIndex = 0; lightIndex < pParams->m_NumSpotLights; ++lightIndex)
		m_SpotLightShadowMapStates[lightIndex] = ShadowMapState::Outdated;

	m_OutdatedSpotLightShadowMapIndices.resize(pParams->m_MaxNumActiveSpotLights);
}

void SpotLightShadowMapRenderer::InitStaticMeshCommands(InitParams* pParams)
{
	RenderEnv* pRenderEnv = pParams->m_pRenderEnv;
	
	assert(pParams->m_NumStaticMeshTypes == 1);
	u32 staticMeshType = 0;
	MeshBatch* pStaticMeshBatch = pParams->m_ppStaticMeshBatches[staticMeshType];
		
	const u32 numMeshes = pStaticMeshBatch->GetNumMeshes();
	const MeshInfo* meshInfos = pStaticMeshBatch->GetMeshInfos();
	const AxisAlignedBox* meshInstanceWorldAABBs = pStaticMeshBatch->GetMeshInstanceWorldAABBs();

	std::vector<u32> meshInstanceIndices;
	std::vector<ShadowMapCommand> staticMeshCommands;
	assert(m_StaticMeshCommandRanges.empty());

	for (u32 lightIndex = 0; lightIndex < pParams->m_NumSpotLights; ++lightIndex)
	{
		const SpotLight* pLight = pParams->m_ppSpotLights[lightIndex];

		const Transform& lightWorldSpaceTransform = pLight->GetTransform();
		const Vector3f& lightWorldSpacePos = lightWorldSpaceTransform.GetPosition();
		const BasisAxes lightWorldSpaceBasis = ExtractBasisAxes(lightWorldSpaceTransform.GetRotation());

		assert(IsNormalized(lightWorldSpaceBasis.m_ZAxis));
		const Vector3f& lightWorldSpaceDir = lightWorldSpaceBasis.m_ZAxis;
		assert(IsNormalized(lightWorldSpaceBasis.m_YAxis));
		const Vector3f& lightWorldSpaceUpDir = lightWorldSpaceBasis.m_YAxis;

		const Matrix4f viewMatrix = CreateLookAtMatrix(lightWorldSpacePos, lightWorldSpacePos + lightWorldSpaceDir, lightWorldSpaceUpDir);
		const Matrix4f projMatrix = CreatePerspectiveFovProjMatrix(pLight->GetOuterConeAngle(), 1.0f, pLight->GetShadowNearPlane(), pLight->GetRange());
		const Matrix4f viewProjMatrix = viewMatrix * projMatrix;
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
					meshInstanceIndices.push_back(meshInstanceIndex);
				}
			}
			if (numVisibleMeshInstances > 0)
			{
				ShadowMapCommand shadowMapCommand;
				shadowMapCommand.m_InstanceOffset = meshInstanceIndices.size() - numVisibleMeshInstances;
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

	assert(m_pStaticMeshCommandBuffer == nullptr);	
	StructuredBufferDesc staticMeshCommandBufferDesc(staticMeshCommands.size(), sizeof(ShadowMapCommand), false/*createSRV*/, false/*createUAV*/);
	m_pStaticMeshCommandBuffer = new Buffer(pRenderEnv, pRenderEnv->m_pDefaultHeapProps, &staticMeshCommandBufferDesc,
		D3D12_RESOURCE_STATE_COPY_DEST, L"SpotLightShadowMapRenderer::m_pStaticMeshCommandBuffer");
	
	UploadData(pRenderEnv, m_pStaticMeshCommandBuffer, staticMeshCommandBufferDesc,
		D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT, staticMeshCommands.data(), staticMeshCommands.size() * sizeof(staticMeshCommands[0]));

	assert(m_pStaticMeshInstanceIndexBuffer == nullptr);
	FormattedBufferDesc staticMeshInstanceIndexBufferDesc(meshInstanceIndices.size(), DXGI_FORMAT_R32_UINT, true/*createSRV*/, false/*createUAV*/);
	m_pStaticMeshInstanceIndexBuffer = new Buffer(pRenderEnv, pRenderEnv->m_pDefaultHeapProps, &staticMeshInstanceIndexBufferDesc,
		D3D12_RESOURCE_STATE_COPY_DEST, L"SpotLightShadowMapRenderer::m_pStaticMeshInstanceIndexBuffer");

	UploadData(pRenderEnv, m_pStaticMeshInstanceIndexBuffer, staticMeshInstanceIndexBufferDesc,
		D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, meshInstanceIndices.data(), meshInstanceIndices.size() * sizeof(meshInstanceIndices[0]));
}
