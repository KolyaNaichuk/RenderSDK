#pragma once

#include "D3DWrapper/Common.h"

class GraphicsDevice;
class RootSignature;

struct StateSubobject : public D3D12_STATE_SUBOBJECT
{
	StateSubobject(D3D12_STATE_SUBOBJECT_TYPE type, const void* pSubobjectDesc);
};

struct StateObjectDesc : public D3D12_STATE_OBJECT_DESC
{
	StateObjectDesc(D3D12_STATE_OBJECT_TYPE type, UINT numSubobjects, const D3D12_STATE_SUBOBJECT* pFirstSubobject);
};

struct RayTracingPipelineConfig : public D3D12_RAYTRACING_PIPELINE_CONFIG
{
	RayTracingPipelineConfig(UINT maxTraceRecursionDepth);
};

struct RayTracingShaderConfig : public D3D12_RAYTRACING_SHADER_CONFIG
{
	RayTracingShaderConfig(UINT maxPayloadSizeInBytes, UINT maxAttributeSizeInBytes);
};

struct ExportDesc : public D3D12_EXPORT_DESC
{
	ExportDesc(LPCWSTR pName, LPCWSTR pExportToRename = nullptr, D3D12_EXPORT_FLAGS flags = D3D12_EXPORT_FLAG_NONE);
};

struct DXILLibraryDesc : public D3D12_DXIL_LIBRARY_DESC
{
	DXILLibraryDesc(D3D12_SHADER_BYTECODE libraryBytecode, UINT numExportDescs, ExportDesc* pExportDescs);
};

struct HitGroupDesc : public D3D12_HIT_GROUP_DESC
{
	HitGroupDesc(LPCWSTR pHitGroupName, D3D12_HIT_GROUP_TYPE type, LPCWSTR pAnyHitShaderName, LPCWSTR pClosestHitShaderName, LPCWSTR pIntersectionShaderName);
};

struct SubobjectToExportsAssociation : public D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION
{
	SubobjectToExportsAssociation(const D3D12_STATE_SUBOBJECT* pSubobjectToAssoc, UINT numExports, LPCWSTR* pFirstExport);
};

struct GlobalRootSignature : D3D12_GLOBAL_ROOT_SIGNATURE
{
	GlobalRootSignature(RootSignature* pGlobalRootSig);
};

struct LocalRootSignature : D3D12_LOCAL_ROOT_SIGNATURE
{
	LocalRootSignature(RootSignature* pLocalRootSig);
};

class StateObject
{
public:
	StateObject(GraphicsDevice* pDevice, const StateObjectDesc* pDesc, LPCWSTR pName);
	ID3D12StateObject* GetD3DObject() { return m_D3DStateObject.Get(); }
	
	void* GetShaderIdentifier(LPCWSTR pExportName);
	
private:
	ComPtr<ID3D12StateObject> m_D3DStateObject;
	ComPtr<ID3D12StateObjectProperties> m_D3DStateObjectProperties;
};