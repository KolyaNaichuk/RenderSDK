#pragma once

#include "D3DWrapper/Common.h"

class Buffer;

struct RayTracingGeometryDesc : D3D12_RAYTRACING_GEOMETRY_DESC
{
};

struct RayTracingInstanceDesc : D3D12_RAYTRACING_INSTANCE_DESC
{
};

struct BuildRayTracingAccelerationStructureInputs : D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS
{
};

struct RayTracingAccelerationStructurePrebuildInfo : D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO
{
};

struct BuildRayTracingAccelerationStructureDesc : D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC
{
	BuildRayTracingAccelerationStructureDesc(const D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS* pInputs,
		Buffer* pDestBuffer, Buffer* pScratchBuffer, Buffer* pSourceBuffer = nullptr);
};

struct ShaderRecord
{
	ShaderRecord(void* pShaderIdentifier, UINT shaderIdentifierSizeInBytes);
	ShaderRecord(void* pShaderIdentifier, UINT shaderIdentifierSizeInBytes, void* pLocalRootArguments, UINT localRootArgumentsSizeInBytes);

	void* m_pShaderIdentifier;
	UINT m_ShaderIdentifierSizeInBytes;

	void* m_pLocalRootArguments;
	UINT m_LocalRootArgumentsSizeInBytes;
};

class ShaderRecordData
{
public:
	ShaderRecordData(UINT maxNumRecords, UINT recordSizeInBytes);
	~ShaderRecordData();

	UINT GetNumRecords() const;
	UINT GetSizeInBytes() const;
	const void* GetData() const;

	void Append(const ShaderRecord& record);
	void Reset();

private:
	BYTE* m_pData;
	UINT m_RecordSizeInBytes;
	UINT m_SizeInBytes;
	UINT m_MaxSizeInBytes;
	UINT m_NumRecords;
};