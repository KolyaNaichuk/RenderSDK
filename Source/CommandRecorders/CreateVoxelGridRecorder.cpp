#include "CommandRecorders/CreateVoxelGridRecorder.h"
#include "DX/DXPipelineState.h"
#include "DX/DXRootSignature.h"
#include "DX/DXCommandList.h"
#include "DX/DXResource.h"

enum RootParams
{
	kObjectTransformCBVRootParam = 0,
	kCameraTransformCBVRootParam,
	kGridConfigCBVRootParam,
	kColorSRVRootParam,
	kColorSamplerRootParam,
	kGridBufferUAVRootParam,
	kNumRootParams
};
/*
class Mesh
{
public:
	Mesh();
	~Mesh();

	Mesh(const Mesh& mesh) = delete;
	Mesh& operator= (const Mesh& mesh) = delete;

	u32 GetNumFaces() const;
	u32 GetNumVertices() const;
		
	const u16* Get16BitIndices() const;
	void Set16BitIndices(u32 numFaces, const u16* pIndices);

	const u32* Get32BitIndices() const;
	void Set32BitIndices(u32 numFaces, const u32* pIndices);

	const Vector3f* GetPositions() const;
	void SetPositions(u32 numVertices, const Vector3f* pPositions);

	const Vector3f* GetNormals() const;
	void SetNormals(u32 numVertices, const Vector3f* pNormals);

	const Vector2f* GetTexCoords() const;
	void SetTexCoords(u32 numVertices, const Vector2f* pTexCoords);

	void Clear();

private:
	u32 m_NumFaces;
	u32 m_NumVertices;

	u16* m_Indices16Bit;
	u32* m_Indices32Bit;

	Vector3f* m_Positions;
	Vector3f* m_Normals;
	Vector2f* m_TexCoords;
};
*/

CreateVoxelGridRecorder::CreateVoxelGridRecorder(DXDevice* pDevice)
	: m_pRootSignature(nullptr)
	, m_pPipelineState(nullptr)
{
	DXShader vertexShader(L"CreateVoxelGridVS.hlsl", "Main", "vs_4_0");
	DXShader geometryShader(L"CreateVoxelGridGS.hlsl", "Main", "gs_4_0");
	DXShader pixelShader(L"CreateVoxelGridPS.hlsl", "Main", "ps_5_1");

	DXCBVRange objectTransformCBVRange(1, 0);
	DXCBVRange cameraTransformCBVRange(1, 0);
	DXCBVRange gridConfigCBVRange(1, 0);
	DXSRVRange colorSRVRange(1, 0);
	DXSamplerRange colorSamplerRange(1, 0);
	DXUAVRange gridBufferUAVRange(1, 0);

	D3D12_ROOT_PARAMETER rootParams[kNumRootParams];
	rootParams[kObjectTransformCBVRootParam] = DXRootDescriptorTableParameter(1, &objectTransformCBVRange, D3D12_SHADER_VISIBILITY_VERTEX);
	rootParams[kCameraTransformCBVRootParam] = DXRootDescriptorTableParameter(1, &cameraTransformCBVRange, D3D12_SHADER_VISIBILITY_GEOMETRY);
	rootParams[kGridConfigCBVRootParam] = DXRootDescriptorTableParameter(1, &gridConfigCBVRange, D3D12_SHADER_VISIBILITY_PIXEL);
	rootParams[kColorSRVRootParam] = DXRootDescriptorTableParameter(1, &colorSRVRange, D3D12_SHADER_VISIBILITY_PIXEL);
	rootParams[kColorSamplerRootParam] = DXRootDescriptorTableParameter(1, &colorSamplerRange, D3D12_SHADER_VISIBILITY_PIXEL);
	rootParams[kGridBufferUAVRootParam] = DXRootDescriptorTableParameter(1, &gridBufferUAVRange, D3D12_SHADER_VISIBILITY_PIXEL);
	
	DXRootSignatureDesc rootSignatureDesc(kNumRootParams, rootParams, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
	m_pRootSignature = new DXRootSignature(pDevice, &rootSignatureDesc, L"CreateVoxelGridRecorder::m_pRootSignature");

	const DXInputElementDesc inputElementDescs[] = {
		DXInputElementDesc("POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0),
		DXInputElementDesc("NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12),
		DXInputElementDesc("TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24)
	};
	
	DXGraphicsPipelineStateDesc pipelineStateDesc;
	pipelineStateDesc.SetRootSignature(m_pRootSignature);
	pipelineStateDesc.SetVertexShader(&vertexShader);
	pipelineStateDesc.SetGeometryShader(&geometryShader);
	pipelineStateDesc.SetPixelShader(&pixelShader);
	pipelineStateDesc.SetInputLayout(ARRAYSIZE(inputElementDescs), inputElementDescs);
	pipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

	m_pPipelineState = new DXPipelineState(pDevice, &pipelineStateDesc, L"CreateVoxelGridRecorder::m_pPipelineState");
}

CreateVoxelGridRecorder::~CreateVoxelGridRecorder()
{
	delete m_pPipelineState;
	delete m_pRootSignature;
}

void CreateVoxelGridRecorder::Record(DXCommandList* pCommandList, DXCommandAllocator* pCommandAllocator,
	UINT numDXDescriptorHeaps, ID3D12DescriptorHeap* pDXFirstDescriptorHeap,
	D3D12_GPU_DESCRIPTOR_HANDLE objectTransformCBVDescriptor,
	D3D12_GPU_DESCRIPTOR_HANDLE cameraTransformCBVDescriptor,
	D3D12_GPU_DESCRIPTOR_HANDLE gridConfigCBVDescriptor,
	DXResource* pGridBuffer, D3D12_GPU_DESCRIPTOR_HANDLE gridBufferUAVDescriptor,
	DXResource* pColorTexture, D3D12_GPU_DESCRIPTOR_HANDLE colorSRVDescriptor,
	D3D12_GPU_DESCRIPTOR_HANDLE colorSamplerDescriptor,
	const D3D12_RESOURCE_STATES* pGridBufferEndState,
	const D3D12_RESOURCE_STATES* pColorTextureEndState)
{
	pCommandList->Reset(pCommandAllocator, m_pPipelineState);
	pCommandList->SetGraphicsRootSignature(m_pRootSignature);
	
	if (pGridBuffer->GetState() != D3D12_RESOURCE_STATE_UNORDERED_ACCESS)
		pCommandList->TransitionBarrier(pGridBuffer, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

	if (pColorTexture->GetState() != D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE)
		pCommandList->TransitionBarrier(pColorTexture, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

	pCommandList->SetDescriptorHeaps(numDXDescriptorHeaps, &pDXFirstDescriptorHeap);
	pCommandList->SetGraphicsRootDescriptorTable(kObjectTransformCBVRootParam, objectTransformCBVDescriptor);
	pCommandList->SetGraphicsRootDescriptorTable(kCameraTransformCBVRootParam, cameraTransformCBVDescriptor);
	pCommandList->SetGraphicsRootDescriptorTable(kGridConfigCBVRootParam, gridConfigCBVDescriptor);
	pCommandList->SetGraphicsRootDescriptorTable(kColorSRVRootParam, colorSRVDescriptor);
	pCommandList->SetGraphicsRootDescriptorTable(kColorSamplerRootParam, colorSamplerDescriptor);
	pCommandList->SetGraphicsRootDescriptorTable(kGridBufferUAVRootParam, gridBufferUAVDescriptor);

	pCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	//pCommandList->IASetVertexBuffers();
	//pCommandList->IASetIndexBuffer();

	if (pGridBufferEndState != nullptr)
		pCommandList->TransitionBarrier(pGridBuffer, *pGridBufferEndState);

	if (pColorTextureEndState != nullptr)
		pCommandList->TransitionBarrier(pColorTexture, *pColorTextureEndState);

	pCommandList->Close();
}
