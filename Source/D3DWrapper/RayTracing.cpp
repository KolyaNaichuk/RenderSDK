#include "D3DWrapper/RayTracing.h"
#include "D3DWrapper/GraphicsResource.h"

BuildRayTracingAccelerationStructureDesc::BuildRayTracingAccelerationStructureDesc(const D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS* pInputs,
	Buffer* pDestBuffer, Buffer* pScratchBuffer, Buffer* pSourceBuffer)
{
	assert(pDestBuffer != nullptr);
	assert(pScratchBuffer != nullptr);

	DestAccelerationStructureData = pDestBuffer->GetGPUVirtualAddress();
	Inputs = *pInputs;
	SourceAccelerationStructureData = (pSourceBuffer != nullptr) ? pSourceBuffer->GetGPUVirtualAddress() : 0;
	ScratchAccelerationStructureData = pScratchBuffer->GetGPUVirtualAddress();
}

ShaderRecord::ShaderRecord(void* pShaderIdentifier, UINT shaderIdentifierSizeInBytes)
	: ShaderRecord(pShaderIdentifier, shaderIdentifierSizeInBytes, nullptr, 0)
{
}

ShaderRecord::ShaderRecord(void* pShaderIdentifier, UINT shaderIdentifierSizeInBytes, void* pLocalRootArguments, UINT localRootArgumentsSizeInBytes)
	: m_pShaderIdentifier(pShaderIdentifier)
	, m_ShaderIdentifierSizeInBytes(shaderIdentifierSizeInBytes)
	, m_pLocalRootArguments(pLocalRootArguments)
	, m_LocalRootArgumentsSizeInBytes(localRootArgumentsSizeInBytes)
{
}

ShaderRecordData::ShaderRecordData(UINT maxNumRecords, UINT recordSizeInBytes)
{
	assert(maxNumRecords > 0);
	assert(recordSizeInBytes > 0);

	m_RecordSizeInBytes = Align(recordSizeInBytes, D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT);

	m_SizeInBytes = 0;
	m_MaxSizeInBytes = maxNumRecords * m_RecordSizeInBytes;
	m_NumRecords = 0;

	m_pData = new BYTE[m_MaxSizeInBytes];
}

ShaderRecordData::~ShaderRecordData()
{
	SafeArrayDelete(m_pData);
}

UINT ShaderRecordData::GetNumRecords() const
{
	return m_NumRecords;
}

UINT ShaderRecordData::GetSizeInBytes() const
{
	return m_SizeInBytes;
}

const void* ShaderRecordData::GetData() const
{
	return m_pData;
}

void ShaderRecordData::Append(const ShaderRecord& record)
{
	assert(m_SizeInBytes + m_RecordSizeInBytes <= m_MaxSizeInBytes);

	assert(record.m_pShaderIdentifier != nullptr);
	assert(record.m_ShaderIdentifierSizeInBytes > 0);

	BYTE* pDestMem = m_pData + m_SizeInBytes;
	CopyMemory(pDestMem, record.m_pShaderIdentifier, record.m_ShaderIdentifierSizeInBytes);

	if (record.m_pLocalRootArguments != nullptr)
	{
		assert(record.m_LocalRootArgumentsSizeInBytes > 0);

		pDestMem += record.m_ShaderIdentifierSizeInBytes;
		CopyMemory(pDestMem, record.m_pLocalRootArguments, record.m_LocalRootArgumentsSizeInBytes);
	}

	m_SizeInBytes += m_RecordSizeInBytes;
	++m_NumRecords;
}

void ShaderRecordData::Reset()
{
	m_SizeInBytes = 0;
	m_NumRecords = 0;
}