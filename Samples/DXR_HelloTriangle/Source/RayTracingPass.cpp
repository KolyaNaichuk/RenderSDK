#include "RayTracingPass.h"
#include "D3DWrapper/RootSignature.h"
#include "D3DWrapper/PipelineState.h"
#include "D3DWrapper/StateObject.h"
#include "D3DWrapper/GraphicsDevice.h"
#include "D3DWrapper/RenderEnv.h"
#include "Profiler/GPUProfiler.h"
#include "Scene/Mesh.h"
#include "Math/Vector3.h"

namespace
{
	LPCWSTR g_pRayGenShaderName = L"RayGeneration";
	LPCWSTR g_pMissShaderName = L"RayMiss";
	LPCWSTR g_pClosesHitShaderName = L"RayClosestHit";
	LPCWSTR g_pHitGroupName = L"HitGroup";
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
	SafeDelete(m_pRayGenLocalRootSignature);
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
	
	const VertexData* pVertexData = pParams->m_pVertexData;
	assert(pVertexData != nullptr);
	
	assert(m_pVertexBuffer == nullptr);
	StructuredBufferDesc vertexBufferDesc(pVertexData->GetNumVertices(), sizeof(Vector3f), true, false, true);
	m_pVertexBuffer = new Buffer(pRenderEnv, pRenderEnv->m_pDefaultHeapProps, &vertexBufferDesc,
		D3D12_RESOURCE_STATE_COPY_DEST, L"RayTracingPass::m_pVertexBuffer");

	StructuredBufferDesc uploadVertexBufferDesc(pVertexData->GetNumVertices(), sizeof(Vector3f), false, false);
	Buffer uploadVertexBuffer(pRenderEnv, pRenderEnv->m_pUploadHeapProps, &uploadVertexBufferDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ, L"RayTracingPass::uploadVertexBuffer");
	uploadVertexBuffer.Write(pVertexData->GetPositions(), pVertexData->GetNumVertices() * sizeof(Vector3f));

	const IndexData* pIndexData = pParams->m_pIndexData;
	assert(pIndexData != nullptr);

	assert(m_pIndexBuffer == nullptr);
	FormattedBufferDesc indexBufferDesc(pIndexData->GetNumIndices(), pIndexData->GetFormat(), true, false, true);
	m_pIndexBuffer = new Buffer(pRenderEnv, pRenderEnv->m_pDefaultHeapProps, &indexBufferDesc,
		D3D12_RESOURCE_STATE_COPY_DEST, L"RayTracingPass::m_pIndexBuffer");

	FormattedBufferDesc uploadIndexBufferDesc(pIndexData->GetNumIndices(), pIndexData->GetFormat(), false, false);
	Buffer uploadIndexBuffer(pRenderEnv, pRenderEnv->m_pUploadHeapProps, &uploadIndexBufferDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ, L"RayTracingPass::uploadIndexBuffer");

	if (pIndexData->GetFormat() == DXGI_FORMAT_R16_UINT)
		uploadIndexBuffer.Write(pIndexData->Get16BitIndices(), pIndexData->GetNumIndices() * sizeof(u16));
	else
		uploadIndexBuffer.Write(pIndexData->Get32BitIndices(), pIndexData->GetNumIndices() * sizeof(u32));

	const ResourceTransitionBarrier resourceBarriers[] =
	{
		ResourceTransitionBarrier(m_pVertexBuffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE),
		ResourceTransitionBarrier(m_pIndexBuffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE)
	};

	CommandList* pUploadCommandList = pRenderEnv->m_pCommandListPool->Create(L"RayTracingPass::uploadVertexDataCommandList");
	pUploadCommandList->Begin();
	pUploadCommandList->CopyResource(m_pVertexBuffer, &uploadVertexBuffer);
	pUploadCommandList->CopyResource(m_pIndexBuffer, &uploadIndexBuffer);
	pUploadCommandList->ResourceBarrier(ARRAYSIZE(resourceBarriers), resourceBarriers);
	pUploadCommandList->End();

	++pRenderEnv->m_LastSubmissionFenceValue;
	pRenderEnv->m_pCommandQueue->ExecuteCommandLists(1, &pUploadCommandList, pRenderEnv->m_pFence, pRenderEnv->m_LastSubmissionFenceValue);
	pRenderEnv->m_pFence->WaitForSignalOnCPU(pRenderEnv->m_LastSubmissionFenceValue);
}

void RayTracingPass::InitAccelerationStructures(InitParams* pParams)
{
	RenderEnv* pRenderEnv = pParams->m_pRenderEnv;

	// Bottom level acceleration structure

	assert(m_pVertexBuffer != nullptr);
	assert(m_pIndexBuffer != nullptr);

	RayTracingTrianglesGeometryDesc geometryDesc(DXGI_FORMAT_R32G32B32_FLOAT, m_pVertexBuffer, m_pIndexBuffer, D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE);

	BuildRayTracingAccelerationStructureInputs BLASBuildInputs;
	BLASBuildInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
	BLASBuildInputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
	BLASBuildInputs.NumDescs = 1;
	BLASBuildInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	BLASBuildInputs.pGeometryDescs = &geometryDesc;

	RayTracingAccelerationStructurePrebuildInfo BLASPrebuildInfo;
	pRenderEnv->m_pDevice->GetRaytracingAccelerationStructurePrebuildInfo(&BLASBuildInputs, &BLASPrebuildInfo);

	assert(BLASPrebuildInfo.ResultDataMaxSizeInBytes > 0);
	assert(BLASPrebuildInfo.ScratchDataSizeInBytes > 0);
	assert(BLASPrebuildInfo.UpdateScratchDataSizeInBytes == 0);

	assert(m_pBLASBuffer == nullptr);
	StructuredBufferDesc BLASBufferDesc(1, BLASPrebuildInfo.ResultDataMaxSizeInBytes, true, true);
	m_pBLASBuffer = new Buffer(pRenderEnv, pRenderEnv->m_pDefaultHeapProps, &BLASBufferDesc,
		D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, L"RayTracingPass::m_pBLASBuffer");

	StructuredBufferDesc BLASScratchBufferDesc(1, BLASPrebuildInfo.ScratchDataSizeInBytes, false, true);
	Buffer BLASScratchBuffer(pRenderEnv, pRenderEnv->m_pDefaultHeapProps, &BLASScratchBufferDesc,
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS, L"RayTracingPass::BLASScratchBuffer");

	// Top level instance data

	const FLOAT instanceMatrix[3][4] =
	{
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f
	};

	RayTracingInstanceDesc instanceDesc;
	CopyMemory(instanceDesc.Transform, instanceMatrix, sizeof(instanceMatrix));
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

	const ResourceTransitionBarrier uploadResourceBarriers[] = {
		ResourceTransitionBarrier(m_pInstanceBuffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE)
	};
	CommandList* pUploadCommandList = pRenderEnv->m_pCommandListPool->Create(L"RayTracingPass::uploadInstanceDataCommandList");
	pUploadCommandList->Begin();
	pUploadCommandList->CopyResource(m_pInstanceBuffer, &uploadInstanceBuffer);
	pUploadCommandList->ResourceBarrier(ARRAYSIZE(uploadResourceBarriers), uploadResourceBarriers);
	pUploadCommandList->End();

	++pRenderEnv->m_LastSubmissionFenceValue;
	pRenderEnv->m_pCommandQueue->ExecuteCommandLists(1, &pUploadCommandList, pRenderEnv->m_pFence, pRenderEnv->m_LastSubmissionFenceValue);
	pRenderEnv->m_pFence->WaitForSignalOnCPU(pRenderEnv->m_LastSubmissionFenceValue);

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
	StructuredBufferDesc TLASBufferDesc(1, TLASPrebuildInfo.ResultDataMaxSizeInBytes, true, true);
	m_pTLASBuffer = new Buffer(pRenderEnv, pRenderEnv->m_pDefaultHeapProps, &TLASBufferDesc,
		D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, L"RayTracingPass::m_pTLASBuffer");

	StructuredBufferDesc TLASScratchBufferDesc(1, TLASPrebuildInfo.ScratchDataSizeInBytes, false, true);
	Buffer TLASScratchBuffer(pRenderEnv, pRenderEnv->m_pDefaultHeapProps, &TLASScratchBufferDesc,
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS, L"RayTracingPass::TLASScratchBuffer");

	// Common

	BuildRayTracingAccelerationStructureDesc buildBLASDesc(&BLASBuildInputs, m_pBLASBuffer, &BLASScratchBuffer);
	BuildRayTracingAccelerationStructureDesc buildTLASDesc(&TLASBuildInputs, m_pTLASBuffer, &TLASScratchBuffer);

	CommandList* pBuildCommandList = pRenderEnv->m_pCommandListPool->Create(L"RayTracingPass::buildRayTracingASCommandList");
	pBuildCommandList->Begin();
	pBuildCommandList->BuildRaytracingAccelerationStructure(&buildBLASDesc, 0, nullptr);
	pBuildCommandList->BuildRaytracingAccelerationStructure(&buildTLASDesc, 0, nullptr);
	pBuildCommandList->End();

	++pRenderEnv->m_LastSubmissionFenceValue;
	pRenderEnv->m_pCommandQueue->ExecuteCommandLists(1, &pBuildCommandList, pRenderEnv->m_pFence, pRenderEnv->m_LastSubmissionFenceValue);
	pRenderEnv->m_pFence->WaitForSignalOnCPU(pRenderEnv->m_LastSubmissionFenceValue);
}

void RayTracingPass::InitRootSignatures(InitParams* pParams)
{
	RenderEnv* pRenderEnv = pParams->m_pRenderEnv;

	enum RayGenRootParams
	{
		kRayGenRootCBVParam = 0,
		kRayGenRootSRVTableParam,
		kRayGenNumRootParams
	};

	D3D12_ROOT_PARAMETER rayGenRootParams[kRayGenNumRootParams];
	rayGenRootParams[kRayGenRootCBVParam] = RootCBVParameter(0, D3D12_SHADER_VISIBILITY_ALL);

	D3D12_DESCRIPTOR_RANGE rayGenDescriptorRanges[] = {CBVDescriptorRange(1, 0), SRVDescriptorRange(1, 0), UAVDescriptorRange(1, 0)};
	rayGenRootParams[kRayGenRootSRVTableParam] = RootDescriptorTableParameter(ARRAYSIZE(rayGenDescriptorRanges), rayGenDescriptorRanges, D3D12_SHADER_VISIBILITY_ALL);

	assert(m_pRayGenLocalRootSignature == nullptr);
	RootSignatureDesc rayGenRootSignatureDesc(kRayGenNumRootParams, rayGenRootParams, D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE);
	m_pRayGenLocalRootSignature = new RootSignature(pRenderEnv->m_pDevice, &rayGenRootSignatureDesc, L"RayTracingPass::m_pRayGenLocalRootSignature");

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
		kRayGenLocalRSIndex,
		kRayGenLocalRSAssocIndex,
		kEmptyLocalRSIndex,
		kEmptyLocalRSAssocIndex,
		kNumStateObjects
	};
	D3D12_STATE_SUBOBJECT stateSubobjects[kNumStateObjects];

	Shader shaderLibrary(L"Shaders//RayTracing.hlsl", nullptr, L"lib_6_3");
	ExportDesc exportDescs[] = {ExportDesc(g_pRayGenShaderName), ExportDesc(g_pMissShaderName), ExportDesc(g_pClosesHitShaderName)};
	
	DXILLibraryDesc libraryDesc(shaderLibrary.GetBytecode(), ARRAYSIZE(exportDescs), exportDescs);
	stateSubobjects[kDXILLibraryIndex] = StateSubobject(D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY, &libraryDesc);

	HitGroupDesc hitGroupDesc(g_pHitGroupName, D3D12_HIT_GROUP_TYPE_TRIANGLES, nullptr, g_pClosesHitShaderName, nullptr);
	stateSubobjects[kHitGroupIndex] = StateSubobject(D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP, &hitGroupDesc);

	RayTracingPipelineConfig pipelineConfig(1);
	stateSubobjects[kPipelineConfigIndex] = StateSubobject(D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG, &pipelineConfig);

	RayTracingShaderConfig shaderConfig(3 * sizeof(f32)/*color*/, 2 * sizeof(f32)/*intersectionAttribs*/);
	stateSubobjects[kShaderConfigIndex] = StateSubobject(D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG, &shaderConfig);

	assert(m_pRayGenLocalRootSignature != nullptr);
	stateSubobjects[kRayGenLocalRSIndex] = StateSubobject(D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE, m_pRayGenLocalRootSignature);

	LPCWSTR rayGenLocalRSExports[] = {g_pRayGenShaderName};
	SubobjectToExportsAssociation rayGenLocalRSExportsAssoc(&stateSubobjects[kRayGenLocalRSIndex], ARRAYSIZE(rayGenLocalRSExports), rayGenLocalRSExports);
	stateSubobjects[kRayGenLocalRSAssocIndex] = StateSubobject(D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION, &rayGenLocalRSExportsAssoc);

	assert(m_pEmptyLocalRootSignature != nullptr);
	stateSubobjects[kEmptyLocalRSIndex] = StateSubobject(D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE, m_pEmptyLocalRootSignature);

	LPCWSTR emptyLocalRSExports[] = {g_pMissShaderName, g_pClosesHitShaderName};
	SubobjectToExportsAssociation emptyLocalRSExportsAssoc(&stateSubobjects[kEmptyLocalRSIndex], ARRAYSIZE(emptyLocalRSExports), emptyLocalRSExports);
	stateSubobjects[kEmptyLocalRSAssocIndex] = StateSubobject(D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION, &emptyLocalRSExportsAssoc);

	assert(m_pStateObject == nullptr);
	StateObjectDesc stateObjectDesc(D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE, kNumStateObjects, stateSubobjects);
	m_pStateObject = new StateObject(pRenderEnv->m_pDevice, &stateObjectDesc, L"RayTracingPass::m_pStateObject");
}

void RayTracingPass::InitDescriptorHeap(InitParams* pParams)
{
	assert(!m_SRVHeapStart.IsValid());
	RenderEnv* pRenderEnv = pParams->m_pRenderEnv;

	m_SRVHeapStart = pRenderEnv->m_pShaderVisibleSRVHeap->Allocate();
	pRenderEnv->m_pDevice->CopyDescriptor(m_SRVHeapStart,
		pParams->m_pAppDataBuffer->GetCBVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	pRenderEnv->m_pDevice->CopyDescriptor(pRenderEnv->m_pShaderVisibleSRVHeap->Allocate(),
		m_pTLASBuffer->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	pRenderEnv->m_pDevice->CopyDescriptor(pRenderEnv->m_pShaderVisibleSRVHeap->Allocate(),
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
		struct RootArguments
		{
			D3D12_GPU_DESCRIPTOR_HANDLE m_SRVTableBaseHandle;
		};

		assert(m_SRVHeapStart.IsValid());
		RootArguments rootArguments;
		rootArguments.m_SRVTableBaseHandle = m_SRVHeapStart;
		
		const UINT shaderRecordSizeInBytes = shaderIdentifierSizeInBytes + sizeof(rootArguments);
		ShaderRecordData shaderRecordData(1, shaderRecordSizeInBytes);
		shaderRecordData.Append(ShaderRecord(m_pStateObject->GetShaderIdentifier(g_pRayGenShaderName), shaderIdentifierSizeInBytes, &rootArguments, sizeof(rootArguments)));
		
		ShaderTableDesc shaderTableDesc(shaderRecordData.GetNumRecords(), shaderRecordSizeInBytes);
		m_pRayGenShaderTable = new Buffer(pRenderEnv, pRenderEnv->m_pDefaultHeapProps, &shaderTableDesc,
			D3D12_RESOURCE_STATE_COPY_DEST, L"RayTracingPass::m_pRayGenShaderTable");
		
		pUploadRayGenShaderTable = new Buffer(pRenderEnv, pRenderEnv->m_pUploadHeapProps, &shaderTableDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ, L"RayTracingPass::pUploadRayGenShaderTable");
		pUploadRayGenShaderTable->Write(shaderRecordData.GetData(), shaderRecordData.GetSizeInBytes());
	}

	{
		const UINT shaderRecordSizeInBytes = shaderIdentifierSizeInBytes;
		ShaderRecordData shaderRecordData(1, shaderRecordSizeInBytes);
		shaderRecordData.Append(ShaderRecord(m_pStateObject->GetShaderIdentifier(g_pMissShaderName), shaderIdentifierSizeInBytes));

		ShaderTableDesc shaderTableDesc(shaderRecordData.GetNumRecords(), shaderRecordSizeInBytes);
		m_pMissShaderTable = new Buffer(pRenderEnv, pRenderEnv->m_pDefaultHeapProps, &shaderTableDesc,
			D3D12_RESOURCE_STATE_COPY_DEST, L"RayTracingPass::m_pMissShaderTable");

		pUploadMissShaderTable = new Buffer(pRenderEnv, pRenderEnv->m_pUploadHeapProps, &shaderTableDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ, L"RayTracingPass::pUploadMissShaderTable");
		pUploadMissShaderTable->Write(shaderRecordData.GetData(), shaderRecordData.GetSizeInBytes());
	}

	{
		const UINT shaderRecordSizeInBytes = shaderIdentifierSizeInBytes;
		ShaderRecordData shaderRecordData(1, shaderRecordSizeInBytes);
		shaderRecordData.Append(ShaderRecord(m_pStateObject->GetShaderIdentifier(g_pHitGroupName), shaderIdentifierSizeInBytes));
		
		ShaderTableDesc shaderTableDesc(shaderRecordData.GetNumRecords(), shaderRecordSizeInBytes);
		m_pHitGroupTable = new Buffer(pRenderEnv, pRenderEnv->m_pDefaultHeapProps, &shaderTableDesc,
			D3D12_RESOURCE_STATE_COPY_DEST, L"RayTracingPass::m_pHitGroupTable");

		pUploadHitGroupTable = new Buffer(pRenderEnv, pRenderEnv->m_pUploadHeapProps, &shaderTableDesc,
			D3D12_RESOURCE_STATE_COPY_DEST, L"RayTracingPass::pUploadHitGroupTable");
		pUploadHitGroupTable->Write(shaderRecordData.GetData(), shaderRecordData.GetSizeInBytes());
	}
	
	const ResourceTransitionBarrier resourceBarriers[] =
	{
		ResourceTransitionBarrier(m_pRayGenShaderTable, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE),
		ResourceTransitionBarrier(m_pMissShaderTable, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE),
		ResourceTransitionBarrier(m_pHitGroupTable, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE)
	};

	CommandList* pUploadCommandList = pRenderEnv->m_pCommandListPool->Create(L"RayTracingPass::uploadShaderTableDataCommandList");
	pUploadCommandList->Begin();
	pUploadCommandList->CopyResource(m_pRayGenShaderTable, pUploadRayGenShaderTable);
	pUploadCommandList->CopyResource(m_pMissShaderTable, pUploadMissShaderTable);
	pUploadCommandList->CopyResource(m_pHitGroupTable, pUploadHitGroupTable);
	pUploadCommandList->ResourceBarrier(ARRAYSIZE(resourceBarriers), resourceBarriers);
	pUploadCommandList->End();

	++pRenderEnv->m_LastSubmissionFenceValue;
	pRenderEnv->m_pCommandQueue->ExecuteCommandLists(1, &pUploadCommandList, pRenderEnv->m_pFence, pRenderEnv->m_LastSubmissionFenceValue);
	pRenderEnv->m_pFence->WaitForSignalOnCPU(pRenderEnv->m_LastSubmissionFenceValue);

	SafeDelete(pUploadRayGenShaderTable);
	SafeDelete(pUploadMissShaderTable);
	SafeDelete(pUploadHitGroupTable);
}
