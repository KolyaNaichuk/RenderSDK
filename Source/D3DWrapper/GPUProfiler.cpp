#include "D3DWrapper/GPUProfiler.h"
#include "D3DWrapper/GraphicsResource.h"
#include "D3DWrapper/CommandList.h"
#include "D3DWrapper/CommandQueue.h"
#include "D3DWrapper/QueryHeap.h"
#include "D3DWrapper/RenderEnv.h"
#include "Math/Math.h"

GPUProfiler::GPUProfiler(RenderEnv* pRenderEnv, u32 maxNumProfiles, u32 renderLatency)
	: m_MaxNumQueries(2 * maxNumProfiles)
{
	QueryHeapDesc queryHeapDesc(D3D12_QUERY_HEAP_TYPE_TIMESTAMP, m_MaxNumQueries);
	m_pQueryHeap = new QueryHeap(pRenderEnv->m_pDevice, &queryHeapDesc, L"Profiler::m_pQueryHeap");
	
	const u32 numTimestamps = m_MaxNumQueries * renderLatency;
	StructuredBufferDesc timestampBufferDesc(numTimestamps, sizeof(u64), false, false);
	m_pTimestampBuffer = new Buffer(pRenderEnv, pRenderEnv->m_pReadbackHeapProps, &timestampBufferDesc,
		D3D12_RESOURCE_STATE_COPY_DEST, L"Profiler::m_pTimestampBuffer");
			
	m_Profiles.resize(maxNumProfiles);
}

GPUProfiler::~GPUProfiler()
{
	SafeDelete(m_pQueryHeap);
	SafeDelete(m_pTimestampBuffer);
}

void GPUProfiler::StartFrame(u32 currentFrameIndex)
{
	m_CurrentFrameIndex = currentFrameIndex;
}

void GPUProfiler::EndFrame(CommandQueue* pCommandQueue)
{
	const u64 timestampFrequency = pCommandQueue->GetTimestampFrequency();

	SIZE_T numBytesToRead = m_MaxNumQueries * sizeof(u64);
	MemoryRange readRange(m_CurrentFrameIndex * numBytesToRead, numBytesToRead);
	const u64* pTimestampQueryData = (u64*)m_pTimestampBuffer->Map(0, &readRange);
	
	for (u32 profileIndex = 0; profileIndex < m_NumUsedProfiles; ++profileIndex)
	{
		ProfileData& profileData = m_Profiles[profileIndex];

		const u32 startQueryIndex = 2 * profileIndex;
		const u32 endQueryIndex = startQueryIndex + 1;

		u64 startTime = pTimestampQueryData[startQueryIndex];
		u64 endTime = pTimestampQueryData[endQueryIndex];
		
		f64 timeInMilliseconds = (f64(endTime - startTime) / f64(timestampFrequency)) * 1000.0;
		profileData.m_TimeSamples[profileData.m_CurrentSampleIndex] = timeInMilliseconds;
		profileData.m_CurrentSampleIndex = (profileData.m_CurrentSampleIndex + 1) % ProfileData::kNumTimeSamples;

		f64 maxTime = 0.0;
		f64 avgTime = 0.0;
		u32 numAvgSamples = 0;

		for (u32 sampleIndex = 0; sampleIndex < ProfileData::kNumTimeSamples; ++sampleIndex)
		{
			if (profileData.m_TimeSamples[sampleIndex] > 0.0f)
			{
				maxTime = Max(maxTime, profileData.m_TimeSamples[sampleIndex]);

				avgTime += profileData.m_TimeSamples[sampleIndex];
				++numAvgSamples;
			}
		}

		if (numAvgSamples > 0)
			avgTime /= f64(numAvgSamples);

		profileData.m_MaxTime = maxTime;
		profileData.m_AvgTime = avgTime;
	}

	MemoryRange writtenRange(0, 0);
	m_pTimestampBuffer->Unmap(0, &writtenRange);
}

u32 GPUProfiler::StartProfile(CommandList* pCommandList, const char* pProfileName)
{
	assert(pProfileName != nullptr);
	assert(pCommandList != nullptr);

	u32 profileIndex = 0;
	for (; profileIndex < m_NumUsedProfiles; ++profileIndex)
	{
		if (m_Profiles[profileIndex].m_Name == pProfileName)
			break;
	}
	if (profileIndex == m_NumUsedProfiles)
	{
		assert(m_NumUsedProfiles < m_Profiles.size());

		profileIndex = m_NumUsedProfiles++;
		m_Profiles[profileIndex].m_Name = pProfileName;
	}

	const u32 startQueryIndex = 2 * profileIndex;
	pCommandList->EndQuery(m_pQueryHeap, D3D12_QUERY_TYPE_TIMESTAMP, startQueryIndex);
	
	return profileIndex;
}

void GPUProfiler::EndProfile(CommandList* pCommandList, u32 profileIndex)
{
	assert(pCommandList != nullptr);
	
	const u32 startQueryIndex = 2 * profileIndex;
	const u32 endQueryIndex = startQueryIndex + 1;
	pCommandList->EndQuery(m_pQueryHeap, D3D12_QUERY_TYPE_TIMESTAMP, endQueryIndex);

	const u64 destOffset = (m_CurrentFrameIndex * m_MaxNumQueries + startQueryIndex) * sizeof(u64);
	pCommandList->ResolveQueryData(m_pQueryHeap, D3D12_QUERY_TYPE_TIMESTAMP, startQueryIndex, 2, m_pTimestampBuffer, destOffset);
}

void GPUProfiler::OutputToConsole()
{
	enum class SortOrder
	{
		RenderOrder,
		MaxToMinAvgTimeOrder
	};
	const SortOrder sortOrder = SortOrder::MaxToMinAvgTimeOrder;

	std::vector<const ProfileData*> profiles(m_NumUsedProfiles);
	for (u32 profileIndex = 0; profileIndex < m_NumUsedProfiles; ++profileIndex)
		profiles[profileIndex] = &m_Profiles[profileIndex];

	std::function<bool(const ProfileData*, const ProfileData*)> comp;
	if (sortOrder == SortOrder::RenderOrder)
	{
		comp = [](const ProfileData* left, const ProfileData* right)
		{
			return false;
		};
	}
	else if (sortOrder == SortOrder::MaxToMinAvgTimeOrder)
	{
		comp = [](const ProfileData* left, const ProfileData* right)
		{
			return (left->m_AvgTime > right->m_AvgTime);
		};
	}
	else
	{
		assert(false);
	}
	std::sort(profiles.begin(), profiles.end(), comp);

	const u8 outputBufferSize = 255;
	char outputBuffer[outputBufferSize];

	OutputDebugStringA("================================= GPU Profiler ==========================================\n");
	for (u32 profileIndex = 0; profileIndex < m_NumUsedProfiles; ++profileIndex)
	{
		const ProfileData* pProfileData = profiles[profileIndex];	
		
		std::snprintf(outputBuffer, outputBufferSize, "%s: avg: %.3f ms, max: %.3f ms\n",
			pProfileData->m_Name.c_str(), pProfileData->m_AvgTime, pProfileData->m_MaxTime);
		
		OutputDebugStringA(outputBuffer);
	}
	OutputDebugStringA("=========================================================================================\n");
}
