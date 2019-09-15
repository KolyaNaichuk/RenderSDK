#include "RayTracingPass.h"
#include "D3DWrapper/RootSignature.h"
#include "D3DWrapper/PipelineState.h"
#include "D3DWrapper/StateObject.h"
#include "D3DWrapper/GraphicsDevice.h"
#include "D3DWrapper/RenderEnv.h"
#include "D3DWrapper/RayTracing.h"
#include "Profiler/GPUProfiler.h"
#include "Scene/Mesh.h"
#include "Scene/MeshBatch.h"
#include "Math/Vector2.h"
#include "Math/Vector3.h"
#include "Math/Vector4.h"

namespace
{
	LPCWSTR g_pRayGenShaderName = L"RayGeneration";
	LPCWSTR g_pMissShaderName = L"RayMiss";
	LPCWSTR g_pClosesHitShaderName = L"RayClosestHit";
	LPCWSTR g_pHitGroupName = L"HitGroup";

	enum GlobalRootParams
	{
		kGlobalRootCBVParam = 0,
		kGlobalRootTLASParam,
		kGlobalRootSRVTableParam,
		kGlobalNumRootParams
	};
}

RayTracingPass::RayTracingPass(InitParams* pParams)
{
	InitTextures(pParams);
	InitGeometryBuffers(pParams);
	InitAccelerationStructures(pParams);
	InitRootSignatures(pParams);
	InitStateObject(pParams);
	InitDescriptorHeap(pParams);
	InitShaderTables(pParams);
}

RayTracingPass::~RayTracingPass()
{
	SafeDelete(m_pTLASBuffer);
	SafeDelete(m_pBLASBuffer);
	SafeDelete(m_pInstanceBuffer);
	SafeDelete(m_pVertexBuffer);
	SafeDelete(m_pIndexBuffer);
	SafeDelete(m_pGlobalRootSignature);
	SafeDelete(m_pEmptyLocalRootSignature);
	SafeDelete(m_pRayGenShaderTable);
	SafeDelete(m_pMissShaderTable);
	SafeDelete(m_pHitGroupTable);
	SafeDelete(m_pStateObject);
	SafeDelete(m_pRayTracedResult);
}

void RayTracingPass::Record(RenderParams* pParams)
{
	RenderEnv* pRenderEnv = pParams->m_pRenderEnv;
	CommandList* pCommandList = pParams->m_pCommandList;
	GPUProfiler* pGPUProfiler = pRenderEnv->m_pGPUProfiler;

	pCommandList->Begin();
#ifdef ENABLE_PROFILING
	u32 profileIndex = pGPUProfiler->StartProfile(pCommandList, "RayTracingPass");
#endif // ENABLE_PROFILING

	pCommandList->SetPipelineState(m_pStateObject);
	pCommandList->SetDescriptorHeaps(pRenderEnv->m_pShaderVisibleSRVHeap);
	pCommandList->SetComputeRootSignature(m_pGlobalRootSignature);
	pCommandList->SetComputeRootConstantBufferView(kGlobalRootCBVParam, pParams->m_pAppDataBuffer);
	pCommandList->SetComputeRootShaderResourceView(kGlobalRootTLASParam, m_pTLASBuffer);
	pCommandList->SetComputeRootDescriptorTable(kGlobalRootSRVTableParam, m_GlobalSRVHeapStart);
		
	if (!m_ResourceBarriers.empty())
		pCommandList->ResourceBarrier((UINT)m_ResourceBarriers.size(), m_ResourceBarriers.data());
	
	DispatchRaysDesc dispatchDesc(pParams->m_NumRaysX, pParams->m_NumRaysY, 1,
		m_pRayGenShaderTable, m_pMissShaderTable, m_pHitGroupTable);
	pCommandList->DispatchRays(&dispatchDesc);
	
#ifdef ENABLE_PROFILING
	pGPUProfiler->EndProfile(pCommandList, profileIndex);
#endif // ENABLE_PROFILING
	pCommandList->End();
}

void RayTracingPass::InitTextures(InitParams* pParams)
{
	RenderEnv* pRenderEnv = pParams->m_pRenderEnv;
	
	assert(m_pRayTracedResult == nullptr);
	const FLOAT optimizedClearColor[] = {0.0f, 0.0f, 0.0f, 0.0f};
	ColorTexture2DDesc rayTracedResultDesc(DXGI_FORMAT_R10G10B10A2_UNORM, pParams->m_NumRaysX, pParams->m_NumRaysY, false, true, true);
	m_pRayTracedResult = new ColorTexture(pRenderEnv, pRenderEnv->m_pDefaultHeapProps, &rayTracedResultDesc,
		pParams->m_InputResourceStates.m_RayTracedResultState, optimizedClearColor, L"RayTracingPass::m_pRayTracedResult");

	m_OutputResourceStates.m_RayTracedResultState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
	if (pParams->m_InputResourceStates.m_RayTracedResultState != m_OutputResourceStates.m_RayTracedResultState)
		m_ResourceBarriers.emplace_back(m_pRayTracedResult, pParams->m_InputResourceStates.m_RayTracedResultState, m_OutputResourceStates.m_RayTracedResultState);
}

void RayTracingPass::InitGeometryBuffers(InitParams* pParams)
{
	RenderEnv* pRenderEnv = pParams->m_pRenderEnv;
	MeshBatch* pMeshBatch = pParams->m_pMeshBatch;

	// Vertex Buffer

	const u8 vertexFormatFlags = pMeshBatch->GetVertexFormatFlags();
	assert((vertexFormatFlags & VertexData::FormatFlag_Position) != 0);
	
	const Vector3f* pPositions = pMeshBatch->GetPositions();
	const u32 vertexSizeInBytes = sizeof(pPositions[0]);
	
	assert(m_pVertexBuffer == nullptr);
	StructuredBufferDesc vertexBufferDesc(pMeshBatch->GetNumVertices(), vertexSizeInBytes, true, false, true);
	m_pVertexBuffer = new Buffer(pRenderEnv, pRenderEnv->m_pDefaultHeapProps, &vertexBufferDesc,
		D3D12_RESOURCE_STATE_COPY_DEST, L"RayTracingPass::m_pVertexBuffer");

	StructuredBufferDesc uploadVertexBufferDesc(pMeshBatch->GetNumVertices(), vertexSizeInBytes, false, false, false);
	Buffer uploadVertexBuffer(pRenderEnv, pRenderEnv->m_pUploadHeapProps, &uploadVertexBufferDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ, L"RayTracingPass::uploadVertexBuffer");
	uploadVertexBuffer.Write(pMeshBatch->GetPositions(), vertexSizeInBytes * pMeshBatch->GetNumVertices());

	// Index Buffer
	
	u32 indexSizeInBytes = 0;
	const void* pIndexData = nullptr;
	
	if (pMeshBatch->GetIndexFormat() == DXGI_FORMAT_R16_UINT)
	{
		indexSizeInBytes = sizeof(u16);
		pIndexData = pMeshBatch->Get16BitIndices();
	}
	else
	{
		indexSizeInBytes = sizeof(u32);
		pIndexData = pMeshBatch->Get32BitIndices();
	}

	assert(m_pIndexBuffer == nullptr);
	FormattedBufferDesc indexBufferDesc(pMeshBatch->GetNumIndices(), pMeshBatch->GetIndexFormat(), true, false, true);
	m_pIndexBuffer = new Buffer(pRenderEnv, pRenderEnv->m_pDefaultHeapProps, &indexBufferDesc,
		D3D12_RESOURCE_STATE_COPY_DEST, L"RayTracingPass::m_pIndexBuffer");

	FormattedBufferDesc uploadIndexBufferDesc(pMeshBatch->GetNumIndices(), pMeshBatch->GetIndexFormat(), false, false, false);
	Buffer uploadIndexBuffer(pRenderEnv, pRenderEnv->m_pUploadHeapProps, &uploadIndexBufferDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ, L"RayTracingPass::uploadIndexBuffer");
	uploadIndexBuffer.Write(pIndexData, indexSizeInBytes * pMeshBatch->GetNumIndices());

	// Common

	const ResourceTransitionBarrier resourceBarriers[] =
	{
		ResourceTransitionBarrier(m_pVertexBuffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE),
		ResourceTransitionBarrier(m_pIndexBuffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE)
	};

	CommandList* pCommandList = pRenderEnv->m_pCommandListPool->Create(L"RayTracingPass::pUploadVertexDataCommandList");
	pCommandList->Begin();
	pCommandList->CopyResource(m_pVertexBuffer, &uploadVertexBuffer);
	pCommandList->CopyResource(m_pIndexBuffer, &uploadIndexBuffer);
	pCommandList->ResourceBarrier(ARRAYSIZE(resourceBarriers), resourceBarriers);
	pCommandList->End();

	++pRenderEnv->m_LastSubmissionFenceValue;
	pRenderEnv->m_pCommandQueue->ExecuteCommandLists(1, &pCommandList, pRenderEnv->m_pFence, pRenderEnv->m_LastSubmissionFenceValue);
	pRenderEnv->m_pFence->WaitForSignalOnCPU(pRenderEnv->m_LastSubmissionFenceValue);
}

void RayTracingPass::InitAccelerationStructures(InitParams* pParams)
{
	RenderEnv* pRenderEnv = pParams->m_pRenderEnv;
	MeshBatch* pMeshBatch = pParams->m_pMeshBatch;

	// Bottom level acceleration structure

	const VertexBufferView* pVBView = m_pVertexBuffer->GetVBView();
	const IndexBufferView* pIBView = m_pIndexBuffer->GetIBView();
	const u32 indexStrideInBytes = (pMeshBatch->GetIndexFormat() == DXGI_FORMAT_R16_UINT) ? sizeof(u16) : sizeof(u32);

	const u32 numMeshes = pMeshBatch->GetNumMeshes();
	const MeshInfo* meshInfos = pMeshBatch->GetMeshInfos();
	const Matrix4f* instanceWorldMatrices = pMeshBatch->GetMeshInstanceWorldMatrices();

	// Bottom level geometry transforms

	struct Tranform3x4
	{
		f32 m_Values[3][4];
	};

	std::vector<Tranform3x4> geometryTransforms(numMeshes);
	for (u32 index = 0; index < numMeshes; ++index)
	{
		const MeshInfo& meshInfo = meshInfos[index];
		assert(meshInfo.m_InstanceCount == 1);

		const Matrix4f& worldMatrix = instanceWorldMatrices[index];
		Matrix4f transposedWorldMatrix = Transpose(worldMatrix);

		CopyMemory(&geometryTransforms[index], &transposedWorldMatrix, sizeof(Tranform3x4));
	}

	StructuredBufferDesc transformBufferDesc((UINT)geometryTransforms.size(), sizeof(Tranform3x4), true, false);
	Buffer transformBuffer(pRenderEnv, pRenderEnv->m_pDefaultHeapProps, &transformBufferDesc,
		D3D12_RESOURCE_STATE_COPY_DEST, L"RayTracingPass::m_pTransformBuffer");

	StructuredBufferDesc uploadTrasformBufferDesc((UINT)geometryTransforms.size(), sizeof(Tranform3x4), false, false);
	Buffer uploadTransformBuffer(pRenderEnv, pRenderEnv->m_pUploadHeapProps, &uploadTrasformBufferDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ, L"RayTracingPass::uploadTransformBuffer");
	uploadTransformBuffer.Write(geometryTransforms.data(), geometryTransforms.size() * sizeof(Tranform3x4));

	{
		ResourceTransitionBarrier resourceBarrier(&transformBuffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		
		CommandList* pCommandList = pRenderEnv->m_pCommandListPool->Create(L"RayTracingPass::pUploadTransformDataCommandList");
		pCommandList->Begin();
		pCommandList->CopyResource(&transformBuffer, &uploadTransformBuffer);
		pCommandList->ResourceBarrier(1, &resourceBarrier);
		pCommandList->End();

		++pRenderEnv->m_LastSubmissionFenceValue;
		pRenderEnv->m_pCommandQueue->ExecuteCommandLists(1, &pCommandList, pRenderEnv->m_pFence, pRenderEnv->m_LastSubmissionFenceValue);
		pRenderEnv->m_pFence->WaitForSignalOnCPU(pRenderEnv->m_LastSubmissionFenceValue);
	}
	
	// Bottom level geometry descriptions

	std::vector<RayTracingGeometryDesc> geometryDescs(numMeshes);
	for (u32 index = 0; index < numMeshes; ++index)
	{
		const MeshInfo& meshInfo = meshInfos[index];
		assert(meshInfo.m_InstanceCount == 1);
				
		geometryDescs[index].Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
		geometryDescs[index].Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;

		geometryDescs[index].Triangles.Transform3x4 = transformBuffer.GetGPUVirtualAddress() + index * sizeof(Tranform3x4);
		geometryDescs[index].Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
		geometryDescs[index].Triangles.VertexCount = meshInfo.m_VertexCount;
		geometryDescs[index].Triangles.VertexBuffer.StartAddress = pVBView->BufferLocation + pVBView->StrideInBytes * meshInfo.m_BaseVertexLocation;
		geometryDescs[index].Triangles.VertexBuffer.StrideInBytes = pVBView->StrideInBytes;

		geometryDescs[index].Triangles.IndexFormat = pIBView->Format;
		geometryDescs[index].Triangles.IndexCount = meshInfo.m_IndexCount;
		geometryDescs[index].Triangles.IndexBuffer = pIBView->BufferLocation + indexStrideInBytes * meshInfo.m_StartIndexLocation;
	}

	BuildRayTracingAccelerationStructureInputs BLASBuildInputs;
	BLASBuildInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
	BLASBuildInputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
	BLASBuildInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	BLASBuildInputs.NumDescs = (UINT)geometryDescs.size();
	BLASBuildInputs.pGeometryDescs = geometryDescs.data();
		
	RayTracingAccelerationStructurePrebuildInfo BLASPrebuildInfo;
	pRenderEnv->m_pDevice->GetRaytracingAccelerationStructurePrebuildInfo(&BLASBuildInputs, &BLASPrebuildInfo);

	assert(BLASPrebuildInfo.ResultDataMaxSizeInBytes > 0);
	assert(BLASPrebuildInfo.ScratchDataSizeInBytes > 0);
	assert(BLASPrebuildInfo.UpdateScratchDataSizeInBytes == 0);

	assert(m_pBLASBuffer == nullptr);
	RawBufferDesc BLASBufferDesc(BLASPrebuildInfo.ResultDataMaxSizeInBytes, true, true);
	m_pBLASBuffer = new Buffer(pRenderEnv, pRenderEnv->m_pDefaultHeapProps, &BLASBufferDesc,
		D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, L"RayTracingPass::m_pBLASBuffer");

	RawBufferDesc BLASScratchBufferDesc(BLASPrebuildInfo.ScratchDataSizeInBytes, false, true);
	Buffer BLASScratchBuffer(pRenderEnv, pRenderEnv->m_pDefaultHeapProps, &BLASScratchBufferDesc,
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS, L"RayTracingPass::BLASScratchBuffer");

	// Top level instance description

	RayTracingInstanceDesc instanceDesc;
	ZeroMemory(instanceDesc.Transform, sizeof(instanceDesc.Transform));
	instanceDesc.Transform[0][0] = 1.0f;
	instanceDesc.Transform[1][1] = 1.0f;
	instanceDesc.Transform[2][2] = 1.0f;
	instanceDesc.InstanceID = 0;
	instanceDesc.InstanceMask = 1;
	instanceDesc.InstanceContributionToHitGroupIndex = 0;
	instanceDesc.Flags = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;
	instanceDesc.AccelerationStructure = m_pBLASBuffer->GetGPUVirtualAddress();

	assert(m_pInstanceBuffer == nullptr);
	StructuredBufferDesc instanceBufferDesc(1, sizeof(instanceDesc), true, false);
	m_pInstanceBuffer = new Buffer(pRenderEnv, pRenderEnv->m_pDefaultHeapProps, &instanceBufferDesc,
		D3D12_RESOURCE_STATE_COPY_DEST, L"RayTracingPass::m_pInstanceBuffer");

	StructuredBufferDesc uploadInstanceBufferDesc(1, sizeof(instanceDesc), false, false);
	Buffer uploadInstanceBuffer(pRenderEnv, pRenderEnv->m_pUploadHeapProps, &uploadInstanceBufferDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ, L"RayTracingPass::uploadInstanceBuffer");
	uploadInstanceBuffer.Write(&instanceDesc, sizeof(instanceDesc));

	{
		ResourceTransitionBarrier resourceBarrier(m_pInstanceBuffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		
		CommandList* pCommandList = pRenderEnv->m_pCommandListPool->Create(L"RayTracingPass::pUploadInstanceDataCommandList");
		pCommandList->Begin();
		pCommandList->CopyResource(m_pInstanceBuffer, &uploadInstanceBuffer);
		pCommandList->ResourceBarrier(1, &resourceBarrier);
		pCommandList->End();

		++pRenderEnv->m_LastSubmissionFenceValue;
		pRenderEnv->m_pCommandQueue->ExecuteCommandLists(1, &pCommandList, pRenderEnv->m_pFence, pRenderEnv->m_LastSubmissionFenceValue);
		pRenderEnv->m_pFence->WaitForSignalOnCPU(pRenderEnv->m_LastSubmissionFenceValue);
	}
		
	// Top level acceleration structure

	BuildRayTracingAccelerationStructureInputs TLASBuildInputs;
	TLASBuildInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
	TLASBuildInputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
	TLASBuildInputs.NumDescs = 1;
	TLASBuildInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	TLASBuildInputs.InstanceDescs = m_pInstanceBuffer->GetGPUVirtualAddress();

	RayTracingAccelerationStructurePrebuildInfo TLASPrebuildInfo;
	pRenderEnv->m_pDevice->GetRaytracingAccelerationStructurePrebuildInfo(&TLASBuildInputs, &TLASPrebuildInfo);

	assert(TLASPrebuildInfo.ResultDataMaxSizeInBytes > 0);
	assert(TLASPrebuildInfo.ScratchDataSizeInBytes > 0);
	assert(TLASPrebuildInfo.UpdateScratchDataSizeInBytes == 0);

	assert(m_pTLASBuffer == nullptr);
	RawBufferDesc TLASBufferDesc(TLASPrebuildInfo.ResultDataMaxSizeInBytes, true, true);
	m_pTLASBuffer = new Buffer(pRenderEnv, pRenderEnv->m_pDefaultHeapProps, &TLASBufferDesc,
		D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, L"RayTracingPass::m_pTLASBuffer");

	RawBufferDesc TLASScratchBufferDesc(TLASPrebuildInfo.ScratchDataSizeInBytes, false, true);
	Buffer TLASScratchBuffer(pRenderEnv, pRenderEnv->m_pDefaultHeapProps, &TLASScratchBufferDesc,
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS, L"RayTracingPass::TLASScratchBuffer");

	// Common
	
	BuildRayTracingAccelerationStructureDesc buildBLASDesc(&BLASBuildInputs, m_pBLASBuffer, &BLASScratchBuffer);
	BuildRayTracingAccelerationStructureDesc buildTLASDesc(&TLASBuildInputs, m_pTLASBuffer, &TLASScratchBuffer);

	{
		ResourceUAVBarrier resourceBarrier(m_pBLASBuffer);

		CommandList* pCommandList = pRenderEnv->m_pCommandListPool->Create(L"RayTracingPass::pBuildRayTracingASCommandList");
		pCommandList->Begin();
		pCommandList->BuildRaytracingAccelerationStructure(&buildBLASDesc, 0, nullptr);
		pCommandList->ResourceBarrier(1, &resourceBarrier);
		pCommandList->BuildRaytracingAccelerationStructure(&buildTLASDesc, 0, nullptr);
		pCommandList->End();

		++pRenderEnv->m_LastSubmissionFenceValue;
		pRenderEnv->m_pCommandQueue->ExecuteCommandLists(1, &pCommandList, pRenderEnv->m_pFence, pRenderEnv->m_LastSubmissionFenceValue);
		pRenderEnv->m_pFence->WaitForSignalOnCPU(pRenderEnv->m_LastSubmissionFenceValue);
	}
}

void RayTracingPass::InitRootSignatures(InitParams* pParams)
{
	RenderEnv* pRenderEnv = pParams->m_pRenderEnv;

	D3D12_ROOT_PARAMETER globalRootParams[kGlobalNumRootParams];
	globalRootParams[kGlobalRootCBVParam] = RootCBVParameter(0, D3D12_SHADER_VISIBILITY_ALL);
	globalRootParams[kGlobalRootTLASParam] = RootSRVParameter(0, D3D12_SHADER_VISIBILITY_ALL);

	D3D12_DESCRIPTOR_RANGE globalDescriptorRanges[] = {UAVDescriptorRange(1, 0)};
	globalRootParams[kGlobalRootSRVTableParam] = RootDescriptorTableParameter(ARRAYSIZE(globalDescriptorRanges), globalDescriptorRanges, D3D12_SHADER_VISIBILITY_ALL);
	
	assert(m_pGlobalRootSignature == nullptr);
	RootSignatureDesc globalRootSignatureDesc(kGlobalNumRootParams, globalRootParams);
	m_pGlobalRootSignature = new RootSignature(pRenderEnv->m_pDevice, &globalRootSignatureDesc, L"RayTracingPass::m_pGlobalRootSignature");

	assert(m_pEmptyLocalRootSignature == nullptr);
	RootSignatureDesc emptyLocalRootSignatureDesc(D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE);
	m_pEmptyLocalRootSignature = new RootSignature(pRenderEnv->m_pDevice, &emptyLocalRootSignatureDesc, L"RayTracingPass::m_pEmptyLocalRootSignature");
}

void RayTracingPass::InitStateObject(InitParams* pParams)
{
	RenderEnv* pRenderEnv = pParams->m_pRenderEnv;

	enum StateObjectIndices
	{
		kDXILLibraryIndex = 0,
		kHitGroupIndex,
		kPipelineConfigIndex,
		kShaderConfigIndex,
		kShaderConfigAssocIndex,
		kGlobalRootSigIndex,
		kEmptyLocalRootSigIndex,
		kEmptyLocalRootSigAssocIndex,
		kNumStateObjects
	};
	D3D12_STATE_SUBOBJECT stateSubobjects[kNumStateObjects];

	Shader shaderLibrary(L"Shaders//RayTracing.hlsl", L"", L"lib_6_3");
	ExportDesc exportDescs[] = {ExportDesc(g_pRayGenShaderName), ExportDesc(g_pMissShaderName), ExportDesc(g_pClosesHitShaderName)};
	
	DXILLibraryDesc libraryDesc(shaderLibrary.GetBytecode(), ARRAYSIZE(exportDescs), exportDescs);
	stateSubobjects[kDXILLibraryIndex] = StateSubobject(D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY, &libraryDesc);

	HitGroupDesc hitGroupDesc(g_pHitGroupName, D3D12_HIT_GROUP_TYPE_TRIANGLES, nullptr, g_pClosesHitShaderName, nullptr);
	stateSubobjects[kHitGroupIndex] = StateSubobject(D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP, &hitGroupDesc);

	RayTracingPipelineConfig pipelineConfig(1);
	stateSubobjects[kPipelineConfigIndex] = StateSubobject(D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG, &pipelineConfig);

	RayTracingShaderConfig shaderConfig(sizeof(Vector3f)/*color*/, sizeof(Vector2f)/*attribs*/);
	stateSubobjects[kShaderConfigIndex] = StateSubobject(D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG, &shaderConfig);

	LPCWSTR shaderConfigExports[] = {g_pRayGenShaderName, g_pMissShaderName, g_pClosesHitShaderName};
	SubobjectToExportsAssociation shaderConfigExportsAssoc(&stateSubobjects[kShaderConfigIndex], ARRAYSIZE(shaderConfigExports), shaderConfigExports);
	stateSubobjects[kShaderConfigAssocIndex] = StateSubobject(D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION, &shaderConfigExportsAssoc);

	assert(m_pGlobalRootSignature != nullptr);
	GlobalRootSignature globalRootSignature(m_pGlobalRootSignature);
	stateSubobjects[kGlobalRootSigIndex] = StateSubobject(D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE, &globalRootSignature);

	assert(m_pEmptyLocalRootSignature != nullptr);
	LocalRootSignature emptyLocalRootSignature(m_pEmptyLocalRootSignature);
	stateSubobjects[kEmptyLocalRootSigIndex] = StateSubobject(D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE, &emptyLocalRootSignature);

	LPCWSTR emptyLocalRootSigExports[] = {g_pRayGenShaderName, g_pClosesHitShaderName, g_pMissShaderName};
	SubobjectToExportsAssociation emptyLocalRootSigExportsAssoc(&stateSubobjects[kEmptyLocalRootSigIndex], ARRAYSIZE(emptyLocalRootSigExports), emptyLocalRootSigExports);
	stateSubobjects[kEmptyLocalRootSigAssocIndex] = StateSubobject(D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION, &emptyLocalRootSigExportsAssoc);

	assert(m_pStateObject == nullptr);
	StateObjectDesc stateObjectDesc(D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE, kNumStateObjects, stateSubobjects);
	m_pStateObject = new StateObject(pRenderEnv->m_pDevice, &stateObjectDesc, L"RayTracingPass::m_pStateObject");
}

void RayTracingPass::InitDescriptorHeap(InitParams* pParams)
{
	assert(!m_GlobalSRVHeapStart.IsValid());
	RenderEnv* pRenderEnv = pParams->m_pRenderEnv;
		
	m_GlobalSRVHeapStart = pRenderEnv->m_pShaderVisibleSRVHeap->Allocate();
	pRenderEnv->m_pDevice->CopyDescriptor(m_GlobalSRVHeapStart,
		m_pRayTracedResult->GetUAVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}

void RayTracingPass::InitShaderTables(InitParams* pParams)
{
	RenderEnv* pRenderEnv = pParams->m_pRenderEnv;

	assert(m_pRayGenShaderTable == nullptr);
	assert(m_pMissShaderTable == nullptr);
	assert(m_pHitGroupTable == nullptr);

	assert(m_pStateObject != nullptr);

	Buffer* pUploadRayGenShaderTable = nullptr;
	Buffer* pUploadMissShaderTable = nullptr;
	Buffer* pUploadHitGroupTable = nullptr;

	const SIZE_T shaderIdentifierSizeInBytes = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
	{		
		const UINT shaderRecordSizeInBytes = shaderIdentifierSizeInBytes;
		ShaderRecordData shaderRecordData(1, shaderRecordSizeInBytes);
		shaderRecordData.Append(ShaderRecord(m_pStateObject->GetShaderIdentifier(g_pRayGenShaderName), shaderIdentifierSizeInBytes));
		
		ShaderTableDesc shaderTableDesc(shaderRecordData.GetNumRecords(), shaderRecordSizeInBytes, true);
		m_pRayGenShaderTable = new Buffer(pRenderEnv, pRenderEnv->m_pDefaultHeapProps, &shaderTableDesc,
			D3D12_RESOURCE_STATE_COPY_DEST, L"RayTracingPass::m_pRayGenShaderTable");
		
		ShaderTableDesc uploadShaderTableDesc(shaderRecordData.GetNumRecords(), shaderRecordSizeInBytes);
		pUploadRayGenShaderTable = new Buffer(pRenderEnv, pRenderEnv->m_pUploadHeapProps, &uploadShaderTableDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ, L"RayTracingPass::pUploadRayGenShaderTable");
		pUploadRayGenShaderTable->Write(shaderRecordData.GetData(), shaderRecordData.GetSizeInBytes());
	}

	{
		const UINT shaderRecordSizeInBytes = shaderIdentifierSizeInBytes;
		ShaderRecordData shaderRecordData(1, shaderRecordSizeInBytes);
		shaderRecordData.Append(ShaderRecord(m_pStateObject->GetShaderIdentifier(g_pMissShaderName), shaderIdentifierSizeInBytes));

		ShaderTableDesc shaderTableDesc(shaderRecordData.GetNumRecords(), shaderRecordSizeInBytes, true);
		m_pMissShaderTable = new Buffer(pRenderEnv, pRenderEnv->m_pDefaultHeapProps, &shaderTableDesc,
			D3D12_RESOURCE_STATE_COPY_DEST, L"RayTracingPass::m_pMissShaderTable");

		ShaderTableDesc uploadShaderTableDesc(shaderRecordData.GetNumRecords(), shaderRecordSizeInBytes);
		pUploadMissShaderTable = new Buffer(pRenderEnv, pRenderEnv->m_pUploadHeapProps, &uploadShaderTableDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ, L"RayTracingPass::pUploadMissShaderTable");
		pUploadMissShaderTable->Write(shaderRecordData.GetData(), shaderRecordData.GetSizeInBytes());
	}

	{
		const UINT numGeometriesInBLAS = pParams->m_pMeshBatch->GetNumMeshes();

		const UINT shaderRecordSizeInBytes = shaderIdentifierSizeInBytes;
		ShaderRecordData shaderRecordData(numGeometriesInBLAS, shaderRecordSizeInBytes);

		for (UINT index = 0; index < numGeometriesInBLAS; ++index)
			shaderRecordData.Append(ShaderRecord(m_pStateObject->GetShaderIdentifier(g_pHitGroupName), shaderIdentifierSizeInBytes));
		
		ShaderTableDesc shaderTableDesc(shaderRecordData.GetNumRecords(), shaderRecordSizeInBytes, true);
		m_pHitGroupTable = new Buffer(pRenderEnv, pRenderEnv->m_pDefaultHeapProps, &shaderTableDesc,
			D3D12_RESOURCE_STATE_COPY_DEST, L"RayTracingPass::m_pHitGroupTable");

		ShaderTableDesc uploadShaderTableDesc(shaderRecordData.GetNumRecords(), shaderRecordSizeInBytes);
		pUploadHitGroupTable = new Buffer(pRenderEnv, pRenderEnv->m_pUploadHeapProps, &uploadShaderTableDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ, L"RayTracingPass::pUploadHitGroupTable");
		pUploadHitGroupTable->Write(shaderRecordData.GetData(), shaderRecordData.GetSizeInBytes());
	}
	
	const ResourceTransitionBarrier resourceBarriers[] =
	{
		ResourceTransitionBarrier(m_pRayGenShaderTable, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE),
		ResourceTransitionBarrier(m_pMissShaderTable, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE),
		ResourceTransitionBarrier(m_pHitGroupTable, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE)
	};

	CommandList* pCommandList = pRenderEnv->m_pCommandListPool->Create(L"RayTracingPass::pUploadShaderTableDataCommandList");
	pCommandList->Begin();
	pCommandList->CopyResource(m_pRayGenShaderTable, pUploadRayGenShaderTable);
	pCommandList->CopyResource(m_pMissShaderTable, pUploadMissShaderTable);
	pCommandList->CopyResource(m_pHitGroupTable, pUploadHitGroupTable);
	pCommandList->ResourceBarrier(ARRAYSIZE(resourceBarriers), resourceBarriers);
	pCommandList->End();

	++pRenderEnv->m_LastSubmissionFenceValue;
	pRenderEnv->m_pCommandQueue->ExecuteCommandLists(1, &pCommandList, pRenderEnv->m_pFence, pRenderEnv->m_LastSubmissionFenceValue);
	pRenderEnv->m_pFence->WaitForSignalOnCPU(pRenderEnv->m_LastSubmissionFenceValue);

	SafeDelete(pUploadRayGenShaderTable);
	SafeDelete(pUploadMissShaderTable);
	SafeDelete(pUploadHitGroupTable);
}
