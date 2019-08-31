#include "D3DWrapper/StateObject.h"
#include "D3DWrapper/RootSignature.h"
#include "D3DWrapper/GraphicsDevice.h"

StateSubobject::StateSubobject(D3D12_STATE_SUBOBJECT_TYPE type, const void* pSubobjectDesc)
{
	Type = type;
	pDesc = pSubobjectDesc;
}

StateObjectDesc::StateObjectDesc(D3D12_STATE_OBJECT_TYPE type, UINT numSubobjects, const D3D12_STATE_SUBOBJECT* pFirstSubobject)
{
	Type = type;
	NumSubobjects = numSubobjects;
	pSubobjects = pFirstSubobject;
}

RayTracingPipelineConfig::RayTracingPipelineConfig(UINT maxTraceRecursionDepth)
{
	MaxTraceRecursionDepth = maxTraceRecursionDepth;
}

RayTracingShaderConfig::RayTracingShaderConfig(UINT maxPayloadSizeInBytes, UINT maxAttributeSizeInBytes)
{
	MaxPayloadSizeInBytes = maxPayloadSizeInBytes;
	MaxAttributeSizeInBytes = maxAttributeSizeInBytes;
}

ExportDesc::ExportDesc(LPCWSTR pName, LPCWSTR pExportToRename, D3D12_EXPORT_FLAGS flags)
{
	Name = pName;
	ExportToRename = pExportToRename;
	Flags = flags;
}

DXILLibraryDesc::DXILLibraryDesc(D3D12_SHADER_BYTECODE libraryBytecode, UINT numExportDescs, ExportDesc* pExportDescs)
{
	DXILLibrary = libraryBytecode;
	NumExports = numExportDescs;
	pExports = pExportDescs;
}

HitGroupDesc::HitGroupDesc(LPCWSTR pHitGroupName, D3D12_HIT_GROUP_TYPE type, LPCWSTR pAnyHitShaderName, LPCWSTR pClosestHitShaderName, LPCWSTR pIntersectionShaderName)
{
	HitGroupExport = pHitGroupName;
	Type = type;
	AnyHitShaderImport = pAnyHitShaderName;
	ClosestHitShaderImport = pClosestHitShaderName;
	IntersectionShaderImport = pIntersectionShaderName;
}

SubobjectToExportsAssociation::SubobjectToExportsAssociation(const D3D12_STATE_SUBOBJECT* pSubobjectToAssoc, UINT numExports, LPCWSTR* pFirstExport)
{
	pSubobjectToAssociate = pSubobjectToAssoc;
	NumExports = numExports;
	pExports = pFirstExport;
}

GlobalRootSignature::GlobalRootSignature(RootSignature* pGlobalRootSig)
{
	pGlobalRootSignature = pGlobalRootSig->GetD3DObject();
}

LocalRootSignature::LocalRootSignature(RootSignature* pLocalRootSig)
{
	pLocalRootSignature = pLocalRootSig->GetD3DObject();
}

StateObject::StateObject(GraphicsDevice* pDevice, const StateObjectDesc* pDesc, LPCWSTR pName)
{
	ID3D12Device5* pD3DDevice = pDevice->GetD3DObject();
	VerifyD3DResult(pD3DDevice->CreateStateObject(pDesc, IID_PPV_ARGS(&m_D3DStateObject)));

#ifdef ENABLE_GRAPHICS_DEBUGGING
	m_D3DStateObject->SetName(pName);
#endif // ENABLE_GRAPHICS_DEBUGGING

	VerifyD3DResult(m_D3DStateObject->QueryInterface(IID_PPV_ARGS(&m_D3DStateObjectProperties)));
}

void* StateObject::GetShaderIdentifier(LPCWSTR pExportName)
{
	return m_D3DStateObjectProperties->GetShaderIdentifier(pExportName);
}
