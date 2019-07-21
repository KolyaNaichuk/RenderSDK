#include "DXApplication.h"
#include "D3DWrapper/GraphicsFactory.h"
#include "D3DWrapper/GraphicsDevice.h"
#include "D3DWrapper/SwapChain.h"
#include "D3DWrapper/CommandQueue.h"
#include "D3DWrapper/CommandList.h"
#include "D3DWrapper/DescriptorHeap.h"
#include "D3DWrapper/RootSignature.h"
#include "D3DWrapper/PipelineState.h"
#include "D3DWrapper/StateObject.h"
#include "D3DWrapper/GraphicsResource.h"
#include "D3DWrapper/RenderEnv.h"
#include "D3DWrapper/Fence.h"
#include "D3DWrapper/GraphicsUtils.h"
#include "Math/Vector3.h"

DXApplication::DXApplication(HINSTANCE hApp)
	: Application(hApp, L"Hello Triangle", 0, 0, 1024, 512)
	, m_pDevice(nullptr)
	, m_pSwapChain(nullptr)
	, m_pCommandQueue(nullptr)
	, m_pCommandListPool(nullptr)
	, m_pShaderInvisibleRTVHeap(nullptr)
	, m_pShaderInvisibleSRVHeap(nullptr)
	, m_pVertexBuffer(nullptr)
	, m_pIndexBuffer(nullptr)
	, m_pBLASBuffer(nullptr)
	, m_pTLASBuffer(nullptr)
	, m_pInstanceBuffer(nullptr)
	, m_pRayGenLocalRootSignature(nullptr)
	, m_pEmptyLocalRootSignature(nullptr)
	, m_pStateObject(nullptr)
	, m_pDefaultHeapProps(new HeapProperties(D3D12_HEAP_TYPE_DEFAULT))
	, m_pUploadHeapProps(new HeapProperties(D3D12_HEAP_TYPE_UPLOAD))
	, m_pRenderEnv(new RenderEnv())
	, m_pFence(nullptr)
	, m_pViewport(nullptr)
	, m_pScissorRect(nullptr)
	, m_BackBufferIndex(0)
{
	for (u8 index = 0; index < kNumBackBuffers; ++index)
		m_FrameCompletionFenceValues[index] = m_pRenderEnv->m_LastSubmissionFenceValue;
}

DXApplication::~DXApplication()
{
	SafeDelete(m_pCommandListPool);
	SafeDelete(m_pRenderEnv);
	SafeDelete(m_pDefaultHeapProps);
	SafeDelete(m_pUploadHeapProps);
	SafeDelete(m_pViewport);
	SafeDelete(m_pScissorRect);
	SafeDelete(m_pTLASBuffer);
	SafeDelete(m_pBLASBuffer);
	SafeDelete(m_pInstanceBuffer);
	SafeDelete(m_pVertexBuffer);
	SafeDelete(m_pIndexBuffer);
	SafeDelete(m_pRayGenLocalRootSignature);
	SafeDelete(m_pEmptyLocalRootSignature);
	SafeDelete(m_pStateObject);
	SafeDelete(m_pFence);
	SafeDelete(m_pShaderInvisibleSRVHeap);
	SafeDelete(m_pShaderInvisibleRTVHeap);
	SafeDelete(m_pDevice);
	SafeDelete(m_pSwapChain);
	SafeDelete(m_pCommandQueue);
}

void DXApplication::OnInit()
{
	InitRenderEnvironment();
	BuildGeometryBuffers();
	BuildAccelerationStructures();
	CreateRootSignatures();
	CreateStateObject();
}

void DXApplication::OnUpdate()
{
}

void DXApplication::OnRender()
{
	/*
	CommandList* pCommandList = m_pCommandListPool->Create(L"renderRectCommandList");
	
	pCommandList->Begin(m_pPipelineState);
	pCommandList->SetGraphicsRootSignature(m_pRootSignature);
		
	ColorTexture* pRenderTarget = m_pSwapChain->GetBackBuffer(m_BackBufferIndex);
	ResourceTransitionBarrier renderTargetBarrier(pRenderTarget, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
	pCommandList->ResourceBarrier(1, &renderTargetBarrier);
		
	const FLOAT clearColor[4] = {0.1f, 0.7f, 0.4f, 1.0f};
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = pRenderTarget->GetRTVHandle();
	pCommandList->ClearRenderTargetView(rtvHandle, clearColor);

	pCommandList->OMSetRenderTargets(1, &rtvHandle);
	pCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	pCommandList->IASetVertexBuffers(0, 1, m_pVertexBuffer->GetVBView());
	pCommandList->IASetIndexBuffer(m_pIndexBuffer->GetIBView());
	pCommandList->RSSetViewports(1, m_pViewport);
	pCommandList->RSSetScissorRects(1, m_pScissorRect);
	pCommandList->DrawIndexedInstanced(6, 1, 0, 0, 0);

	ResourceTransitionBarrier presentStateBarrier(pRenderTarget, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
	pCommandList->ResourceBarrier(1, &presentStateBarrier);
	pCommandList->End();

	++m_pRenderEnv->m_LastSubmissionFenceValue;
	m_pCommandQueue->ExecuteCommandLists(1, &pCommandList, m_pFence, m_pRenderEnv->m_LastSubmissionFenceValue);

	++m_pRenderEnv->m_LastSubmissionFenceValue;
	m_pSwapChain->Present(1, 0);
	m_pCommandQueue->Signal(m_pFence, m_pRenderEnv->m_LastSubmissionFenceValue);

	m_FrameCompletionFenceValues[m_BackBufferIndex] = m_pRenderEnv->m_LastSubmissionFenceValue;
	m_BackBufferIndex = m_pSwapChain->GetCurrentBackBufferIndex();
	m_pFence->WaitForSignalOnCPU(m_FrameCompletionFenceValues[m_BackBufferIndex]);
	*/
}

void DXApplication::OnDestroy()
{
	m_pCommandQueue->Signal(m_pFence, m_pRenderEnv->m_LastSubmissionFenceValue);
	m_pFence->WaitForSignalOnCPU(m_pRenderEnv->m_LastSubmissionFenceValue);
}

void DXApplication::InitRenderEnvironment()
{
	GraphicsFactory factory;
	m_pDevice = new GraphicsDevice(&factory, D3D_FEATURE_LEVEL_12_1);

	D3D12_FEATURE_DATA_D3D12_OPTIONS5 supportedOptions;
	m_pDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &supportedOptions, sizeof(supportedOptions));
	assert(supportedOptions.RaytracingTier == D3D12_RAYTRACING_TIER_1_0);

	DescriptorHeapDesc rtvHeapDesc(D3D12_DESCRIPTOR_HEAP_TYPE_RTV, kNumBackBuffers, false);
	m_pShaderInvisibleRTVHeap = new DescriptorHeap(m_pDevice, &rtvHeapDesc, L"m_pShaderInvisibleRTVHeap");

	DescriptorHeapDesc srvHeapDesc(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 32, false);
	m_pShaderInvisibleSRVHeap = new DescriptorHeap(m_pDevice, &srvHeapDesc, L"m_pShaderInvisibleSRVHeap");

	CommandQueueDesc commandQueueDesc(D3D12_COMMAND_LIST_TYPE_DIRECT);
	m_pCommandQueue = new CommandQueue(m_pDevice, &commandQueueDesc, L"m_pCommandQueue");

	m_pCommandListPool = new CommandListPool(m_pDevice, D3D12_COMMAND_LIST_TYPE_DIRECT);
	m_pFence = new Fence(m_pDevice, m_pRenderEnv->m_LastSubmissionFenceValue, L"m_pFence");

	m_pRenderEnv->m_pDevice = m_pDevice;
	m_pRenderEnv->m_pCommandListPool = m_pCommandListPool;
	m_pRenderEnv->m_pCommandQueue = m_pCommandQueue;
	m_pRenderEnv->m_pFence = m_pFence;
	m_pRenderEnv->m_pDefaultHeapProps = m_pDefaultHeapProps;
	m_pRenderEnv->m_pUploadHeapProps = m_pUploadHeapProps;
	m_pRenderEnv->m_pShaderInvisibleRTVHeap = m_pShaderInvisibleRTVHeap;
	m_pRenderEnv->m_pShaderInvisibleSRVHeap = m_pShaderInvisibleSRVHeap;

	const RECT bufferRect = m_pWindow->GetClientRect();
	const UINT bufferWidth = bufferRect.right - bufferRect.left;
	const UINT bufferHeight = bufferRect.bottom - bufferRect.top;

	m_pViewport = new Viewport(0.0f, 0.0f, FLOAT(bufferWidth), FLOAT(bufferHeight));
	m_pScissorRect = new Rect(0, 0, bufferWidth, bufferHeight);

	SwapChainDesc swapChainDesc(kNumBackBuffers, m_pWindow->GetHWND(), bufferWidth, bufferHeight);
	m_pSwapChain = new SwapChain(&factory, m_pRenderEnv, &swapChainDesc, m_pCommandQueue);
	m_BackBufferIndex = m_pSwapChain->GetCurrentBackBufferIndex();
}

void DXApplication::BuildGeometryBuffers()
{
	const Vector3f vertices[] =
	{
		Vector3f(0.0f, 10.0f, 2.0f),
		Vector3f(10.0f, -10.0f, 2.0f),
		Vector3f(-10.0f, -10.0f, 2.0f)
	};
	const WORD indices[] = {0, 1, 2};

	assert(m_pVertexBuffer == nullptr);
	StructuredBufferDesc vertexBufferDesc(ARRAYSIZE(vertices), sizeof(vertices[0]), true, false, true);
	m_pVertexBuffer = new Buffer(m_pRenderEnv, m_pRenderEnv->m_pDefaultHeapProps, &vertexBufferDesc, D3D12_RESOURCE_STATE_COPY_DEST, L"m_pVertexBuffer");

	StructuredBufferDesc uploadVertexBufferDesc(ARRAYSIZE(vertices), sizeof(vertices[0]), false, false);
	Buffer uploadVertexBuffer(m_pRenderEnv, m_pRenderEnv->m_pUploadHeapProps, &uploadVertexBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, L"uploadVertexBuffer");
	uploadVertexBuffer.Write(vertices, sizeof(vertices));

	assert(m_pIndexBuffer == nullptr);
	FormattedBufferDesc indexBufferDesc(ARRAYSIZE(indices), GetIndexBufferFormat(sizeof(indices[0])), true, false, true);
	m_pIndexBuffer = new Buffer(m_pRenderEnv, m_pRenderEnv->m_pDefaultHeapProps, &indexBufferDesc, D3D12_RESOURCE_STATE_COPY_DEST, L"m_pIndexBuffer");

	FormattedBufferDesc uploadIndexBufferDesc(ARRAYSIZE(indices), GetIndexBufferFormat(sizeof(indices[0])), false, false);
	Buffer uploadIndexBuffer(m_pRenderEnv, m_pRenderEnv->m_pUploadHeapProps, &uploadIndexBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, L"uploadIndexBuffer");
	uploadIndexBuffer.Write(indices, sizeof(indices));

	const ResourceTransitionBarrier resourceBarriers[] = {
		ResourceTransitionBarrier(m_pVertexBuffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE),
		ResourceTransitionBarrier(m_pIndexBuffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE)
	};
	
	CommandList* pUploadCommandList = m_pCommandListPool->Create(L"uploadVertexDataCommandList");
	pUploadCommandList->Begin();
	pUploadCommandList->CopyResource(m_pVertexBuffer, &uploadVertexBuffer);
	pUploadCommandList->CopyResource(m_pIndexBuffer, &uploadIndexBuffer);
	pUploadCommandList->ResourceBarrier(ARRAYSIZE(resourceBarriers), resourceBarriers);
	pUploadCommandList->End();

	++m_pRenderEnv->m_LastSubmissionFenceValue;
	m_pRenderEnv->m_pCommandQueue->ExecuteCommandLists(1, &pUploadCommandList, m_pRenderEnv->m_pFence, m_pRenderEnv->m_LastSubmissionFenceValue);
	m_pRenderEnv->m_pFence->WaitForSignalOnCPU(m_pRenderEnv->m_LastSubmissionFenceValue);
}

void DXApplication::BuildAccelerationStructures()
{
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
	m_pDevice->GetRaytracingAccelerationStructurePrebuildInfo(&BLASBuildInputs, &BLASPrebuildInfo);

	assert(BLASPrebuildInfo.ResultDataMaxSizeInBytes > 0);
	assert(BLASPrebuildInfo.ScratchDataSizeInBytes > 0);
	assert(BLASPrebuildInfo.UpdateScratchDataSizeInBytes == 0);

	assert(m_pBLASBuffer == nullptr);
	StructuredBufferDesc BLASBufferDesc(1, BLASPrebuildInfo.ResultDataMaxSizeInBytes, true, true);
	m_pBLASBuffer = new Buffer(m_pRenderEnv, m_pRenderEnv->m_pDefaultHeapProps, &BLASBufferDesc, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, L"m_pBLASBuffer");

	StructuredBufferDesc BLASScratchBufferDesc(1, BLASPrebuildInfo.ScratchDataSizeInBytes, false, true);
	Buffer BLASScratchBuffer(m_pRenderEnv, m_pRenderEnv->m_pDefaultHeapProps, &BLASScratchBufferDesc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, L"BLASScratchBuffer");
				
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
	m_pInstanceBuffer = new Buffer(m_pRenderEnv, m_pRenderEnv->m_pDefaultHeapProps, &instanceBufferDesc, D3D12_RESOURCE_STATE_COPY_DEST, L"m_pInstanceBuffer");
	
	StructuredBufferDesc uploadInstanceBufferDesc(1, sizeof(instanceDesc), false, false);
	Buffer uploadInstanceBuffer(m_pRenderEnv, m_pRenderEnv->m_pUploadHeapProps, &uploadInstanceBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, L"uploadInstanceBuffer");
	uploadInstanceBuffer.Write(&instanceDesc, sizeof(instanceDesc));

	const ResourceTransitionBarrier uploadResourceBarriers[] = {
		ResourceTransitionBarrier(m_pInstanceBuffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE)
	};
	CommandList* pUploadCommandList = m_pCommandListPool->Create(L"uploadInstanceDataCommandList");
	pUploadCommandList->Begin();
	pUploadCommandList->CopyResource(m_pInstanceBuffer, &uploadInstanceBuffer);
	pUploadCommandList->ResourceBarrier(ARRAYSIZE(uploadResourceBarriers), uploadResourceBarriers);
	pUploadCommandList->End();

	++m_pRenderEnv->m_LastSubmissionFenceValue;
	m_pRenderEnv->m_pCommandQueue->ExecuteCommandLists(1, &pUploadCommandList, m_pRenderEnv->m_pFence, m_pRenderEnv->m_LastSubmissionFenceValue);
	m_pRenderEnv->m_pFence->WaitForSignalOnCPU(m_pRenderEnv->m_LastSubmissionFenceValue);

	// Top level acceleration structure

	BuildRayTracingAccelerationStructureInputs TLASBuildInputs;
	TLASBuildInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
	TLASBuildInputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
	TLASBuildInputs.NumDescs = 1;
	TLASBuildInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	TLASBuildInputs.InstanceDescs = m_pInstanceBuffer->GetGPUVirtualAddress();

	RayTracingAccelerationStructurePrebuildInfo TLASPrebuildInfo;
	m_pDevice->GetRaytracingAccelerationStructurePrebuildInfo(&TLASBuildInputs, &TLASPrebuildInfo);

	assert(TLASPrebuildInfo.ResultDataMaxSizeInBytes > 0);
	assert(TLASPrebuildInfo.ScratchDataSizeInBytes > 0);
	assert(TLASPrebuildInfo.UpdateScratchDataSizeInBytes == 0);

	assert(m_pTLASBuffer == nullptr);
	StructuredBufferDesc TLASBufferDesc(1, TLASPrebuildInfo.ResultDataMaxSizeInBytes, true, true);
	m_pTLASBuffer = new Buffer(m_pRenderEnv, m_pRenderEnv->m_pDefaultHeapProps, &TLASBufferDesc, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, L"m_pTLASBuffer");

	StructuredBufferDesc TLASScratchBufferDesc(1, TLASPrebuildInfo.ScratchDataSizeInBytes, false, true);
	Buffer TLASScratchBuffer(m_pRenderEnv, m_pRenderEnv->m_pDefaultHeapProps, &TLASScratchBufferDesc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, L"TLASScratchBuffer");

	// Common

	BuildRayTracingAccelerationStructureDesc buildBLASDesc(&BLASBuildInputs, m_pBLASBuffer, &BLASScratchBuffer);
	BuildRayTracingAccelerationStructureDesc buildTLASDesc(&TLASBuildInputs, m_pTLASBuffer, &TLASScratchBuffer);

	CommandList* pBuildCommandList = m_pCommandListPool->Create(L"buildRayTracingASCommandList");
	pBuildCommandList->Begin();
	pBuildCommandList->BuildRaytracingAccelerationStructure(&buildBLASDesc, 0, nullptr);
	pBuildCommandList->BuildRaytracingAccelerationStructure(&buildTLASDesc, 0, nullptr);
	pBuildCommandList->End();

	++m_pRenderEnv->m_LastSubmissionFenceValue;
	m_pRenderEnv->m_pCommandQueue->ExecuteCommandLists(1, &pBuildCommandList, m_pRenderEnv->m_pFence, m_pRenderEnv->m_LastSubmissionFenceValue);
	m_pRenderEnv->m_pFence->WaitForSignalOnCPU(m_pRenderEnv->m_LastSubmissionFenceValue);
}

void DXApplication::CreateRootSignatures()
{
	enum RayGenRootParams
	{
		kRayGenRootCBVParam = 0,
		kRayGenRootSRVTableParam,
		kRayGenNumRootParams
	};

	D3D12_ROOT_PARAMETER rayGenRootParams[kRayGenNumRootParams];
	rayGenRootParams[kRayGenRootCBVParam] = RootCBVParameter(0, D3D12_SHADER_VISIBILITY_ALL);

	D3D12_DESCRIPTOR_RANGE rayGenDescriptorRanges[] = {SRVDescriptorRange(1, 0), UAVDescriptorRange(1, 0)};
	rayGenRootParams[kRayGenRootSRVTableParam] = RootDescriptorTableParameter(ARRAYSIZE(rayGenDescriptorRanges), rayGenDescriptorRanges, D3D12_SHADER_VISIBILITY_ALL);
	 
	assert(m_pRayGenLocalRootSignature == nullptr);
	RootSignatureDesc rayGenRootSignatureDesc(kRayGenNumRootParams, rayGenRootParams, D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE);
	m_pRayGenLocalRootSignature = new RootSignature(m_pRenderEnv->m_pDevice, &rayGenRootSignatureDesc, L"m_pRayGenLocalRootSignature");

	assert(m_pEmptyLocalRootSignature == nullptr);
	RootSignatureDesc emptyLocalRootSignatureDesc(D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE);
	m_pEmptyLocalRootSignature = new RootSignature(m_pRenderEnv->m_pDevice, &emptyLocalRootSignatureDesc, L"m_pEmptyLocalRootSignature");
}

void DXApplication::CreateStateObject()
{
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
	
	LPCWSTR pRayGenShaderName = L"RayGeneration";
	LPCWSTR pMissShaderName = L"RayMiss";
	LPCWSTR pClosesHitShaderName = L"RayClosestHit";
	LPCWSTR pHitGroupName = L"HitGroup";

	ExportDesc exportDescs[] =
	{
		ExportDesc(pRayGenShaderName),
		ExportDesc(pMissShaderName),
		ExportDesc(pClosesHitShaderName)
	};

	DXILLibraryDesc libraryDesc(shaderLibrary.GetBytecode(), ARRAYSIZE(exportDescs), exportDescs);
	stateSubobjects[kDXILLibraryIndex] = StateSubobject(D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY, &libraryDesc);

	HitGroupDesc hitGroupDesc(pHitGroupName, D3D12_HIT_GROUP_TYPE_TRIANGLES, nullptr, pClosesHitShaderName, nullptr);
	stateSubobjects[kHitGroupIndex] = StateSubobject(D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP, &hitGroupDesc);

	RayTracingPipelineConfig pipelineConfig(1);
	stateSubobjects[kPipelineConfigIndex] = StateSubobject(D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG, &pipelineConfig);

	RayTracingShaderConfig shaderConfig(3 * sizeof(f32)/*color*/, 2 * sizeof(f32)/*intersectionAttribs*/);
	stateSubobjects[kShaderConfigIndex] = StateSubobject(D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG, &shaderConfig);

	assert(m_pRayGenLocalRootSignature != nullptr);
	stateSubobjects[kRayGenLocalRSIndex] = StateSubobject(D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE, m_pRayGenLocalRootSignature);

	LPCWSTR rayGenLocalRSExports[] = {pRayGenShaderName};
	SubobjectToExportsAssociation rayGenLocalRSExportsAssoc(&stateSubobjects[kRayGenLocalRSIndex], ARRAYSIZE(rayGenLocalRSExports), rayGenLocalRSExports);
	stateSubobjects[kRayGenLocalRSAssocIndex] = StateSubobject(D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION, &rayGenLocalRSExportsAssoc);

	assert(m_pEmptyLocalRootSignature != nullptr);
	stateSubobjects[kEmptyLocalRSIndex] = StateSubobject(D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE, m_pEmptyLocalRootSignature);
	
	LPCWSTR emptyLocalRSExports[] = {pMissShaderName, pHitGroupName};
	SubobjectToExportsAssociation emptyLocalRSExportsAssoc(&stateSubobjects[kEmptyLocalRSIndex], ARRAYSIZE(emptyLocalRSExports), emptyLocalRSExports);
	stateSubobjects[kEmptyLocalRSAssocIndex] = StateSubobject(D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION, &emptyLocalRSExportsAssoc);

	assert(m_pStateObject == nullptr);
	StateObjectDesc stateObjectDesc(D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE, kNumStateObjects, stateSubobjects);
	m_pStateObject = new StateObject(m_pRenderEnv->m_pDevice, &stateObjectDesc, L"m_pStateObject");
}

